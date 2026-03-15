#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "../util.h"
#include "linked-list.h"
#include "fileio.h"

struct userWrapper {
  struct llNode* user_meta;
  struct llNode* user_files;
};

struct allUserData {
  uint32_t count;
} users_meta;

// Used to link users together
struct llNode* users_meta_head = NULL;

struct userWrapper* users[1];

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
  users[0] = generate_user("root");
  users_meta.count = 1;

  return 0;
}

// TODO: generate a way to keep track of the user folder, hash the users name as a folder name?
// Or just use a counter?
uint64_t hash_char(char character, uint32_t i) {
  srand(character);
  
  uint64_t c = (uint64_t)character;
  uint64_t not = ~((c * 17) << 0x03);
  uint64_t and = (c << 2 * rand()) & (c << 5 * 217 % 51);
  uint64_t or = c << 19 | c << 17;

  return ((c * i) % 2963 + (c ^ not ^ and ^ or)) ^ (i * 4977);
}

char* get_user_folder_name(struct userWrapper* user);

int main(){
  printf("H @ 1: %lu\n", hash_char('H', 1));
  printf("1 @ 0: %lu\n", hash_char('1', 0));
  printf("_ @ 5: %lu\n", hash_char('_', 5));
  printf("t @ 23: %lu\n", hash_char('t', 23));
}
