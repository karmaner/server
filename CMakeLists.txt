cmake_minimum_required(VERSION 3.10)
project(server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON) 

include(cmake/utils.cmake)

# 设置默认构建类型为 Debug（如果未指定）
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# 调试编译选项（使用 _DEBUG 避免与代码中的 DEBUG 枚举冲突）
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -D_DEBUG")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -D_DEBUG")

# Clang 专用选项（可选）
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-fstandalone-debug)
  message(STATUS "Clang detected: -fstandalone-debug enabled")
endif()


set(BIN_DIR ${CMAKE_SOURCE_DIR}/bin)
set(LIB_DIR ${CMAKE_SOURCE_DIR}/lib)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${LIB_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${LIB_DIR})

add_subdirectory(3rd/yaml-cpp)

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(examples)
