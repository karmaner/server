#include "basic/utils.h"

#include <execinfo.h>
#include <string.h>
#include <sys/syscall.h>

#include "basic/fiber.h"
#include "basic/log.h"

namespace Basic {

int set_thread_name(const char* name) {
  return prctl(PR_SET_NAME, name);
}

const char* get_thread_name() {
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

uint64_t get_fiber_id() {
  return Fiber::GetFiberId();
}

void my_backtrace(std::vector<std::string>& bt, int size, int skip) {
  void** array = (void**)malloc((sizeof(void*) * size));
  size_t s     = ::backtrace(array, size);

  char** strings = backtrace_symbols(array, s);
  if (strings == nullptr) {
    LOG_ERROR("backtrace_synbols error");
    return;
  }

  for (size_t i = skip; i < s; ++i) {
    bt.push_back(strings[i]);
  }

  free(strings);
  free(array);
}

std::string backtrace2string(int size, int skip, const std::string& prefix) {
  std::vector<std::string> bt;
  my_backtrace(bt, size, skip);

  std::stringstream ss;
  for (size_t i = 0; i < bt.size(); ++i) {
    ss << prefix << bt[i] << std::endl;
  }
  return ss.str();
}

}  // namespace Basic