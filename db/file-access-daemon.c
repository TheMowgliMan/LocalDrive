#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>

#include "../util.h"
#include "linked-list.h"
#include "fileio.h"

#define LOGINSTATUS_OK 0 // Login or logout performed normally
#define LOGINSTATUS_WRONG_PASSWORD 1 // The username-password hash did not match
#define LOGINSTATUS_TIMEOUT 2 // The user was logged out because their session ran out
#define LOGINSTATUS_LOGGED_OUT 3 // The login or logout attempt failed because there is no login

#define ACCEPTABLE_DELAY 6 // Seconds backwards the timestamp may be during login attempt
#define SESSION_LENGTH 600 // Seconds after last activity to log the user out and invalidate the access key

struct userLoginInfo { // Maximum danger
  time_t last_login_stamp;
  uint64_t access_key;
  uint8_t is_logged_in;
  uint8_t status;
};

struct userFileIOCtxSll {
  struct fileIOCtx* ioctx;
  struct userFileIOCtxSll* next;
};

struct userWrapper {
  struct llNode* user_meta; // While llNode* from linked-list.h is for storing files, it works for other things too
  struct llNode* user_files;

  uint64_t password_hash;
  uint64_t username_hash;

  struct userLoginInfo* login;

  struct userFileIOCtxSll* open_files;
};

struct allUserData {
  uint32_t count;
  struct userWrapper** users; // Have to use pointer-to-pointers because it's an array!
} users_meta;


// Used to link users together
struct llNode* users_meta_head = NULL;

struct userWrapper* users;


static uint64_t starter = 200560490131;
static const uint8_t NAME_LEN = 21; // 20 digits for uint64_t + 1 for "\0"


uint64_t generate_access_key() {
  #ifdef _POSIX_VERSION
  FILE *r = fopen("/dev/urandom/", "r");
  if (!r) {
	hcf("Cannot generate access keys, as opening '/dev/urandom/' failed!",
		"uint64_t generate_access_key() @ file-access-daemon.c")
  }

  uint64_t rnum;
  read(r, &rnum, sizeof(uint64_t));

  return rnum;
  
  #else
  hcf("Cannot generate access keys, as '/dev/urandom/' does not exist!",
	  "uint64_t generate_access_key() @ file-access-daemon.c");
  #endif

  return 0;
}

struct userLoginInfo* generate_login() {
  struct userLoginInfo* l = (struct userLoginInfo*)xmalloc(sizeof(struct userLoginInfo),
														   "struct userLoginInfo* generate_login() @ file-access-daemon.c");

  l->last_login_stamp = 0; // Hasn't logged in since last start
  l->is_logged_in = false;
  l->access_key = 0;

  return l;
}

struct userWrapper* generate_user(const char* name) {
  struct userWrapper* u = (struct userWrapper*)xmalloc(sizeof(struct userWrapper),
													   "struct userWrapper* generate_user() @ file-access-daemon.c");

  append(users_meta_head, name, 0, 0);
  u->user_meta = prev_node(users_meta_head);
  u->user_files = new_ll();

  u->login = generate_login();
  u->open_files = NULL;

  return u;
}

// Very terrible 64-bit hash function
uint64_t hash_char(char character, uint32_t i) {
  srand((character + i) * i);
  
  uint64_t c = (uint64_t)character;
  uint64_t not = ~(((c + i * rand()) * 17) << 0x03);
  uint64_t and = ((c + i) << 2 * rand()) & ((c << 5) * 217);
  uint64_t or = (c + i * 2967) << 5 | (c * rand()) << 3;

  return ((c * i) + (((c * i * 1111111111111111111) ^ not) & (and ^ or * 658493658747))) ^ (i * 4977177437865208637);
}

uint64_t hash_as_number(const char* password) {
  uint64_t hash = starter;

  for (int i = 0; password[i]; i++) {
	hash *= hash ^ hash_char(password[i], i);
  }

  return hash;
}

int initialize(char* root_password) {
  users_meta_head = new_ll();
  
  // We use an array of users because it doesn't change often
  // ...maybe make it a linked list?
  users_meta.users = (struct userWrapper**)xmalloc(sizeof(struct userWrapper**),
												   "int initialize() @ file-access-daemon.c");
  users_meta.users[0] = generate_user("root");
  users_meta.users[0]->password_hash = hash_as_number(root_password);
  users_meta.users[0]->username_hash = 0x0000000000000000;
  users_meta.count = 1;

  return 0;
}

char* get_user_folder_name(struct userWrapper* user) {
  uint64_t hash = starter;

  for(int i = 0; user->user_meta->fname[i]; i++) {
	hash *= hash ^ hash_char(user->user_meta->fname[i], i);
  }

  if (strcmp(user->user_meta->fname, "root") == 0) {
	hash = 0x0000000000000000;
  }

  char* strhash = (char*)xmalloc(sizeof(char) * NAME_LEN,
								 "char* get_user_folder_name() @ file-access-daemon.c");
  snprintf(strhash, (size_t)NAME_LEN, "%" PRIu64, hash);

  return strhash;
}

