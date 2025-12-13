#pragma once

#include <cstdint>

#include "basic/mutex.h"
#include "basic/scheduler.h"
#include "basic/timer.h"

namespace Basic {

class IOManager : public Scheduler, public TimerManager {
public:
  typedef std::shared_ptr<IOManager> ptr;
  typedef RWMutex                    LockType;

  enum Event {
    NONE  = 0x0,
    READ  = 0x1,
    WRITE = 0x4,
  };

private:
  struct FdContext {
    typedef Mutex MutexType;
    struct EventContext {
      Scheduler*            scheduler = nullptr;  // 事件执行的scheduler
      Fiber::ptr            fiber;                // 事件协程
      std::function<void()> cb;                   // 事件的回调函数
    };

    EventContext& getContext(Event event);
    void          resetContext(EventContext& ctx);
    void          triggerEvent(Event event);

    EventContext read;           // 读事件
    EventContext write;          // 写事件
    int          fd     = 0;     // 事件关联的句柄
    Event        events = NONE;  // 已经注册的事件
    MutexType    mutex;
  };

public:
  IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "");
  ~IOManager();

  // 0 success, -1 error
  int  addEvent(int fd, Event event, std::function<void()> cb = nullptr);
  bool delEvent(int fd, Event event);
  bool cancelEvent(int fd, Event event);

  bool cancelAll(int fd);

  static IOManager* GetThis();

protected:
  void tickle() override;
  bool stopping() override;
  void idle() override;

  void contextResize(size_t size);

  void onTimerInsertedAtFront() override;
  bool stopping(uint64_t& timeout);

private:
  int m_epfd = 0;
  int m_tickleFds[2];

  std::atomic<size_t>     m_pendingEventCount = {0};
  LockType                m_lock;
  std::vector<FdContext*> m_fdContexts;
};
}  // namespace Basic