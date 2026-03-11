#ifndef UTIL_H_
#define UTIL_H_

#include <stddef.h>

char* red();
char* noc();

void hcf(char* msg, char* from);
void* xmalloc(size_t size, char* user);

#endif
