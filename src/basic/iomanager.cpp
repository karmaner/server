#include "basic/iomanager.h"

#include <fcntl.h>
#include <string.h>
#include <sys/epoll.h>

#include "basic/log.h"
#include "basic/macro.h"

namespace Basic {

IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event) {
  switch (event) {
    case IOManager::READ:
      return read;
    case IOManager::WRITE:
      return write;
    default:
      ASSERT2(false, "getContext");
  }
  throw std::invalid_argument("getContext invalid event");
}

void IOManager::FdContext::resetContext(EventContext& ctx) {
  ctx.scheduler = nullptr;
  ctx.fiber.reset();
  ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(IOManager::Event event) {
  LOG_INFO("fd=%d, triggerEvent event=%d, events=%d", fd, event, events);
  ASSERT(events & event);
  events            = (Event)(events & ~event);
  EventContext& ctx = getContext(event);
  if (ctx.cb) {
    ctx.scheduler->schedule(&ctx.cb);
  } else {
    ctx.scheduler->schedule(&ctx.fiber);
  }
  ctx.scheduler = nullptr;
  return;
}

IOManager::IOManager(size_t threads, const std::string& name, bool use_caller)
    : Scheduler(threads, name, use_caller) {
  m_epfd = epoll_create(5000);
  ASSERT(m_epfd > 0);

  int rt = pipe(m_tickleFds);
  ASSERT(!rt);

  epoll_event event;
  memset(&event, 0, sizeof(epoll_event));
  event.events  = EPOLLIN | EPOLLET;
  event.data.fd = m_tickleFds[0];

  rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
  ASSERT(!rt);

  rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
  ASSERT(!rt);

  contextResize(32);

  start();
}

IOManager::~IOManager() {
  stop();
  close(m_epfd);
  close(m_tickleFds[0]);
  close(m_tickleFds[1]);

  for (size_t i = 0; i < m_fdContexts.size(); ++i) {
    if (m_fdContexts[i]) { delete m_fdContexts[i]; }
  }
}

void IOManager::contextResize(size_t size) {
  m_fdContexts.resize(size);

  for (size_t i = 0; i < m_fdContexts.size(); ++i) {
    if (!m_fdContexts[i]) {
      m_fdContexts[i]     = new FdContext;
      m_fdContexts[i]->fd = i;
    }
  }
}

void IOManager::onTimerInsertedAtFront() {
  tickle();
}

bool IOManager::stopping(uint64_t& timeout) {
  timeout = getNextTimer();
  return timeout == ~0ull && m_pendingEventCount == 0 && Scheduler::stopping();
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
  FdContext*         fd_ctx = nullptr;
  LockType::ReadLock lock(m_lock);
  if ((int)m_fdContexts.size() > fd) {
    fd_ctx = m_fdContexts[fd];
    lock.unlock();
  } else {
    lock.unlock();
    LockType::WriteLock lock2(m_lock);
    contextResize(fd * 1.5);
    fd_ctx = m_fdContexts[fd];
  }

  FdContext::LockType::Lock lock2(fd_ctx->lock);
  if (fd_ctx->events & event) {
    LOG_ERROR("addEvent assert fd=%d,event=%d,fd_ctx.event=%d", fd, event, fd_ctx->events);
    ASSERT(!(fd_ctx->events & event));
  }

  int         op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
  epoll_event epevent;
  epevent.events   = EPOLLET | fd_ctx->events | event;
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(m_epfd, op, fd, &epevent);
  if (rt) {
    LOG_ERROR("epoll_ctl(%d, %d, %d, %d):%d(%d) (%s)", m_epfd, op, fd, epevent.events, rt, errno,
              strerror(errno));
    return -1;
  }

  ++m_pendingEventCount;
  fd_ctx->events                     = (Event)(fd_ctx->events | event);
  FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
  ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);

  event_ctx.scheduler = Scheduler::GetThis();
  if (cb) {
    event_ctx.cb.swap(cb);
  } else {
    event_ctx.fiber = Fiber::GetThis();
    ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC, "state=" << event_ctx.fiber->getState());
  }
  return 0;
}

bool IOManager::delEvent(int fd, Event event) {
  LockType::ReadLock lock(m_lock);
  if ((int)m_fdContexts.size() <= fd) { return false; }

  FdContext* fd_ctx = m_fdContexts[fd];
  lock.unlock();

  FdContext::LockType::Lock lock2(fd_ctx->lock);
  if (!(fd_ctx->events & event)) { return false; }

  Event       new_events = (Event)(fd_ctx->events & ~event);
  int         op         = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.events   = EPOLLET | new_events;
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(m_epfd, op, fd, &epevent);
  if (rt) {
    LOG_ERROR("epoll_ctl(%d, %d, %d, %d):%d(%d) (%s)", m_epfd, op, fd, epevent.events, rt, errno,
              strerror(errno));
    return false;
  }

  --m_pendingEventCount;
  fd_ctx->events                     = new_events;
  FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
  fd_ctx->resetContext(event_ctx);
  return true;
}

