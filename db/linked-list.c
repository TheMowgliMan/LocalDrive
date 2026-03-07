#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <syslog.h>

#define true 1
#define false 0

#define LL_FILENAME_TOO_LONG_ERROR 1
#define LL_NOT_HEAD_NODE 2
#define LL_BAD_INDEX 3
#define LL_WRONG_NODE 4

// A struct for a single node in the linked list.
struct llNode;

struct llNode {
  char fname[256];
  uint64_t size; // in bytes
  time_t timestamp;

  uint8_t is_head;
  uint32_t frevision;

  // pointers for the next and last (historical) node in the linked list
  struct llNode* next_node;
  struct llNode* prev_node;

  // pointers for the next and last node alphabetically
  struct llNode* next_alpha;
  struct llNode* prev_alpha;

  // pointers for the next and last node by filesize
  struct llNode* next_largest;
  struct llNode* next_smallest;

  // pointers for next and last node by last access
  struct llNode* next_oldest;
  struct llNode* next_youngest;
};

char* red() {
  return "\u001b[31m";
}

char* noc() {
  return "\u001b[0m";
}

void hcf(char* msg) {
  openlog("LocalDrive Database Index", LOG_PERROR | LOG_PID, LOG_MAKEPRI(LOG_FTP, LOG_CRIT));
  syslog(LOG_MAKEPRI(LOG_FTP, LOG_CRIT), "%s", msg);
  closelog();

  fprintf(stderr, "%sFatal: %s%s", red(), msg, noc());

  exit(2); // TODO: Use atexit to backup the database at crash!
}

void bump_timestamp(struct llNode* ll) {
  ll->timestamp = time(NULL);
}

int set_fname(struct llNode* ll, char* fn) {
  if (strlen(fn) > 255) {
	fprintf(stderr, "%sError: filename too long: maximum 255, got %ld.%s", red(), strlen(fn), noc());
	return LL_FILENAME_TOO_LONG_ERROR;
  }

  memcpy(ll->fname, fn, sizeof(char) * strlen(fn));
  bump_timestamp(ll);

  return 0;
}

// A safe malloc call
void* xmalloc(size_t size) {
  void *pointer = malloc(size);
  if (pointer == 0 || pointer == NULL) {
	hcf("Failed to allocate virtual memory during xmalloc call in database!");
  }

  return pointer;
}

// create a linked list
struct llNode* new_ll() {
  struct llNode* ll = (struct llNode*)xmalloc(sizeof(struct llNode));
  
  ll->is_head = true;
  ll->timestamp = time(NULL);
  ll->frevision = 0;
  ll->size = 0;

  char tmp[] = "LIST HEAD (NOT A FILE!)";
  memcpy(ll->fname, tmp, sizeof(char) * strlen(tmp));

  ll->next_node = NULL;
  ll->prev_node = NULL;

  ll->next_alpha = NULL;
  ll->prev_alpha = NULL;

  ll->next_largest = NULL;
  ll->next_smallest = NULL;

  ll->next_oldest = NULL;
  ll->next_youngest = NULL;

  return ll;
}

/* Get via Nodes:
   gets item *x* after head */
struct llNode* get_n(struct llNode* ll, uint64_t item) {
  struct llNode* now = ll;
  struct llNode* next = now->next_node;

  if (next == NULL) {
	fprintf(stderr, "%sError: invalid list index for get_n(); there are no items in this list!%s", red(), noc());
	return ll;
  }

  uint64_t i = 0;
  while (i < item) {
	now = next;
	next = now->next_node;

	i++;

	if (next == NULL) {
	  fprintf(stderr, "%sError: invalid list index for get_n(); got %ld, but length is %ld%s", red(), item, i, noc());
	  return ll;
	}
  }

  return next;
}


// Traverse Forward via Nodes: gets the last item of the list as added to it
struct llNode* tfwd_n(struct llNode* ll) {
  struct llNode* now = ll;
  struct llNode* next = now->next_node;

