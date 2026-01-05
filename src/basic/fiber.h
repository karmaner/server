#pragma once

#include <ucontext.h>

#include <functional>
#include <memory>
#include <unordered_map>

namespace Basic {

class Fiber : public std::enable_shared_from_this<Fiber> {
  friend class Scheduler;

public:
  typedef std::shared_ptr<Fiber> ptr;

  enum State { INIT, HOLD, EXEC, TERM, READY, EXCEPT };

  static const char* to_string(State state) {
    static const std::unordered_map<State, const char*> stateNames = {
        {INIT, "INIT"}, {HOLD, "HOLD"},   {EXEC, "EXEC"},
        {TERM, "TERM"}, {READY, "READY"}, {EXCEPT, "EXCEPT"}};
    static const char* unknown = "UNKNOWN";
    auto               it      = stateNames.find(state);
    return (it != stateNames.end()) ? it->second : unknown;
  }

  Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
  ~Fiber();

  void reset(std::function<void()> cb);

  void swapIn();
  void swapOut();

  void call();
  void back();

  uint64_t getId() const { return m_id; }
  State    getState() const { return m_state; }

public:
  static void       SetThis(Fiber* f);
  static Fiber::ptr GetThis();

  static void     Yield2Ready();
  static void     Yield2Hold();
  static uint64_t TotalFibers();
  static uint64_t GetFiberId();

  static void MainFunc();
  static void CallerMainFunc();

private:
  Fiber();

private:
  uint64_t m_id = 0;

  uint32_t m_stacksize = 0;
  State    m_state     = INIT;

  ucontext_t m_ctx;
  void*      m_stack = nullptr;

  std::function<void()> m_cb;
};

}  // namespace Basic