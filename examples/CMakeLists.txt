file(GLOB_RECURSE EXAMPLES_SOURCE "*.cpp" "*.c")

foreach(FILE ${EXAMPLES_SOURCE})
  get_filename_component(EXAMPLE ${FILE} NAME_WE)

  add_executable(${EXAMPLE} ${FILE})
  target_link_libraries(${EXAMPLE} PUBLIC server)
endforeach()
