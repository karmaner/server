#include "basic/mutex.h"

#include <semaphore.h>

#include <stdexcept>

namespace Basic {
Semaphore::Semaphore(const uint32_t count) {
  if (sem_init(&m_sem, 0, count)) { throw std::logic_error("sem_init error"); }
}

Semaphore::~Semaphore() {
  sem_destroy(&m_sem);
}

void Semaphore::wait() {
  if (sem_wait(&m_sem)) { throw std::logic_error("sem_wait error"); }
}

void Semaphore::notify() {
  if (sem_post(&m_sem)) { throw std::logic_error("sem_post error"); }
}

}  // namespace Basic