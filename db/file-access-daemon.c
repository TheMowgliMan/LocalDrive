#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "../util.h"
#include "linked-list.h"
#include "fileio.h"

struct userWrapper {
  struct llNode* user_meta; // While llNode* from linked-list.h is for storing files, it works for other things too
  struct llNode* user_files;
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

struct userWrapper* generate_user(char* name) {
  struct userWrapper* u = (struct userWrapper*)xmalloc(sizeof(struct userWrapper),
													   "struct userWrapper* generate_user() @ file-access-daemon.c");

  append(users_meta_head, name, 0, 0);
  u->user_meta = prev_node(users_meta_head);
  u->user_files = new_ll();

  return u;
}

int initialize() {
  users_meta_head = new_ll();
  
  // We use an array of users because it doesn't change often
  // ...maybe make it a linked list?
  users_meta.users = (struct userWrapper**)xmalloc(sizeof(struct userWrapper**),
												   "int initialize() @ file-access-daemon.c");
  users_meta.users[0] = generate_user("root");
  users_meta.count = 1;

  return 0;
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

char* get_user_folder_name(struct userWrapper* user) {
  uint64_t hash = starter;

  for(int i = 0; user->user_meta->fname[i]; i++) {
	hash *= hash ^ hash_char(user->user_meta->fname[i], i);
  }

  char* strhash = (char*)xmalloc(sizeof(char) * NAME_LEN,
								 "char* get_user_folder_name() @ file-access-daemon.c");
  snprintf(strhash, (size_t)NAME_LEN, "%" PRIu64, hash);

  return strhash;
}

int addUser(char* name) {
  struct userWrapper* u = generate_user(name);

  users_meta.count += 1;

  struct userWrapper** new_array = realloc(users_meta.users, sizeof(struct userWrapper) * users_meta.count);
  if (!new_array) {
	hcf("Failed realloc to add new user!", "int addUser() @ file-access-daemon.c");
  }

  users_meta.users = new_array;
  users_meta.users[users_meta.count - 1] = u;

  return 0;
}
