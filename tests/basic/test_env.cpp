#include <unistd.h>

#include <fstream>
#include <iostream>

#include "basic//env.h"

using namespace Basic;

struct A {
  A() {
    std::ifstream ifs("/proc/" + std::to_string(getpid()) + "/cmdline", std::ios::binary);
    std::string   content;
    content.resize(4096);

    ifs.read(&content[0], content.size());
    content.resize(ifs.gcount());

    for (size_t i = 0; i < content.size(); ++i) {
      std::cout << i << " - " << content[i] << " - " << (int)content[i] << std::endl;
    }
  }
};

A a;

int main(int argc, char* argv[]) {
  std::cout << "argc=" << argc << std::endl;
  EnvMgr::GetInstance()->addHelp("s", "start with the terminal");
  EnvMgr::GetInstance()->addHelp("d", "run as daemon");
  EnvMgr::GetInstance()->addHelp("p", "print help");
  if (!EnvMgr::GetInstance()->init(argc, argv)) {
    EnvMgr::GetInstance()->printHelp();
    return 0;
  }

  std::cout << "exe=" << EnvMgr::GetInstance()->getExe() << std::endl;
  std::cout << "cwd=" << EnvMgr::GetInstance()->getCwd() << std::endl;

  std::cout << "path=" << EnvMgr::GetInstance()->getEnv("PATH", "xxx") << std::endl;
  std::cout << "test=" << EnvMgr::GetInstance()->getEnv("TEST", "") << std::endl;
  std::cout << "set env " << EnvMgr::GetInstance()->setEnv("TEST", "yy") << std::endl;
  std::cout << "test=" << EnvMgr::GetInstance()->getEnv("TEST", "") << std::endl;
  if (EnvMgr::GetInstance()->has("p")) { EnvMgr::GetInstance()->printHelp(); }
  return 0;
}
