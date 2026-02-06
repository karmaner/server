#pragma once

#include <unistd.h>

#include <functional>

#include "basic/singleton.h"

namespace Basic {

struct ProcessInfo {
  pid_t    parent_id         = 0;  // 父进程id
  pid_t    main_id           = 0;  // 主进程id
  uint64_t parent_start_time = 0;  // 父进程启动时间
  uint64_t main_start_time   = 0;  // 主进程启动时间
  uint32_t restart_count     = 0;  // 主重启次数

  std::string to_string() const;
};

typedef Singleton<ProcessInfo> ProcessInfoMgr;

int start_daemon(int argc, char* argv[], std::function<int(int argc, char* argv[])> main_func,
                 bool is_daemon);

};  // namespace Basic