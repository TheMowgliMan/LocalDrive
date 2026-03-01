/* This file describes a linked list datastructure used for holding references to filenames in the database for
   fast searching and sorting. Nodes are linked alphabetically, by age, and by filesize, in a dual and circular fashion. */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct Node {
  char filename[256] = "";

  struct Node *next_alpha;
  struct Node *prev_alpha;

  struct Node *next_oldest;
  struct Node *next_youngest;

  struct Node *next_largest;
  struct Node *next_smallest;
};
  
