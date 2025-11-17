#pragma once

#include <string>
#include <typeinfo>

#ifdef __GNUG__
#include <cxxabi.h>
#endif

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