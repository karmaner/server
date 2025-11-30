#include "server.h"

using namespace Basic;

void test_fiber() {
  LOG_INFO("test in fiber");
}

int main() {
  LOG_INFO("main");
  Scheduler sc(3, false, "test");
  sc.start();
  LOG_INFO("schedule");
  sc.stop();
  LOG_INFO("over");
  return 0;
}