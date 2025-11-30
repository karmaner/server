#pragma once

#include <memory>
#include <vector>

#include "basic/fiber.h"
#include "basic/mutex.h"
#include "basic/thread.h"

namespace Basic {

class Scheduler {
public:
  typedef std::shared_ptr<Scheduler> ptr;
  typedef Mutex                      LockType;

  Scheduler(size_t thread_count = 1, bool use_caller = true, const std::string& name = "");
  virtual ~Scheduler();

  const std::string& getName() const { return m_name; }

public:
  static Scheduler* GetThis();
  void              setThis();
  static Fiber*     GetMainFiber();

  void start();
  void stop();

public:
  template <typename FiberOrCb>
  void schedule(FiberOrCb fc, int thread = -1) {
    bool need_tickle = false;
    {
      LockType::Lock lock(m_lock);
      need_tickle = scheduleNoLock(fc, thread);
    }

    if (need_tickle) { tickle(); }
  }

  template <typename InputIterator>
  void schedule(InputIterator begin, InputIterator end) {
    bool need_tickle = false;
    {
      LockType::Lock lock(m_lock);
      while (begin != end) {
        need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
        ++begin;
      }
    }
    if (need_tickle) { tickle(); }
  }

private:
  template <typename FiberOrCb>
  bool scheduleNoLock(FiberOrCb fc, int thread) {
    bool           need_tickle = m_fibers.empty();
    FiberAndThread ft(fc, thread);
    if (ft.fiber || ft.cb) { m_fibers.push_back(ft); }
    return need_tickle;
  }

  struct FiberAndThread {
    Fiber::ptr            fiber;
    std::function<void()> cb;
    int                   thread;

    FiberAndThread(Fiber::ptr f, int thr) : fiber(f), thread(thr) {}

    FiberAndThread(Fiber::ptr* f, int thr) : thread(thr) { fiber.swap(*f); }

    FiberAndThread(std::function<void()> f, int thr) : cb(f), thread(thr) {}

    FiberAndThread(std::function<void()>* f, int thr) : thread(thr) { cb.swap(*f); }

    FiberAndThread() : thread(-1) {}

    void reset() {
      fiber  = nullptr;
      cb     = nullptr;
      thread = -1;
    }
  };

protected:
  virtual void tickle();
  void         run();
  virtual bool stopping();
  virtual void idle();

protected:
  std::vector<int>    m_threadIds;
  size_t              m_threadCount = 0;
  std::atomic<size_t> m_activeThreadCount{0};
  std::atomic<size_t> m_idleThreadCount{0};

  bool m_stopping   = true;
  bool m_autoStop   = false;
  int  m_rootThread = 0;

private:
  LockType    m_lock;
  std::string m_name;

  Fiber::ptr                  m_rootFiber;
  std::vector<Thread::ptr>    m_threads;
  std::vector<FiberAndThread> m_fibers;
};

}  // namespace Basic