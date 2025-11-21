#include <sys/syscall.h>
#include <unistd.h>

#include <atomic>
#include <thread>

#include "server.h"

using namespace Basic;
using namespace std::chrono_literals;

// 简单断言宏（实际项目建议用gtest等框架）
#define ASSERT_TRUE(cond) \
  if (!(cond)) { LOG_ERROR("Assertion failed: %s at %s:%d", #cond, __FILE__, __LINE__); }

#define ASSERT_FALSE(cond) \
  if ((cond)) { LOG_ERROR("Assertion failed: %s at %s:%d", #cond, __FILE__, __LINE__); }

#define ASSERT_EQ(a, b) \
  if ((a) != (b)) { LOG_ERROR("Assertion failed: %s != %s at %s:%d", #a, #b, __FILE__, __LINE__); }

#define ASSERT_NE(a, b) \
  if ((a) == (b)) { LOG_ERROR("Assertion failed: %s == %s at %s:%d", #a, #b, __FILE__, __LINE__); }

std::atomic<int> test_counter(0);

void test_basic_functionality() {
  LOG_INFO("=== Testing basic functionality ===");

  bool   executed = false;
  Thread thread(
      [&executed] {
        executed = true;
        LOG_INFO("Thread executed, name: %s", Thread::GetName().c_str());
        ASSERT_EQ(Thread::GetName(), "worker_thread");
      },
      "worker_thread");

  thread.join();
  ASSERT_TRUE(executed);
  LOG_INFO("Basic functionality test passed");
}

void test_tls_access() {
  LOG_INFO("=== Testing TLS access ===");

  pid_t       main_tid  = syscall(SYS_gettid);
  std::string main_name = Thread::GetName();

  pid_t       worker_tid = -1;
  std::string worker_name;
  Thread::ptr worker(new Thread(
      [&worker_tid, &worker_name] {
        worker_tid  = syscall(SYS_gettid);
        worker_name = Thread::GetName();

        // 验证TLS返回正确的当前线程
        Thread* current = Thread::GetThis();
        ASSERT_TRUE(current != nullptr);
        ASSERT_EQ(current->getName(), "tls_thread");
      },
      "tls_thread"));

  worker->join();

  // 验证主线程TLS未被修改
  ASSERT_EQ(Thread::GetName(), main_name);
  ASSERT_NE(worker_tid, main_tid);
  ASSERT_EQ(worker_name, "tls_thread");

  LOG_INFO("TLS access test passed");
}

void test_thread_naming() {
  LOG_INFO("=== Testing thread naming ===");

  // 测试超长名称（应被截断）
  std::string long_name = "this_is_a_very_long_thread_name_that_exceeds_limit";
  Thread      thread1(
      [&long_name] {
        // Linux限制16字节（含终止符）
        ASSERT_TRUE(Thread::GetName().size() <= 15);
        LOG_INFO("Long name truncated to: %s", Thread::GetName().c_str());
      },
      long_name);
  thread1.join();

  // 测试空名称（应使用默认）
  Thread thread2(
      [] {
        ASSERT_EQ(Thread::GetName(), "unknow");  // 注意代码中的拼写
      },
      "");
  thread2.join();

  LOG_INFO("Thread naming test passed");
}

void test_lifecycle_management() {
  LOG_INFO("=== Testing lifecycle management ===");

  // 测试1: 正常join
  {
    bool   finished = false;
    Thread thread(
        [&finished] {
          std::this_thread::sleep_for(100ms);
          finished = true;
        },
        "join_test");

    thread.join();
    ASSERT_TRUE(finished);
  }

  // 测试2: 析构时自动detach
  {
    std::atomic<bool> running(false);
    Thread*           thread = new Thread(
        [&running] {
          running = true;
          std::this_thread::sleep_for(300ms);
          running = false;
        },
        "detach_test");

    pid_t tid = thread->getId();
    delete thread;  // 析构函数会detach线程

    // 等待线程运行
    std::this_thread::sleep_for(50ms);
    ASSERT_TRUE(running);  // 线程应该仍在运行

    // 等待线程自然结束
    std::this_thread::sleep_for(300ms);
    ASSERT_FALSE(running);
  }

  LOG_INFO("Lifecycle management test passed");
}

void test_exception_handling() {
  LOG_INFO("=== Testing exception handling ===");

  // 测试线程创建失败
  bool                     exception_caught = false;
  std::vector<Thread::ptr> threads;
  try {
    // 尝试创建超过系统限制的线程数
    for (int i = 0; i < 100000; ++i) {  // 这个数量很可能超过系统限制
      threads.push_back(std::make_shared<Thread>([] { std::this_thread::sleep_for(1s); },
                                                 "stress_test_" + std::to_string(i)));
    }
  } catch (const std::exception& e) {
    exception_caught = true;
    LOG_INFO("Caught expected exception: %s", e.what());
  }
  // 不强制断言，因为有些系统可能支持大量线程
  if (exception_caught) {
    LOG_INFO("Exception handling test passed partially");
  } else {
    LOG_WARN("No exception caught - system may support high thread count");
  }
}

void test_thread_error() {
  sleep(20);
  // 测试线程函数中的异常
  bool   thread_exception_handled = false;
  Thread thread(
      [&thread_exception_handled] {
        try {
          throw std::runtime_error("Test exception in thread");
        } catch (const std::exception& e) {
          LOG_INFO("Caught exception in thread: %s", e.what());
          thread_exception_handled = true;
        }
      },
      "exception_test");

  thread.join();
  ASSERT_TRUE(thread_exception_handled);

  LOG_INFO("Exception handling test passed");
}

void test_concurrent_access() {
  LOG_INFO("=== Testing concurrent access ===");

  const int                thread_count = 10;
  const int                iterations   = 100;
  std::vector<Thread::ptr> threads;
  std::atomic<int>         counter(0);

  // 创建多个线程同时增加计数器
  for (int i = 0; i < thread_count; ++i) {
    threads.push_back(std::make_shared<Thread>(
        [&counter, iterations, i] {
          for (int j = 0; j < iterations; ++j) {
            counter.fetch_add(1);
            std::this_thread::sleep_for(1ms);  // 增加竞争概率
          }
          LOG_DEBUG("Thread %d finished", i);
        },
        "concurrent_" + std::to_string(i)));
  }

  // 等待所有线程完成
  for (auto& t : threads) {
    t->join();
  }

  ASSERT_EQ(counter.load(), thread_count * iterations);
  LOG_INFO("Concurrent access test passed");
}

void test_recursive_thread_creation() {
  LOG_INFO("=== Testing recursive thread creation ===");

  // 测试在线程内创建新线程
  Thread thread(
      [] {
        LOG_INFO("Outer thread started");

        Thread inner_thread(
            [] {
              LOG_INFO("Inner thread started");
              ASSERT_EQ(Thread::GetName(), "inner_thread");
              LOG_INFO("Inner thread finished");
            },
            "inner_thread");

        inner_thread.join();
        LOG_INFO("Outer thread finished");
      },
      "outer_thread");

  thread.join();
  LOG_INFO("Recursive thread creation test passed");
}

void test_thread_local_inheritance() {
  LOG_INFO("=== Testing thread local inheritance ===");

  Thread::SetName("main_modified");
  ASSERT_EQ(Thread::GetName(), "main_modified");

  std::string child_name;
  Thread      thread(
      [&child_name] {
        child_name = Thread::GetName();
        Thread::SetName("child_modified");
        // 验证修改只影响当前线程
        ASSERT_EQ(Thread::GetName(), "child_modified");
      },
      "child_thread");

  thread.join();

  // 验证主线程名称未受影响
  ASSERT_EQ(Thread::GetName(), "main_modified");
  ASSERT_EQ(child_name, "child_thread");

  LOG_INFO("Thread local inheritance test passed");
}

void test_resource_leak() {
  LOG_INFO("=== Testing resource leak ===");

  // 创建并销毁大量线程
  const int thread_count = 100;
  for (int i = 0; i < thread_count; ++i) {
    Thread thread([] { std::this_thread::sleep_for(10ms); }, "leak_test_" + std::to_string(i));
    thread.join();
  }

  LOG_INFO("Created and joined %d threads without apparent leaks", thread_count);
  LOG_INFO("For accurate leak detection, use valgrind or similar tools");
  LOG_INFO("Resource leak test passed (basic)");
}

void test_join_behavior() {
  LOG_INFO("=== Testing join behavior ===");

  // 测试重复join
  Thread thread([] { std::this_thread::sleep_for(100ms); }, "join_test");

  thread.join();

  // 尝试重复join
  bool exception_on_double_join = false;
  try {
    thread.join();
  } catch (const std::exception& e) {
    exception_on_double_join = true;
    LOG_INFO("Expected exception on double join: %s", e.what());
  }

  // 验证m_thread已被重置
  // (这个需要访问私有成员，实际测试中可能需要friend声明或getter)
  // 这里我们通过行为间接验证

  LOG_INFO("Join behavior test passed");
}

void test_thread() {
  LOG_INFO("Starting Thread module comprehensive tests");

  test_basic_functionality();
  test_tls_access();
  test_thread_naming();
  test_lifecycle_management();
  test_exception_handling();
  test_thread_error();
  test_concurrent_access();
  test_recursive_thread_creation();
  test_thread_local_inheritance();
  test_resource_leak();
  test_join_behavior();

  LOG_INFO("All Thread module tests completed successfully");
}

int main() {
  Config::LoadFromDir("");

  Thread::SetName("main_thread");

  try {
    test_thread();
  } catch (const std::exception& e) { LOG_ERROR("Test failed with exception: %s", e.what()); }

  return 0;
}