#include "basic/daemon.h"
#include "basic/iomanager.h"
#include "basic/log.h"

using namespace Basic;

Timer::ptr timer;

int server_main(int argc, char* argv[]) {
  LOG_INFO_STREAM << ProcessInfoMgr::GetInstance()->to_string();
  IOManager iom(1);
  timer = iom.addTimer(
      1000,
      []() {
        LOG_INFO_STREAM << "onTimer";
        static int count = 0;
        if (++count > 10) {
          LOG_INFO_STREAM << ProcessInfoMgr::GetInstance()->to_string();
          exit(1);
        }
      },
      true);
  return 0;
}

int main(int argc, char* argv[]) {
  return start_daemon(argc, argv, server_main, argc != 1);
}