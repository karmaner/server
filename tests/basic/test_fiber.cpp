#include <exception>
#include <vector>

#include "basic/fiber.h"
#include "basic/log.h"
#include "server.h"

using namespace Basic;

// 专注于测试call和back方法
void test_call_back() {
  LOG_INFO("=== 开始call/back方法测试 ===");

  // 确保主线程fiber已初始化
  Fiber::GetThis();

  int step = 0;

  // 创建一个协程
  Fiber::ptr test_fiber(new Fiber(
      [&step]() {
        LOG_INFO("协程内部: step 1");
        step = 1;

        // 使用back切换回主线程 (而不是Yield2Hold)
        LOG_INFO("协程内部: 准备back到主线程");
        Fiber::GetThis()->back();

        LOG_INFO("协程内部: step 2 (恢复执行)");
        step = 2;

        // 再次使用back切换回主线程
        LOG_INFO("协程内部: 准备第二次back到主线程");
        Fiber::GetThis()->back();

        LOG_INFO("协程内部: 结束");
      },
      128 * 1024, true));  // 128KB栈大小

  LOG_INFO("主线程: 准备call切换到协程");
  // 使用call切换到协程 (而不是swapIn)
  test_fiber->call();
  LOG_INFO("主线程: 从协程back返回, 当前step=%d", step);

  if (step != 1) {
    LOG_ERROR("测试失败: step应为1, 实际为%d", step);
    return;
  }

  LOG_INFO("主线程: 准备再次call切换到协程");
  // 再次使用call切换到协程
  test_fiber->call();
  LOG_INFO("主线程: 从协程第二次back返回, 当前step=%d", step);

  if (step != 2) {
    LOG_ERROR("测试失败: step应为2, 实际为%d", step);
    return;
  }

  test_fiber->call();

  LOG_INFO("=== call/back方法测试成功 ===");
}

// 测试基础call/back功能
void test_basic_call_back() {
  LOG_INFO("=== 开始基础call/back测试 ===");

  // 确保主线程fiber已初始化
  Fiber::GetThis();

  bool what_good = false;

  // 创建协程
  Fiber::ptr f1(new Fiber(
      [&what_good]() {
        what_good = true;
        LOG_INFO("我在协程内部, what_good已设置为true");

        // 直接使用back方法返回 (而不是依赖自动切换)
        LOG_INFO("协程内部: 使用back返回主线程");

        // 这行不会被执行，因为已经back回主线程了
        LOG_INFO("这行不会被执行");
      },
      128 * 1024, true));

  LOG_INFO("主线程: 使用call切换到协程");
  f1->call();  // 使用call切换到协程
  LOG_INFO("主线程: 从协程back返回, what_good=%d", what_good);

  if (!what_good) {
    LOG_ERROR("测试失败: what_good应为true");
  } else {
    LOG_INFO("测试成功!");
  }
}

void test_mutil_thread_fiber() {
  LOG_INFO("=== 开始多线程协程测试 ===");

  const int                thread_count = 3;  // 创建3个工作线程
  std::vector<Thread::ptr> threads;
  std::atomic<int>         completed_threads(0);

  // 为每个线程创建启动函数
  for (int i = 0; i < thread_count; ++i) {
    auto thread_func = [i, &completed_threads]() {
      Fiber::ptr main_fiber = Fiber::GetThis();
      LOG_INFO("线程 %d (名称=%s, ID=%d): 主协程已初始化, fiber_id=%d", i,
               Thread::GetName().c_str(), Thread::GetThis()->getId(), main_fiber->getId());

      // 每个线程创建2个测试协程
      std::vector<Fiber::ptr> fibers;
      int                     step = 0;

      // 协程1: 两次切换
      fibers.push_back(std::make_shared<Fiber>(
          [&step, i]() {
            LOG_INFO("线程 %d: 协程1开始执行 (fiber_id=%d)", i, Fiber::GetThis()->getId());
            step = 1;
            LOG_INFO("线程 %d: 协程1 - step设置为1, 准备back", i);
            Fiber::GetThis()->back();

            LOG_INFO("线程 %d: 协程1 - 恢复执行, step设置为2", i);
            step = 2;
            LOG_INFO("线程 %d: 协程1 - 准备第二次back", i);
            Fiber::GetThis()->back();

            LOG_INFO("线程 %d: 协程1 - 执行完毕", i);
          },
          128 * 1024, true));

      // 协程2: 单次切换
      fibers.push_back(std::make_shared<Fiber>(
          [&step, i]() {
            LOG_INFO("线程 %d: 协程2开始执行 (fiber_id=%d)", i, Fiber::GetThis()->getId());
            step = 3;
            LOG_INFO("线程 %d: 协程2 - step设置为3, 准备back", i);
            Fiber::GetThis()->back();

            LOG_INFO("线程 %d: 协程2 - 执行完毕", i);
          },
          128 * 1024, true));

      // 线程执行流程
      LOG_INFO("线程 %d: 调用协程1", i);
      fibers[0]->call();
      LOG_INFO("线程 %d: 从协程1返回, 当前step=%d", i, step);

      LOG_INFO("线程 %d: 调用协程2", i);
      fibers[1]->call();
      LOG_INFO("线程 %d: 从协程2返回, 当前step=%d", i, step);

      LOG_INFO("线程 %d: 再次调用协程1", i);
      fibers[0]->call();
      LOG_INFO("线程 %d: 从协程1第二次返回, 当前step=%d", i, step);

      if (fibers[0]->getState() != Fiber::TERM) {
        LOG_INFO("线程 %d: 协程1未完成，强制执行完", i);
        fibers[0]->call();
      }

      if (fibers[1]->getState() != Fiber::TERM) {
        LOG_INFO("线程 %d: 协程2未完成，强制执行完", i);
        fibers[1]->call();
      }

      LOG_INFO("线程 %d: 所有协程执行完毕，准备退出", i);
      completed_threads++;
    };

    threads.push_back(std::make_shared<Thread>(thread_func, "Work_" + std::to_string(i)));
  }

  // // 等待所有线程完成
  // for (auto& thread : threads) {
  //   thread->join();
  // }

  sleep(5);

  LOG_INFO("多线程测试结果: 计划线程数=%d, 实际完成线程数=%d", thread_count,
           (int)completed_threads);

  if (completed_threads == thread_count) {
    LOG_INFO("=== 多线程协程测试成功 ===");
  } else {
    LOG_ERROR("=== 多线程协程测试失败: 仅 %d/%d 线程完成 ===", (int)completed_threads,
              thread_count);
  }
}

void test_fiber() {
  test_basic_call_back();
  test_call_back();
  test_mutil_thread_fiber();
}

int main() {
  Config::LoadFromDir("");
  try {
    test_fiber();
  } catch (std::exception& e) { LOG_ERROR("测试失败: %s", e.what()); }
  return 0;
}