#ifndef LINKED_LIST_H_
#define LINKED_LIST_H_

#include <stdint.h>
#include <time.h>

#define LL_FILENAME_TOO_LONG_ERROR 1
#define LL_NOT_HEAD_NODE 2
#define LL_BAD_INDEX 3
#define LL_WRONG_NODE 4

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

struct llNode* get_n(struct llNode* ll, uint64_t item);

struct llNode* tfwd_n(struct llNode* ll);
struct llNode* tone_rn(struct llNode* ll, int8_t dir);
struct llNode* next_node(struct llNode* ll);
struct llNode* prev_node(struct llNode* ll);
struct llNode* tamt_rn(struct llNode* ll, int64_t start, int32_t offset);

struct llNode* tfwd_as(struct llNode* ll, char* str);
struct llNode* tfwd_ss(struct llNode* ll, uint64_t size);
struct llNode* tfwd_ts(struct llNode* ll, int64_t size);

int relink_a(struct llNode* ll, struct llNode* item, char* fn, int8_t force_end);
int relink_s(struct llNode* ll, struct llNode* item, uint64_t size, int8_t force_end);
int relink_rf(struct llNode* ll, struct llNode* item);

void bump_meta(struct llNode* llh, struct llNode* ll);
void bump_revision(struct llNode* ll);

int set_fname(struct llNode* llh, struct llNode* ll, char* fn);
void set_size(struct llNode* llh, struct llNode* ll, uint64_t size);

struct llNode* new_ll();
int append(struct llNode* ll, const char* fn, uint64_t fsize, uint32_t revision);

int del_p(struct llNode* ptr);
int del(struct llNode* ll, uint64_t idx);
int delete_the_whole_entire_list(struct llNode* ll);

#endif