  while (next->is_head == false) {
	now = next;
	next = now->next_node;
  }

  return now;
}

/* Traverse One via Nodes, Relative:
   go forward or back a node along the list items */
struct llNode* tone_rn(struct llNode* ll, int8_t dir) {
  if (!(abs(dir) == 1)) {
	// dir has to equal one or negative one
	fprintf(stderr, "%sError: 'dir' for tone_rn() (aka 'Traverse One via Nodes, Relative') must be 1 or -1:\n\
			Actually got %d%s",
			red(), dir, noc());
	return ll;
  }

  if (dir == 1) {
	return ll->next_node;
  }

  return ll->prev_node;
}

/* These next two functions are aliases for tone_rn(ll, 1) and tone_rn(ll, -1) respectively */
struct llNode* next_node(struct llNode* ll) {
  return tone_rn(ll, 1);
}

struct llNode* prev_node(struct llNode* ll) {
  return tone_rn(ll, -1);
}

/* Traverse an Amount via Nodes, Relative:
   moves forward or back an amount along the list items */
struct llNode* tamt_rn(struct llNode* ll, int64_t start, int32_t offset) {
  int8_t direction = offset / abs(offset); // This should be 1 or -1 for `tone_rn()` above
  struct llNode *now = get_n(ll, start);
  
  for (int32_t i = 0; i != offset; i += direction) {
	now = tone_rn(now, direction);
  }

  return now;
}

// Traverse Forwards Alphabetically, Sorted: gets the highest fname item that is less than the input string
struct llNode* tfwd_as(struct llNode* ll, char* str) {
  struct llNode* now = ll;
  struct llNode* next = now->next_alpha;

  while (next->is_head == false && strcmp(next->fname, str) < 0) {
	now = next;
	next = now->next_alpha;
  }

  return now;
}

/* Traverse Forwards by Size, Sorted:
   gets the biggest item smaller than size */
struct llNode* tfwd_ss(struct llNode* ll, uint64_t size) {
  struct llNode* now = ll;
  struct llNode* next = now->next_largest;

  while (next->is_head == false && next->size < size) {
	now = next;
	next = now->next_largest;
  }

  return now;
}

/* Traverse Forwards by Timestamp, Sorted:
   gets the oldest item newer than size */
struct llNode* tfwd_ts(struct llNode* ll, int64_t size) {
  struct llNode* now = ll;
  struct llNode* next = now->next_oldest;

  while (next->is_head == false && next->timestamp > size) {
	now = next;
	next = now->next_oldest;
  }

  return now;
}

// appends an item to the ll (must be the head node!)
int append(struct llNode* ll, char* fn, uint64_t fsize, uint32_t revision) {
  if (ll->is_head == false) {
	fprintf(stderr, "%sError: must pass head node of linked list to append()!%s", red(), noc());
	return LL_NOT_HEAD_NODE;
  }
  struct llNode* lla = (struct llNode*)xmalloc(sizeof(struct llNode));

  lla->is_head = false;
  lla->timestamp = time(NULL);
  lla->frevision = revision;
  lla->size = fsize;

  if (strlen(fn) > 255) {
	fprintf(stderr, "%sError: filename max length is 255, got %ld%s", red(), strlen(fn), noc());
	return LL_FILENAME_TOO_LONG_ERROR;
  }

  memcpy(lla->fname, fn, sizeof(char) * strlen(fn));

  if (ll->next_node == NULL) {
	// this means that the head passed is the head of an empty alist
	ll->next_node = lla;
	ll->prev_node = lla;
	ll->next_alpha = lla;
	ll->prev_alpha = lla;
	ll->next_largest = lla;
	ll->next_smallest = lla;
	ll->next_oldest = lla;
	ll->next_youngest = lla;

	lla->next_node = ll;
	lla->prev_node = ll;
	lla->next_alpha = ll;
	lla->prev_alpha = ll;
	lla->next_largest = ll;
	lla->next_smallest = ll;
  } else {
	// there are more items to the list
	struct llNode *current_end = tfwd_n(ll);

	current_end->next_node = lla;
	lla->prev_node = current_end;

	ll->prev_node = lla; // remember that ll is always the head (i.e., first node) of the linked list
	lla->next_node = ll;

	current_end = tfwd_as(ll, fn);

	lla->next_alpha = current_end->next_alpha;
	lla->prev_alpha = current_end;
	current_end->next_alpha = lla;

	current_end = tfwd_ss(ll, fsize);

	lla->next_largest = current_end->next_largest;
	lla->next_smallest = current_end;
	current_end->next_largest = lla;

	current_end = ll->next_youngest; // This is always going to be the youngest item as far as our resolution can tell

	lla->next_oldest = current_end->next_oldest;
	lla->next_youngest = current_end;
	current_end->next_oldest = lla;
  }

  return 0;
}

