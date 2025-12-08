#include "basic/scheduler.h"

#include "basic/log.h"
#include "basic/macro.h"
#include "basic/utils.h"

namespace Basic {

static thread_local Scheduler* t_scheduler       = nullptr;
static thread_local Fiber*     t_scheduler_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name) : m_name(name) {
  ASSERT(threads > 0);

  if (use_caller) {
    Fiber::GetThis();
    --threads;

    ASSERT(GetThis() == nullptr);
    t_scheduler = this;

    m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this)));
    Thread::SetName(m_name);

    t_scheduler_fiber = m_rootFiber.get();
    m_rootThread      = get_thread_id();
    m_threadIds.push_back(m_rootThread);
  } else {
    m_rootThread = -1;
  }
  m_threadCount = threads;
}

Scheduler::~Scheduler() {
  ASSERT(m_stopping);
  if (GetThis() == this) { t_scheduler = nullptr; }
}

Scheduler* Scheduler::GetThis() {
  return t_scheduler;
}

Fiber* Scheduler::GetMainFiber() {
  return t_scheduler_fiber;
}

void Scheduler::start() {
  LockType::Lock lock(m_lock);
  if (!m_stopping) { return; }
  m_stopping = false;
  ASSERT(m_threads.empty());

  m_threads.resize(m_threadCount);
  for (size_t i = 0; i < m_threadCount; ++i) {
    m_threads[i].reset(
        new Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));
    m_threadIds.push_back(m_threads[i]->getId());
  }
  lock.unlock();

  // if (m_rootFiber) {
  //   m_rootFiber->swapIn();
  //   // m_rootFiber->call();
  //   LOG_INFO("call out %s", Fiber::to_string(m_rootFiber->getState()));
  // }
}

void Scheduler::stop() {
  m_autoStop = true;
  if (m_rootFiber && m_threadCount == 0 &&
      (m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::INIT)) {
    LOG_INFO("%s stopped", this->m_name.c_str());
    m_stopping = true;

    if (stopping()) { return; }
  }

  // bool exit_on_this_fiber = false;
  if (m_rootThread != -1) {
    ASSERT(GetThis() == this);
  } else {
    ASSERT(GetThis() != this);
  }

  m_stopping = true;
  for (size_t i = 0; i < m_threadCount; ++i) {
    tickle();
  }

  if (m_rootFiber) { tickle(); }

  if (stopping()) { return; }

  // if(exit_on_this_fiber) {
  // }
}

void Scheduler::setThis() {
  t_scheduler = this;
}

void Scheduler::run() {
  LOG_DEBUG("scheduler:%s run", m_name.c_str());
  setThis();
  if (get_thread_id() != m_rootThread) { t_scheduler_fiber = Fiber::GetThis().get(); }

  Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
  Fiber::ptr cb_fiber;

  FiberAndThread ft;
  while (true) {
    ft.reset();
    bool tickle_me = false;
    bool is_active = false;
    {
      LockType::Lock lock(m_lock);
      auto           it = m_fibers.begin();
      while (it != m_fibers.end()) {
        if (it->thread != -1 && it->thread != get_thread_id()) {
          ++it;
          tickle_me = true;
          continue;
        }

        ASSERT(it->fiber || it->cb);
        if (it->fiber && it->fiber->getState() == Fiber::EXEC) {
          ++it;
          continue;
        }

        ft = *it;
        m_fibers.erase(it++);
        ++m_activeThreadCount;
        is_active = true;
        break;
      }
      tickle_me |= it != m_fibers.end();
    }

    if (tickle_me) { tickle(); }

    if (ft.fiber &&
        (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT)) {
      ft.fiber->swapIn();
      --m_activeThreadCount;

      if (ft.fiber->getState() == Fiber::READY) {
        schedule(ft.fiber);
      } else if (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT) {
        ft.fiber->m_state = Fiber::HOLD;
      }
      ft.reset();
    } else if (ft.cb) {
      if (cb_fiber) {
        cb_fiber->reset(ft.cb);
      } else {
        cb_fiber.reset(new Fiber(ft.cb));
      }
      ft.reset();
      cb_fiber->swapIn();
      --m_activeThreadCount;
      if (cb_fiber->getState() == Fiber::READY) {
        schedule(cb_fiber);
        cb_fiber.reset();
      } else if (cb_fiber->getState() == Fiber::EXCEPT || cb_fiber->getState() == Fiber::TERM) {
        cb_fiber->reset(nullptr);
      } else {  // if(cb_fiber->getState() != Fiber::TERM) {
        cb_fiber->m_state = Fiber::HOLD;
        cb_fiber.reset();
      }
    } else {
      if (is_active) {
        --m_activeThreadCount;
        continue;
      }
      if (idle_fiber->getState() == Fiber::TERM) {
        LOG_INFO("idle fiber term");
        break;
      }

      ++m_idleThreadCount;
      idle_fiber->swapIn();
      --m_idleThreadCount;
      if (idle_fiber->getState() != Fiber::TERM && idle_fiber->getState() != Fiber::EXCEPT) {
        idle_fiber->m_state = Fiber::HOLD;
      }
    }
  }
}

void Scheduler::tickle() {
  LOG_INFO("tickle");
}

bool Scheduler::stopping() {
  LockType::Lock lock(m_lock);
  return m_autoStop && m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
}

void Scheduler::idle() {
  LOG_INFO("idle");
}

}  // namespace Basic