int addUser(const char* name, const char* pwrd) {
  struct userWrapper* u = generate_user(name);

  users_meta.count += 1;

  struct userWrapper** new_array = realloc(users_meta.users, sizeof(struct userWrapper) * users_meta.count);
  if (!new_array) {
	hcf("Failed realloc to add new user!", "int addUser() @ file-access-daemon.c");
  }

  char* fldrn = get_user_folder_name(u);
  u->username_hash = hash_as_number(name);

  u->password_hash = hash_as_number(pwrd);

  users_meta.users = new_array;
  users_meta.users[users_meta.count - 1] = u;

  errno = 0;
  mkdir(fldrn, 0777);
  if (errno != 0) {
	if (errno = EEXIST) {
	  ; // No news is good news
	} else {
	  hcf(strerror(errno), "int mkdir() @ int addUser() @ file-access-daemon.c");
	}
  }

  return 0;
}

int userRegisterFile(struct userWrapper* user, const char* fname, uint64_t ts, uint64_t fsize, uint32_t fr) {
  append(user->user_files, fname, fsize, fr);
  prev_node(user->user_files)->timestamp = (time_t)ts;

  return 0;
}

uint8_t userFileExists(struct userWrapper* user, const char* fname) {
  for (struct llNode* fsearch = user->user_files;
	   !(fsearch->next_node->is_head);
	   fsearch = fsearch->next_node) {
	// We don't actually have to use the OS I/O systems,
	// Because we have this handy-dandy linked-list.
	if (strcmp(fsearch->fname, fname) == 0) {
	  return true;
	}

	// Just keeeeeep loopin'...
  }

  return false;
}

int login(struct userWrapper* user, uint64_t unph) { // "unph": username and password hash, also XOR'ed with timestamp
  uint8_t timeout = false;
  
  if (user->login->is_logged_in && (now() - user->login->last_login_stamp) <= SESSION_LENGTH) {
	user->login->last_login_stamp = now();

	if (user->login->access_key == 0) user->login->access_key = generate_access_key();

	return LOGINSTATUS_OK;
  } else if (user->login->is_logged_in) { /* We know that the user's session timed out because of the above if conditional */
	timeout = true;
  }

  for (int i = 0; i <= ACCEPTABLE_DELAY; i++) {
	if ((user->username_hash ^ user->password_hash ^ (now() - i)) == unph) {
	  user->login->is_logged_in = true;
	  user->login->last_login_stamp = now();
	  user->login->access_key = generate_access_key();

	  return LOGINSTATUS_OK;
	}
  }

  if (timeout) return LOGINSTATUS_TIMEOUT; else return LOGINSTATUS_WRONG_PASSWORD;
}

int check_login(struct userWrapper* user) {
  if (user->login->is_logged_in) {
	if ((now() - user->login->last_login_stamp) <= SESSION_LENGTH) {
	  user->login->last_login_stamp = now();
	  return LOGINSTATUS_OK;
	} else {
	  user->login->access_key = 0;
	  return LOGINSTATUS_TIMEOUT;
	}
  }

  return LOGINSTATUS_LOGGED_OUT;
}

uint64_t get_access_key(struct userWrapper* user) {
  return user->login->access_key;
}

int check_access_key(struct userWrapper* user, uint64_t access_key) {
  if ((now() - user->login->last_login_stamp) > SESSION_LENGTH) {
	user->login->access_key = 0;
	return LOGINSTATUS_TIMEOUT;
  }
  if (get_access_key(user) == access_key) return LOGINSTATUS_OK; else return LOGINSTATUS_WRONG_PASSWORD;
  // be sure to run check_login() after this
}

struct fileIOCtx* open_file_context(struct userWrapper* user, char* fname) {
  struct fileIOCtx* ioctx = fileIOCtxInit(fname);

  struct userFileIOCtxSll* f =
	(struct userFileIOCtxSll*)xmalloc(sizeof(struct userFileIOCtxSll),
									  "struct fileIOCtx* open_file_context() @ file-access-daemon.c");

  f->ioctx = ioctx;
  f->next = NULL;
  
  if (user->open_files == NULL) {
	user->open_files = f;
  } else {
	struct userFileIOCtxSll* cur = user->open_files;
	
	while (cur != NULL) {
	  cur = cur->next;
	}

	cur = f;
  }
  
  return ioctx;
}

int openUserFile(struct userWrapper* user, char* fname, uint64_t access_key, uint64_t unph) {
  if (access_key && unph == false) {
	return LOGINSTATUS_LOGGED_OUT;
  }

  if (access_key != 0) {
	int is_valid = check_access_key(user, access_key);
	if (is_valid != LOGINSTATUS_OK && unph == 0) {
	  return is_valid;
	} else {
	  if (is_valid != LOGINSTATUS_OK) {
		is_valid = login(user, unph);

		if (is_valid != LOGINSTATUS_OK) {
		  return is_valid;
		}
	  }
	}
  }
}