bool IOManager::cancelEvent(int fd, Event event) {
  LockType::ReadLock lock(m_lock);
  if ((int)m_fdContexts.size() <= fd) { return false; }
  FdContext* fd_ctx = m_fdContexts[fd];
  lock.unlock();

  FdContext::LockType::Lock lock2(fd_ctx->lock);
  if (!(fd_ctx->events & event)) { return false; }

  Event       new_events = (Event)(fd_ctx->events & ~event);
  int         op         = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.events   = EPOLLET | new_events;
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(m_epfd, op, fd, &epevent);
  if (rt) {
    LOG_ERROR("epoll_ctl(%d, %d, %d, %d):%d(%d) (%s)", m_epfd, op, fd, epevent.events, rt, errno,
              strerror(errno));
    return false;
  }

  fd_ctx->triggerEvent(event);
  --m_pendingEventCount;
  return true;
}

bool IOManager::cancelAll(int fd) {
  LockType::ReadLock lock(m_lock);
  if ((int)m_fdContexts.size() <= fd) { return false; }
  FdContext* fd_ctx = m_fdContexts[fd];
  lock.unlock();

  FdContext::LockType::Lock lock2(fd_ctx->lock);
  if (!fd_ctx->events) { return false; }

  int         op = EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.events   = 0;
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(m_epfd, op, fd, &epevent);
  if (rt) {
    LOG_ERROR("epoll_ctl(%d, %d, %d, %d):%d(%d) (%s)", m_epfd, op, fd, epevent.events, rt, errno,
              strerror(errno));
    return false;
  }

  if (fd_ctx->events & READ) {
    fd_ctx->triggerEvent(READ);
    --m_pendingEventCount;
  }
  if (fd_ctx->events & WRITE) {
    fd_ctx->triggerEvent(WRITE);
    --m_pendingEventCount;
  }

  ASSERT(fd_ctx->events == 0);
  return true;
}

IOManager* IOManager::GetThis() {
  return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

void IOManager::tickle() {
  if (!hasIdleThreads()) { return; }
  int rt = write(m_tickleFds[1], "T", 1);
  ASSERT(rt == 1);
}

bool IOManager::stopping() {
  uint64_t timeout = 0;
  return stopping(timeout);
}

void IOManager::idle() {
  LOG_DEBUG("idle");
  const uint64_t               MAX_EVNETS = 256;
  epoll_event*                 events     = new epoll_event[MAX_EVNETS]();
  std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr) { delete[] ptr; });

  while (true) {
    uint64_t next_timeout = 0;
    if (stopping(next_timeout)) {
      LOG_INFO("name=%s idle stopping exit", getName().c_str());
      break;
    }

    int rt = 0;
    do {
      static const int MAX_TIMEOUT = 3000;
      if (next_timeout != ~0ull) {
        next_timeout = (int)next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
      } else {
        next_timeout = MAX_TIMEOUT;
      }
      rt = epoll_wait(m_epfd, events, 64, (int)next_timeout);
      if (rt < 0 && errno == EINTR) {
      } else {
        break;
      }
    } while (true);

    std::vector<std::function<void()> > cbs;
    listExpiredCb(cbs);
    if (!cbs.empty()) {
      LOG_DEBUG("on timer cbs.size=%d", cbs.size());
      schedule(cbs.begin(), cbs.end());
      cbs.clear();
    }

    for (int i = 0; i < rt; ++i) {
      epoll_event& event = events[i];
      if (event.data.fd == m_tickleFds[0]) {
        uint8_t dummy;
        while (read(m_tickleFds[0], &dummy, 1) > 0)
          ;
        continue;
      }

      FdContext*                fd_ctx = (FdContext*)event.data.ptr;
      FdContext::LockType::Lock lock(fd_ctx->lock);
      if (event.events & (EPOLLERR | EPOLLHUP)) { event.events |= EPOLLIN | EPOLLOUT; }
      int real_events = NONE;
      if (event.events & EPOLLIN) { real_events |= READ; }
      if (event.events & EPOLLOUT) { real_events |= WRITE; }

      if ((fd_ctx->events & real_events) == NONE) { continue; }

      int left_events = (fd_ctx->events & ~real_events);
      int op          = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
      event.events    = EPOLLET | left_events;

      int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
      if (rt2) {
        LOG_ERROR("epoll_ctl(%d, %d,%d,%d):%d (%d) (%s)", m_epfd, op, fd_ctx->fd, event.events, rt2,
                  errno, strerror(errno));
        continue;
      }

      if (real_events & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
      }
      if (real_events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
      }
    }

    Fiber::ptr cur     = Fiber::GetThis();
    auto       raw_ptr = cur.get();
    cur.reset();

    raw_ptr->swapOut();
  }
}

}  // namespace Basic