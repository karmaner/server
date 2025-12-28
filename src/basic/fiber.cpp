#include "basic/fiber.h"

#include <stdlib.h>
#include <ucontext.h>

#include <atomic>
#include <cstdlib>
#include <string>

#include "basic/config.h"
#include "basic/log.h"
#include "basic/macro.h"
#include "basic/scheduler.h"

namespace Basic {

static std::atomic<uint64_t> s_fiber_id{0};
static std::atomic<uint64_t> s_fiber_count{0};

static thread_local Fiber*     t_fiber       = nullptr;  // 线程内部正在运行的协程
static thread_local Fiber::ptr t_threadFiber = nullptr;  // 线程内部的调度协程

static ConfigVar<uint32_t>::ptr g_fiber_static_siez =
    Config::Lookup<uint32_t>("fiber.stack_size", 1024 * 1024, "协程栈大小");

Fiber::Fiber() {
  m_state = EXEC;

  SetThis(this);

  if (getcontext(&m_ctx)) { ASSERT2(false, "getcontext"); }

  ++s_fiber_count;

  LOG_DEBUG("Fiber main");
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
    : m_id(++s_fiber_id), m_cb(cb) {
  ++s_fiber_count;
  m_stacksize = stacksize ? stacksize : g_fiber_static_siez->getValue();

  m_stack = malloc(m_stacksize);
  if (getcontext(&m_ctx)) { ASSERT2(false, "getcontext"); }

  m_ctx.uc_link          = nullptr;  // &t_threadFiber->m_ctx;
  m_ctx.uc_stack.ss_sp   = m_stack;
  m_ctx.uc_stack.ss_size = m_stacksize;

  if (!use_caller)
    makecontext(&m_ctx, &Fiber::MainFunc, 0);
  else
    makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
}

Fiber::~Fiber() {
  --s_fiber_count;
  if (m_stack) {
    ASSERT2(m_state == TERM || m_state == INIT || m_state == EXCEPT, "State=" + std::string(Fiber::to_string(this->getState())));
    free(m_stack);
  } else {
    ASSERT(!m_cb);
    ASSERT(m_state == EXEC);

    Fiber* cur = t_fiber;
    if (cur == this) { SetThis(nullptr); }
  }

  LOG_DEBUG("Fiber::~Fiber() id=%ld total=%llu", m_id, s_fiber_count.load());
}

void Fiber::reset(std::function<void()> cb) {
  ASSERT(m_stack);
  ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);
  m_cb = cb;
  if (getcontext(&m_ctx)) { ASSERT2(false, "getcontext"); }

  m_ctx.uc_link          = nullptr;
  m_ctx.uc_stack.ss_sp   = m_stack;
  m_ctx.uc_stack.ss_size = m_stacksize;

  makecontext(&m_ctx, &Fiber::MainFunc, 0);
  m_state = INIT;
}

void Fiber::swapIn() {
  SetThis(this);
  ASSERT(m_state != EXEC);
  m_state = EXEC;
  LOG_DEBUG("swapIn: fiber change run_fiber=%d, swap_fiber=%d", getId(), Scheduler::GetMainFiber()->getId());
  if (swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) { ASSERT2(false, "swapcontext"); }
}

void Fiber::swapOut() {
  SetThis(Scheduler::GetMainFiber());
  LOG_DEBUG("swapOut: fiber change run_fiber=%d, swap_fiber=%d", Scheduler::GetMainFiber()->getId(), getId());
  if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) { ASSERT2(false, "swapcontext"); }
}

void Fiber::call() {
  SetThis(this);
  m_state = EXEC;
  LOG_DEBUG("call: fiber change run_fiber=%d, swap_fiber=%d", getId(), t_threadFiber->getId());
  if (swapcontext(&t_threadFiber->m_ctx, &m_ctx)) { ASSERT2(false, "swapcontext"); }
}

void Fiber::back() {
  SetThis(t_threadFiber.get());
  LOG_DEBUG("back: fiber change run_fiber=%d, swap_fiber=%d", t_threadFiber->getId(), getId());
  if (swapcontext(&m_ctx, &t_threadFiber->m_ctx)) { ASSERT2(false, "swapcontext"); }
}

void Fiber::SetThis(Fiber* f) {
  t_fiber = f;
}

Fiber::ptr Fiber::GetThis() {
  if (t_fiber) { return t_fiber->shared_from_this(); }
  Fiber::ptr main_fiber(new Fiber);
  ASSERT(t_fiber == main_fiber.get());
  t_threadFiber = main_fiber;
  return t_fiber->shared_from_this();
}

void Fiber::Yield2Ready() {
  Fiber::ptr cur = GetThis();
  ASSERT(cur->m_state == EXEC);
  cur->m_state = READY;
  cur->swapOut();
}

void Fiber::Yield2Hold() {
  Fiber::ptr cur = GetThis();
  ASSERT(cur->m_state == EXEC);
  cur->m_state = HOLD;
  cur->swapOut();
}

uint64_t Fiber::TotalFibers() {
  return s_fiber_count;
}

uint64_t Fiber::GetFiberId() {
  if (t_fiber) { return t_fiber->getId(); }
  return 0;
}

void Fiber::MainFunc() {
  Fiber::ptr cur = GetThis();
  ASSERT(cur);

  try {
    cur->m_cb();
    cur->m_cb    = nullptr;
    cur->m_state = TERM;
  } catch (std::exception& ex) {
    cur->m_state = EXCEPT;
    LOG_ERROR("Fiber Except: %s, fiber_id=%d\n%s", ex.what(), cur->getId(),
              backtrace2string().c_str());
  } catch (...) {
    cur->m_state = EXCEPT;
    LOG_ERROR("Fiber Except: %s, fiber_id=%d\n", cur->getId(), backtrace2string().c_str());
  }

  auto raw_ptr = cur.get();
  cur.reset();
  raw_ptr->swapOut();

  ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()) +
                     ", fiber_state=" + to_string(raw_ptr->getState()));
}

void Fiber::CallerMainFunc() {
  Fiber::ptr cur = GetThis();
  ASSERT(cur);

  try {
    cur->m_cb();
    cur->m_cb    = nullptr;
    cur->m_state = TERM;
  } catch (std::exception& ex) {
    cur->m_state = EXCEPT;
    LOG_ERROR("Fiber Except: %s, fiber_id=%d\n%s", ex.what(), cur->getId(),
              backtrace2string().c_str());
  } catch (...) {
    cur->m_state = EXCEPT;
    LOG_ERROR("Fiber Except: %s, fiber_id=%d\n", cur->getId(), backtrace2string().c_str());
  }

  auto raw_ptr = cur.get();
  cur.reset();
  raw_ptr->back();

  ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()) +
                     ", fiber_state=" + to_string(raw_ptr->getState()));
}

}  // namespace Basic