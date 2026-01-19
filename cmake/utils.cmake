# ragelmaker(src_rl outputlist outputdir)
# ---- 参数说明 ----
# src_rl     : 输入的 .rl 文件路径（相对当前 CMakeLists）
# outputlist : 用来汇总生成结果的变量名，用于 add_library/add_executable
# outputdir  : 输出目录，通常为 ${CMAKE_CURRENT_BINARY_DIR}/generated 或类似路径
function(ragelmaker src_rl outputdir output_var)
  message(STATUS "进入函数")
  message(STATUS "${src_rl}")
  # 提取文件名（不带路径，不带扩展名）
  get_filename_component(src_file ${src_rl} NAME_WE)

  message(STATUS "${src_file}, ${outputdir}")
  # 生成输出文件完整路径，例如 generated/http.rl.cpp
  set(rl_out ${CMAKE_SOURCE_DIR}/src/${outputdir}/${src_file}.rl.cpp)
  message(STATUS "${rl_out}")
  # 定义自定义命令用于生成 Ragel 的 C++ 源码
  add_custom_command(
    OUTPUT ${rl_out}
    COMMAND ragel ${CMAKE_CURRENT_SOURCE_DIR}/${src_rl}
                    -o ${rl_out}
                    -l -C -G2
                    --error-format=msvc
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${src_rl}
    COMMENT "Generating Ragel source ${rl_out}"
    VERBATIM
  )

  # 标记为自动生成文件，使 IDE 不报缺失
  set_source_files_properties(${rl_out} PROPERTIES GENERATED TRUE)
  set(${output_var} ${rl_out} PARENT_SCOPE)
  message(STATUS "返回变量 ${output_var} = ${rl_out}")

endfunction(ragelmaker)
