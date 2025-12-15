

#include "basic/thread.h"

#include <pthread.h>

#include <functional>
#include <stdexcept>

#include "basic/log.h"
#include "basic/utils.h"

namespace Basic {

static thread_local Thread*     t_thread      = nullptr;
static thread_local std::string t_thread_name = "unkonw";

Thread* Thread::GetThis() {
  return t_thread;
}

const std::string& Thread::GetName() {
  return t_thread_name;
}

void Thread::SetName(const std::string& name) {
  if (t_thread) { t_thread->m_name = name; }
  t_thread_name = name;
}

Thread::Thread(std::function<void()> cb, const std::string& name) : m_cb(cb), m_name(name) {
  if (m_name.empty()) { m_name = "unknow"; }

  int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
  if (rt) {
    LOG_ERROR("pthread_create thread fail, rt=%d, name=%s", rt, m_name.c_str());
    throw std::logic_error("pthread_create error");
  }
  m_sem.wait();
}

Thread::~Thread() {
  if (m_thread) { pthread_detach(m_thread); }
}

void Thread::join() {
  if (m_thread) {
    int rt = pthread_join(m_thread, nullptr);
    if (rt) {
      LOG_ERROR("pthread_join thread fail, rt=%d, name=%s", rt, m_name.c_str());
      throw std::logic_error("pthread_join error");
    }
    m_thread = 0;
  }
}

void* Thread::run(void* arg) {
  Thread* thread = static_cast<Thread*>(arg);
  t_thread       = thread;
  t_thread_name  = thread->m_name;
  thread->m_id   = get_thread_id();
  pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

  std::function<void()> cb;
  cb.swap(thread->m_cb);
  thread->m_sem.notify();
  
  cb();
  return 0;
}

}  // namespace Basic