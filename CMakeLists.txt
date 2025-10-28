cmake_minimum_required(VERSION 3.20)

project(server)

set(CXX_STANDEN 17)

include_directories(src)

add_subdirectory(src)
add_subdirectory(tests)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)