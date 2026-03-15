#ifndef __FILE_IO_H
#define __FILE_IO_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define BUF_LEN 2048
#define FILE_OPEN_ATTEMPTS 250

struct sllNode {
  char *buf[BUF_LEN];
  struct sllNode *next;
};

struct fileIOCtx {
  FILE *fhandler;
  char fname[512];

  struct sllNode *fdata;
  struct sllNode *current_read;

  uint16_t attempts;
};


FILE* try_open(struct fileIOCtx* ioctx, const char *opentype);
int free_ioctx_sll(struct fileIOCtx* ioctx);

int fileIOCtxUnload(struct fileIOCtx ioctx);
struct fileIOCtx* fileIOCtxInit(char* name);

int fileIOCtxOpen(struct fileIOCtx *ctx, const char *opentype);
int fileIOCtxClose(struct fileIOCtx* ioctx);

int fileIOCtxLoad(struct fileIOCtx *ioctx);
int fileIOCtxRead(struct fileIOCtx* ioctx, char* buf[static BUF_LEN]);

int fileIOCtxFree(struct fileIOCtx* ioctx);

int fileIOCtxPackage(struct fileIOCtx* ioctx, char* buf[static BUF_LEN]);
int fileIOCtxFlush(struct fileIOCtx* ioctx);

#endif
