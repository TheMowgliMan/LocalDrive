#ifndef LINKED_LIST_H_
#define LINKED_LIST_H_

#include <stdint.h>

#define true 1
#define false 0

#define LL_FILENAME_TOO_LONG_ERROR 1
#define LL_NOT_HEAD_NODE 2
#define LL_bAD_INDEX 3
#define LL_WRONG_NODE 4

struct llNode;

struct llNode* get_n(struct llNode* ll, uint64_t item);

struct llNode* tfwd_n(struct llNode* ll);
struct llNode* tone_rn(struct llNode* ll, int8_t dir);
struct llNode* next_node(struct llNode* ll);
struct llNode* prev_node(struct llNode* ll);
struct llNode* tamt_rn(struct llNode* ll, int64_t start, int32_t offset);

struct llNode* tfwd_as(struct llNode ll, char* str);
struct llNode* tfwd_ss(struct llNode* ll, uint64_t size);
struct llNode* tfwd_ts(struct llNode* ll, int64_t size);

int relink_a(struct llNode* ll, struct llNode* item, char* fn, int8_t force_end);
int relink_s(struct llNode* ll, struct llNode* item, uint64_t size, int8_t force_end);
int relink_rf(struct llNode* ll, struct llNode* item);

#endif
