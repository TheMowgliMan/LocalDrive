#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// A struct for a single node in the linked list.
struct llNode;

struct llNode {
  char fname[256];
  uint64_t size; // in bytes
  struct timespec* timestamp;

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

// tests
int main() {
  struct llNode t;
  struct llNode *test_node = &t;
  
  char test_string[] = "hello_world.txt";
  memmove(test_node->fname, test_string, sizeof(char) * strlen(test_string));
  
  printf("file name: %s \n", test_node->fname);

  return 0;
}
