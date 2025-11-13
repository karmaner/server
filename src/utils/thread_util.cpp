#include "thread_util.h"

#include <string.h>
#include <sys/syscall.h>

int set_thread_name(const char* name) {
  return prctl(PR_SET_NAME, name);
}

const char *get_thread_name() {
  static char thread_name[16] = {0};

  if (prctl(PR_GET_NAME, thread_name) != 0) {
    strncpy(thread_name, "unknown", sizeof(thread_name));
  }
  
  thread_name[sizeof(thread_name) - 1] = '\0';
  return thread_name;
}

int get_thread_id() {
  return (int)syscall(SYS_gettid);
}