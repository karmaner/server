file(GLOB BASIC_FILES "*.cpp" "*.c")

add_library(basicLIB STATIC ${BASIC_FILES})
