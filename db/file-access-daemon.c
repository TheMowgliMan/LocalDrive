#include <stdio.h>
#include <stdlib.h>

#include "../util.h"
#include "linked-list.h"

struct userWrapper {
  struct llNode* user_meta;
  struct llNode* user_files;
};

struct allUserData {
  uint32_t count;
} users_meta;

// Used to link users together
struct llNode* users_meta_head = NULL;

struct userWrapper* generate_user(char* name) {
  struct userWrapper* u = (struct userWrapper*)xmalloc(sizeof(struct userWrapper),
													   "struct userWrapper* generate_user() @ fileio.c");

  append(users_meta_head, name, 0, 0);
  u->user_meta = prev_node(users_meta_head);
  u->user_files = new_ll();

  return u;
}

int initialize() {
  users_meta_head = new_ll();
  
  // We use an array of users because it doesn't change often
  // ...maybe make it a linked list?
  struct userWrapper* users = generate_user("root");
}

// TODO: generate a way to keep track of the user folder, hash the users name as a folder name?
// Or just use a counter?
