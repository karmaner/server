#pragma once

#include <assert.h>

#define ASSERT(x)                                               \
  if (!(x)) {                                                   \
    LOG_ERROR_STREAM << "ASSERTION: " << #x << "\nbacktrace:\n" \
                     << backtrace2string(100, 2, "         ");  \
    assert(x);                                                  \
  }

#define ASSERT2(x, w)                                          \
  if (!(x)) {                                                  \
    LOG_ERROR_STREAM << "ASSERTION: " << #x << "\n"            \
                     << w << "\nbacktrace:\n"                  \
                     << backtrace2string(100, 2, "         "); \
    assert(x);                                                 \
  }