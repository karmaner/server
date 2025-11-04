file(GLOB BASIC_FILES "*.cpp" "*.c")

add_library(basic STATIC ${BASIC_FILES})
