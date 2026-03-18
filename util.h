#ifndef UTIL_H_
#define UTIL_H_

#include <stddef.h>
#include <time.h>

char* red();
char* noc();

time_t now();

void hcf(char* msg, char* from);
void* xmalloc(size_t size, char* user);

#endif
