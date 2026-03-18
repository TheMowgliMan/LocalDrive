#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <syslog.h>
#include "linked-list.h"
#include "../util.h"

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

  // pointers for next and last node by creation time
  struct llNode* next_oldest;
  struct llNode* next_youngest;
};

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


/* Relink Node, Alphabetically:
   disconnects the node from its place in the list and links it anew. */
int relink_a(struct llNode* ll, struct llNode* item, char* fn, int8_t force_end) {
  struct llNode* old_next = item->next_alpha;
  struct llNode* old_prev = item->prev_alpha;

  old_next->prev_alpha = old_prev;
  old_prev->next_alpha = old_next;

  struct llNode* current_end = NULL;
  if (force_end == false) {
	current_end = tfwd_as(ll, fn);
  } else {
	current_end = ll->prev_alpha;
  }

  item->next_alpha = current_end->next_alpha;
  item->prev_alpha = current_end;
  current_end->next_alpha = item;

  return 0;
}

/* Relink Node, by Size:
   disconnects the node from its place in the list and links it anew. */
int relink_s(struct llNode* ll, struct llNode* item, uint64_t size, int8_t force_end) {
  struct llNode* old_next = item->next_largest;
  struct llNode* old_prev = item->next_smallest;

  old_next->next_smallest = old_prev;
  old_prev->next_largest = old_next;

  struct llNode* current_end = NULL;
  if (force_end == false) {
	current_end = tfwd_ss(ll, size);
  } else {
	current_end = ll->next_smallest;
  }

  item->next_largest = current_end->next_largest;
  item->next_smallest = current_end;
  current_end->next_largest = item;

  return 0;
}

/*
  Relink Node, by Age, Forced:
  disconnects the node from its place in the list and links it anew.
  Assumes that the node is the newest node.
*/
int relink_rf(struct llNode* ll, struct llNode* item) {
  struct llNode* old_next = item->next_largest;
  struct llNode* old_prev = item->next_smallest;

  old_next->next_youngest = old_prev;
  old_prev->next_oldest = old_next;

  struct llNode* current_end = ll->next_oldest;

  item->next_youngest = current_end->next_youngest;
  item->next_oldest = current_end;
  current_end->next_youngest = item;

  return 0;
}

/* Quickly bumps the timestamp and revision of the file node. */
void bump_meta(struct llNode* llh, struct llNode* ll) {
  ll->timestamp = now();
  ll->frevision = ll->frevision + 1;

  relink_rf(llh, ll);
}

void bump_revision(struct llNode* ll) {
  ll->frevision = ll->frevision + 1;
}

int set_fname(struct llNode* llh, struct llNode* ll, char* fn) {
  if (strlen(fn) > 255) {
	fprintf(stderr, "%sError: filename too long: maximum 255, got %ld.%s", red(), strlen(fn), noc());
	return LL_FILENAME_TOO_LONG_ERROR;
  }

  memcpy(ll->fname, fn, sizeof(char) * strlen(fn));
  bump_meta(llh, ll);

  relink_a(llh, ll, fn, false);

  return 0;
}

void set_size(struct llNode* llh, struct llNode* ll, uint64_t size) {
  ll->size = size;
  bump_meta(ll, llh);

  relink_s(llh, ll, size, false);
}

// create a linked list
struct llNode* new_ll() {
  struct llNode* ll = (struct llNode*)xmalloc(sizeof(struct llNode), "struct llNode* new_ll() @ linked-list.c");
  
  ll->is_head = true;
  ll->timestamp = now();
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

// appends an item to the ll (must be the head node!)
int append(struct llNode* ll, const char* fn, uint64_t fsize, uint32_t revision) {
  if (ll->is_head == false) {
	fprintf(stderr, "%sError: must pass head node of linked list to append()!%s", red(), noc());
	return LL_NOT_HEAD_NODE;
  }
  struct llNode* lla = (struct llNode*)xmalloc(sizeof(struct llNode), "int append() @ linked-list.c");

  lla->is_head = false;
  lla->timestamp = now();
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

	current_end = ll->next_oldest; // This is always going to be the youngest item as far as our resolution can tell

	lla->next_youngest = current_end->next_youngest;
	lla->next_oldest = current_end;
	current_end->next_youngest = lla;
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
