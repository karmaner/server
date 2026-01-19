#pragma once

#include <sys/types.h>

using element_cb = void (*)(void* data, const char* at, size_t length);

using field_cb = void (*)(void* data, const char* field, size_t flen, const char* value,
                          size_t vlen);