/**
 * 目前设计上存在一些限制，比如大量创建线程以后无法继续在创建
 * 目前只是对于线程做了一个包装，封装了成了 内部执行一个函数
 *
 */

#pragma once

#include <pthread.h>

#include <functional>
#include <memory>
#include <string>

#include "basic/mutex.h"
#include "basic/noncopyable.h"

namespace Basic {

class Thread : public NonCopyable {
public:
  typedef std::shared_ptr<Thread> ptr;
  Thread(std::function<void()> cb, const std::string& name);
  ~Thread();

  pid_t              getId() const { return m_id; }
  const std::string& getName() const { return m_name; }

  void join();

  static Thread*            GetThis();
  static const std::string& GetName();
  static void               SetName(const std::string& name);

private:
  static void* run(void* arg);

private:
  pid_t                 m_id     = -1;
  pthread_t             m_thread = 0;
  std::function<void()> m_cb;
  std::string           m_name;
  Semaphore             m_sem;
};

}  // namespace Basic