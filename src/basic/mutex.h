#pragma once

#include <pthread.h>
#include <semaphore.h>

#include <atomic>

namespace Basic {

class Semaphore {
public:
  Semaphore(const uint32_t count = 0);
  ~Semaphore();

  void wait();

  void notify();

private:
  sem_t m_sem;
};

template <typename T>
struct ScopedLock {
public:
  ScopedLock(T& mutex) : m_mutex(mutex) {
    m_mutex.lock();
    m_locked = true;
  }

  ~ScopedLock() { unlock(); }

  void lock() {
    if (!m_locked) {
      m_mutex.lock();
      m_locked = true;
    }
  }

  void unlock() {
    if (m_locked) {
      m_mutex.unlock();
      m_locked = false;
    }
  }

private:
  T&   m_mutex;
  bool m_locked;
};

template <typename T>
struct ReadScopedLock {
public:
public:
  ReadScopedLock(T& mutex) : m_mutex(mutex) {
    m_mutex.rdlock();
    m_locked = true;
  }

  ~ReadScopedLock() { unlock(); }

  void lock() {
    if (!m_locked) {
      m_mutex.rdlock();
      m_locked = true;
    }
  }

  void unlock() {
    if (m_locked) {
      m_mutex.unlock();
      m_locked = false;
    }
  }

private:
  T& m_mutex;

  bool m_locked;
};

template <typename T>
struct WriteScopedLock {
public:
  WriteScopedLock(T& mutex) : m_mutex(mutex) {
    m_mutex.wrlock();
    m_locked = true;
  }

  ~WriteScopedLock() { unlock(); }

  void lock() {
    if (!m_locked) {
      m_mutex.wrlock();
      m_locked = true;
    }
  }

  void unlock() {
    if (m_locked) {
      m_mutex.unlock();
      m_locked = false;
    }
  }

private:
  T& m_mutex;

  bool m_locked;
};

class Mutex {
public:
  typedef ScopedLock<Mutex> Lock;

  Mutex() { m_lock = PTHREAD_MUTEX_INITIALIZER; }

  ~Mutex() { pthread_mutex_destroy(&m_lock); }

  void lock() { pthread_mutex_lock(&m_lock); }

  void unlock() { pthread_mutex_unlock(&m_lock); }

private:
  pthread_mutex_t m_lock;
};

class NullMutex {
  typedef ScopedLock<NullMutex> Lock;

  NullMutex() {}

  ~NullMutex() {}

  void lock() {}
  void unlock() {}
};

/**
 * @brief 读写互斥量
 */
class RWMutex {
public:
  /// 局部读锁
  typedef WriteScopedLock<RWMutex> ReadLock;

  /// 局部写锁
  typedef WriteScopedLock<RWMutex> WriteLock;

  RWMutex() { m_lock = PTHREAD_RWLOCK_INITIALIZER; }
  ~RWMutex() { pthread_rwlock_destroy(&m_lock); }

  void rdlock() { pthread_rwlock_rdlock(&m_lock); }
  void wrlock() { pthread_rwlock_wrlock(&m_lock); }
  void unlock() { pthread_rwlock_unlock(&m_lock); }

private:
  pthread_rwlock_t m_lock;
};

class SpinLock {
public:
  /// 局部锁
  typedef ScopedLock<SpinLock> Lock;

  SpinLock() { pthread_spin_init(&m_lock, 0); }

  ~SpinLock() { pthread_spin_destroy(&m_lock); }

  void lock() { pthread_spin_lock(&m_lock); }

  void unlock() { pthread_spin_unlock(&m_lock); }

private:
  pthread_spinlock_t m_lock;
};

class CASLock {
public:
  typedef ScopedLock<CASLock> Lock;

  CASLock() { m_lock.clear(); }

  ~CASLock() {}

  void lock() {
    while (std::atomic_flag_test_and_set_explicit(&m_lock, std::memory_order_acquire))
      ;
  }

  void unlock() { std::atomic_flag_clear_explicit(&m_lock, std::memory_order_release); }

private:
  volatile std::atomic_flag m_lock;
};

}  // namespace Basic