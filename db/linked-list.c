#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define true 1
#define false 0

#define LL_FILENAME_TOO_LONG_ERROR -1
#define LL_NOT_HEAD_NODE -2

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
};

// create a linked list
struct llNode* new_ll() {
  struct llNode* ll = (struct llNode*)malloc(sizeof(struct llNode));
  
  ll->is_head = true;
  ll->timestamp = time(NULL);
  ll->frevision = 0;

  char tmp[] = "LIST HEAD (NOT A FILE!)";
  memcpy(ll->fname, tmp, sizeof(char) * strlen(tmp));

  ll->next_node = NULL;
  ll->prev_node = NULL;

  ll->next_alpha = NULL;
  ll->prev_alpha = NULL;

  ll->next_largest = NULL;
  ll->next_smallest = NULL;

  return ll;
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

// appends an item to the ll (must be the head node!)
int append(struct llNode* ll, char* fn, uint64_t fsize, uint32_t revision) {
  if (ll->is_head == false) {
	fprintf(stderr, "must pass head node of linked list to append()!");
	return LL_NOT_HEAD_NODE;
  }
  struct llNode* lla = (struct llNode*)malloc(sizeof(struct llNode));

  lla->is_head = false;
  lla->timestamp = time(NULL);
  lla->frevision = revision;
  lla->size = fsize;

  if (strlen(fn) > 255) {
	fprintf(stderr, "filename max length is 255, got %d", strlen(fn));
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

	lla->next_node = current_end->next_node;
	lls->prev_node = current_end;
	current_end->next_node = lla;
  }
}

// tests
int main() {
  struct llNode t;
  struct llNode *test_node = &t;
  
  char test_string[] = "hello_world.txt";
  memcpy(test_node->fname, test_string, sizeof(char) * strlen(test_string));
  
  printf("file name: %s \n", test_node->fname);

  return 0;
}
