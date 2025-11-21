
include_directories(
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/3rd/yaml-cpp/src
  ${CMAKE_SOURCE_DIR}/include
)

file(GLOB_RECURSE TEST_FILES "*.cpp" "*.c")

foreach(TEST_SOURCE ${TEST_FILES})

  get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
  add_executable(${TEST_NAME} ${TEST_SOURCE})

  target_link_libraries(${TEST_NAME} PUBLIC
    server
  )
endforeach()