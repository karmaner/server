#include "basic/daemon.h"

#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "basic/config.h"
#include "basic/log.h"

namespace Basic {
static ConfigVar<uint32_t>::ptr g_daemon_restart_interval =
    Config::Lookup("daemon.restart_interval", (uint32_t)5, "daemon restart interval");

std::string ProcessInfo::to_string() const {
  std::stringstream ss;
  ss << "parent_id=" << parent_id << " main_id=" << main_id
     << " parent_start_time=" << parent_start_time << " main_start_time=" << main_start_time
     << " restart_count=" << restart_count;
  return ss.str();
}

static int real_start(int argc, char** argv, std::function<int(int argc, char* argv[])> main_cb) {
  ProcessInfoMgr::GetInstance()->main_id         = getpid();
  ProcessInfoMgr::GetInstance()->main_start_time = time(0);
  return main_cb(argc, argv);
}

static int real_daemon(int argc, char** argv, std::function<int(int argc, char** argv)> main_cb) {
  daemon(1, 0);
  ProcessInfoMgr::GetInstance()->parent_id         = getpid();
  ProcessInfoMgr::GetInstance()->parent_start_time = time(0);

  while (true) {
    pid_t pid = fork();
    if (pid == 0) {  // 子进程
      ProcessInfoMgr::GetInstance()->main_id         = getpid();
      ProcessInfoMgr::GetInstance()->main_start_time = time(0);

      LOG_INFO_STREAM << "process start pid=" << getpid();
      return real_start(argc, argv, main_cb);
    } else if (pid < 0) {  // fork错误
      LOG_ERROR_STREAM << "fork fail return=" << pid << "errno=" << errno
                       << " err=" << strerror(errno);
      return -1;
    } else {  // 父进程
      int state = 0;
      waitpid(pid, &state, 0);
      if (state) {
        LOG_ERROR_STREAM << "child crash pid=" << pid << " state=" << state;
      } else {
        LOG_INFO_STREAM << "child finished pid=" << pid;
        break;
      }
      ProcessInfoMgr::GetInstance()->restart_count += 1;
      sleep(g_daemon_restart_interval->getValue());
    }
  }
  return 0;
};

int start_daemon(int argc, char** argv, std::function<int(int argc, char* argv[])> main_cb,
                 bool is_daemon) {
  if (!is_daemon) {
    ProcessInfoMgr::GetInstance()->parent_id         = getpid();
    ProcessInfoMgr::GetInstance()->parent_start_time = time(0);
    return real_start(argc, argv, main_cb);
  }
  return real_daemon(argc, argv, main_cb);
}

}  // namespace Basic