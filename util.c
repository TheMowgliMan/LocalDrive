#include "util.h"

char* red() {
  return "\u001b[31m";
}

char* noc() {
  return "\u001b[0m";
}

void hcf(char* msg, char* from) {
  openlog("LocalDrive Database Index", LOG_PERROR | LOG_PID, LOG_MAKEPRI(LOG_FTP, LOG_CRIT));
  syslog(LOG_MAKEPRI(LOG_FTP, LOG_CRIT), "From %s: %s", from, msg);
  closelog();

  fprintf(stderr, "%sFatal: from %s: %s%s \n", red(), from, msg, noc());

  exit(2); // TODO: back up archive on crash!
}

void* xmalloc(size_t size, char* user) {
  void *pointer = malloc(size);
  if (pointer == 0 || pointer == NULL) {
	printf("From %s:", user);
	hcf("Failed to allocate virtual memory during xmalloc call!");
  }

  return pointer;
}
