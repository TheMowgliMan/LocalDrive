#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../util.h"

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

FILE* try_open(struct fileIOCtx* ioctx, const char *opentype) {
  FILE* fctx = fopen(ioctx->fname, opentype);
  
  if (fctx == NULL) {
	if (ioctx->attempts >= FILE_OPEN_ATTEMPTS) {
	  return NULL;
	} else {
	  // We use recursion here to make many attempts at opening the file
	  ioctx->attempts += 1;
	  
	  usleep(250000);
	  
	  fctx = try_open(ioctx, opentype);
	}
  }

  return fctx;
}

struct fileIOCtx* fileIOCtxInit(char* name) {
  struct fileIOCtx* ctx = (struct fileIOCtx*)xmalloc(sizeof(struct fileIOCtx),
													 "struct fileIOCtx* fileIOCtxInit() @ fileio.c");

  memcpy(ctx->fname, name, strlen(name) * sizeof(char));
  ctx->fhandler = NULL;
  ctx->fdata = NULL;
  ctx->attempts = 0;
  ctx->current_read = NULL;

  return ctx;
}

int fileIOCtxOpen(struct fileIOCtx *ctx, const char *opentype) {
  ctx->attempts = 0;

  FILE *f = try_open(ctx, opentype);
  if (f == NULL) {
	return -1;
  }

  ctx->fhandler = f;
  return 0;
}

int fileIOCtxLoad(struct fileIOCtx* ioctx) {
  flockfile(ioctx->fhandler);

  struct sllNode *node = ioctx->fdata;
  size_t read = BUF_LEN;

  while (node != NULL) {
	read = fread_unlocked(node->buf, BUF_LEN, sizeof(char), ioctx->fhandler);

	if (read == BUF_LEN) {
	  node->next = (struct sllNode*)xmalloc(sizeof(struct sllNode),
											"int fileIOCtxLoad() @ fileio.c");
	} else {
	  node->next = NULL;
	}
	
	node = node->next;
  }

  ioctx->current_read = ioctx->fdata;

  return 0;
}

int fileIOCtxRead(struct fileIOCtx* ioctx, char* buf[static BUF_LEN]) {
  if (ioctx->current_read != NULL) {
	memcpy(buf, ioctx->current_read->buf, sizeof(char) * BUF_LEN);

	ioctx->current_read = ioctx->current_read->next;
	if (ioctx->current_read == NULL) {
	  ioctx->current_read = ioctx->fdata;
	}
	
	return 0;
  } else {
	return 0;
  }
}

