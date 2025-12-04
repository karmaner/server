#include <exception>
#include <iostream>

#include "basic/scheduler.h"
#include "server.h"

using namespace Basic;

void test_fiber_switch() {
  Scheduler sc(3, false, "test");

  std::cout << "=== 测试1: 基本fiber切换 ===" << std::endl;

  auto fiber1 = Fiber::ptr(new Fiber([]() {
    std::cout << "Fiber 1: 开始执行" << std::endl;
    Fiber::Yield2Hold();  // 切换出去
    std::cout << "Fiber 1: 重新获得控制权" << std::endl;
  }));

  auto fiber2 = Fiber::ptr(new Fiber([]() {
    std::cout << "Fiber 2: 开始执行" << std::endl;
    Fiber::Yield2Hold();  // 切换出去
    std::cout << "Fiber 2: 重新获得控制权" << std::endl;
  }));

  sc.schedule(fiber1, fiber2);

  // 手动切换
  fiber1->swapIn();
  std::cout << "Main: fiber1已切换出去，切换到fiber2" << std::endl;
  fiber2->swapIn();
  std::cout << "Main: fiber2已切换出去，切换回fiber1" << std::endl;
  fiber1->swapIn();
  std::cout << "Main: fiber1执行完毕，切换回fiber2" << std::endl;
  fiber2->swapIn();

  std::cout << "测试1完成" << std::endl;
}

void test_fiber() {
  LOG_INFO("test in fiber");
}

void test_basic() {
  LOG_INFO("main");
  Scheduler sc(3, true, "test");
  sc.schedule(test_fiber);
  sc.start();
  LOG_INFO("schedule");
  sc.stop();
  LOG_INFO("over");
  sleep(5);
}

int main() {
  Config::LoadFromDir("");
  try {
    test_fiber_switch();
    // test_basic();
  } catch (std::exception e) { LOG_ERROR("报错%s", e.what()); }
}