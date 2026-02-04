#!/bin/bash

# 构建脚本 - 支持初始化和编译模式
# 使用方法: ./build.sh [-init]
#   -init: 初始化项目（安装依赖、同步 git submodules）
#   无参: 正常编译项目

set -e  # 任何命令失败则退出

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

# 颜色输出函数
print_info() {
  echo -e "\033[36m[INFO]\033[0m $1"
}

print_success() {
  echo -e "\033[32m[SUCCESS]\033[0m $1"
}

print_error() {
  echo -e "\033[31m[ERROR]\033[0m $1"
}

# 初始化项目
init_project() {
  print_info "开始初始化项目..."
  
  # 安装系统依赖
  print_info "安装系统依赖包..."
  sudo apt update
  sudo apt install -y \
      build-essential \
      cmake \
      ninja-build \
      clang \
      clangd \
      llvm \
      clang-format \
      ragel \
      libssl-dev \
      libpthread-stubs0-dev \
      git
    
  print_success "系统依赖安装完成"
  
  # 同步 git submodules
  print_info "同步 git submodules..."
  cd "${SCRIPT_DIR}"
  git submodule update --init --recursive
  
  print_success "项目初始化完成！"
  print_info "现在可以运行 ./build.sh 进行编译"
}

# 构建项目
build_project() {
  print_info "开始编译项目..."
  
  # 创建构建目录
  if [ ! -d "${BUILD_DIR}" ]; then
    print_info "创建构建目录: ${BUILD_DIR}"
    mkdir -p "${BUILD_DIR}"
  fi
  
  cd "${BUILD_DIR}"
  
  # 运行 CMake 配置
  print_info "运行 CMake 配置..."
  cmake -GNinja ..
  
  # 编译项目
  print_info "编译项目..."
  cmake --build .
  
  print_success "项目编译完成！"
  print_info "可执行文件位置: ${SCRIPT_DIR}/bin/"
}

# 主逻辑
main() {
  case "${1:-}" in
    -init)
      init_project
      ;;
    "")
      build_project
      ;;
    -h|--help)
      echo "用法: $0 [选项]"
      echo ""
      echo "选项:"
      echo "  -init          初始化项目（安装依赖、同步 submodules）"
      echo "  （无选项）     正常编译项目"
      echo "  -h, --help     显示此帮助信息"
      ;;
    *)
      print_error "未知参数: $1"
      echo "使用 '$0 --help' 查看帮助信息"
      exit 1
      ;;
  esac
}

main "$@"
