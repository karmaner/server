#pragma once

#include <pthread.h>
#include <sys/prctl.h>
#include <unistd.h>

#include <string>
#include <typeinfo>
#include <vector>

#ifdef __GNUG__
#include <cxxabi.h>
#endif

namespace Basic {

template <typename T>
std::string type_name() {
#ifdef __GNUG__
  int         status    = 0;
  char*       demangled = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, &status);
  std::string name      = (status == 0 && demangled) ? demangled : typeid(T).name();
  std::free(demangled);
  return name;
#elif defined(_MSC_VER)
  return typeid(T).name();
#else
  return typeid(T).name();
#endif
}

template <typename T>
std::string type_name(const T& value) {
  return type_name<T>();
}

/**
 * @brief 设置当前线程的名称
 *
 * @param name 要设置的线程名称（最多15个字符，第16个字符会被自动设为'\0'）
 * @return int 成功返回0，失败返回非0
 */
int set_thread_name(const char* name);

/**
 * @brief 获取当前线程的名称
 *
 * @return const char* 线程名称的指针（静态缓冲区，不需要释放）
 */
const char* get_thread_name();

/**
 * @brief 获取当前线程的ID
 *
 * @return int 当前线程的ID
 */
pid_t get_thread_id();

/**
 * @brief 获取当前的调用栈
 */
void my_backtrace(std::vector<std::string>& bt, int size, int skip = 1);

/**
 * @brief 获取当前调用栈的字符串
 * return 调用字符串
 */
std::string backtrace2string(int size = 64, int skip = 2, const std::string& prefix = "");

}  // namespace Basic