
include_directories(
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/3rd/yaml-cpp/src
  ${CMAKE_SOURCE_DIR}/include
)

add_subdirectory(basic)

file(GLOB SERVER_FILE ./server.h)

add_library(server STATIC ${SERVER_FILE})
target_link_libraries(server
  PUBLIC
    basic
)