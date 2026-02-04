#include <iostream>

#include "basic/uri.h"

using namespace Basic;

int main(int argc, char** argv) {
  // Uri::ptr uri = Uri::Create("http://www.baidu.com/test/uri?id=100&name=sylar#frg");
  // Uri::ptr uri =
  //     Uri::Create("http://admin@www.baidu.com/test/中文/uri?id=100&name=sylar&vv=中文#frg中文");
  // Uri::ptr uri = Uri::Create("http://admin@www.baidu.com");
  Uri::ptr uri = Uri::Create("https://www.baidu.com/test/uri");
  std::cout << uri->to_string() << std::endl;
  auto addr = uri->createAddress();
  std::cout << *addr << std::endl;
  return 0;
}