/* Delete by Pointer:
   deletes the node corresponding to a struct `llNode*` pointer. */
int del_p(struct llNode* ptr) {
  if (ptr->next_node == ptr) {
	// The pointer is the last one
	ptr->next_node = NULL; // This will cause the node to become deletable
  }
  
  if (ptr->next_node != NULL) {
	if (ptr->is_head == true) {
	  fprintf(stderr, "%sError: Cannot free the head node of a non-empty list! \n%s", red(), noc());
	  return LL_WRONG_NODE;
	} else {
	  // We have to unlink the node before we can free it
	  ptr->prev_node->next_node = ptr->next_node;
	  ptr->next_node->prev_node = ptr->prev_node;

	  ptr->prev_alpha->next_alpha = ptr->next_alpha;
	  ptr->next_alpha->prev_alpha = ptr->prev_alpha;

	  ptr->next_smallest->next_largest = ptr->next_largest;
	  ptr->next_largest->next_smallest = ptr->next_smallest;

	  ptr->next_youngest->next_oldest = ptr->next_oldest;
	  ptr->next_oldest->next_youngest = ptr->next_youngest;

	  free(ptr);

	}
  } else {
	free(ptr);
  }

  return 0;
}

/* Delete by Node:
   deletes the node corresponding to an index. Not recommended if you already have the pointer; use del_p() then */
int del(struct llNode* ll, uint64_t idx) {
  struct llNode* deleting = get_n(ll, idx);
  if (deleting == ll) {
	return LL_BAD_INDEX;
  }

  return del_p(deleting);
}

// Does what it says
int delete_the_whole_entire_list(struct llNode* ll) {
  while (!(ll->next_node = ll->prev_node)) {
	del_p(ll->next_node);
  }

  del_p(ll->next_node);
  ll->next_node = NULL; // Allows the head node to be free'd
  del_p(ll);
  
  return 0;
}

// tests
int main() {
  printf("Test 1: basic append() and get_n()\n");
  printf("Creating linked list...\n");
  struct llNode *t = new_ll();

  printf("Adding items...\n");
  append(t, "one", 1, 0);
  append(t, "two", 1, 0);
  append(t, "three", 1, 0);
  append(t, "four", 1, 0);

  printf("Item 1: %s \n", get_n(t, 1)->fname);
  printf("Item 3: %s \n", get_n(t, 3)->fname);

  printf("Sizeof(t): %ld \n", sizeof(struct llNode));

  printf("Test 2: appending and iterating 100,000 nodes\n");

  printf("Creating linked list...\n");
  t = new_ll();

  printf("Appending 100,000 items...\n");
  for (int i = 0; i < 100000; i++) {
	append(t, "some filename", 1, 0);
	if (i % 5000 == 0) {
	  printf("  %dth item...\n", i);
	}
  }

  printf("Iterating 100,000 items...\n");
  char new_fname[] = "new filename!";
  for (struct llNode* iter = t; !(iter->next_node->is_head == true); iter = next_node(iter)) {
	set_fname(iter, new_fname);
  }

  printf("Deleting the list...\n");
  delete_the_whole_entire_list(t);

  hcf("No more tests to run!");
  
  return 0;
}
