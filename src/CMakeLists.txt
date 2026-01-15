
include_directories(
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/3rd/yaml-cpp/src
  ${CMAKE_SOURCE_DIR}/3rd/yaml-cpp/include/
)


file(GLOB_RECURSE ALL_SOURCE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/basic/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/basic/*.c
  ${CMAKE_CURRENT_SOURCE_DIR}/basic/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/http/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/http/*.c
  ${CMAKE_CURRENT_SOURCE_DIR}/http/*.h
)

add_library(server STATIC ${ALL_SOURCE_FILES})

target_link_libraries(server
  PUBLIC
    yaml-cpp
    OpenSSL::SSL
    OpenSSL::Crypto
    pthread
)

target_include_directories(server
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)