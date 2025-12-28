#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

#include "server.h"

using namespace Basic;

int sock = 0;

void test_fiber() {
  LOG_INFO("test_fiber sock=%d", sock);

  // sleep(3);

  // close(sock);
  // sylar::IOManager::GetThis()->cancelAll(sock);

  sock = socket(AF_INET, SOCK_STREAM, 0);
  fcntl(sock, F_SETFL, O_NONBLOCK);

  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port   = htons(80);
  inet_pton(AF_INET, "111.63.65.247", &addr.sin_addr.s_addr);

  if (!connect(sock, (const sockaddr*)&addr, sizeof(addr))) {
  } else if (errno == EINPROGRESS) {
    LOG_INFO("add event errno=%d %s", errno, strerror(errno));
    IOManager::GetThis()->addEvent(sock, IOManager::READ, []() { LOG_INFO("read callback"); });
    IOManager::GetThis()->addEvent(sock, IOManager::WRITE, []() {
      LOG_INFO("write callback");
      IOManager::GetThis()->cancelEvent(sock, IOManager::READ);
      // close(sock);
    });
  } else {
    LOG_INFO("else %d %s", errno, strerror(errno));
  }
}

void test1() {
  std::cout << "EPOLLIN=" << EPOLLIN << " EPOLLOUT=" << EPOLLOUT << std::endl;
  IOManager iom(2, "test", false);
  iom.schedule(&test_fiber);
}

Timer::ptr s_timer;
void       test_timer() {
  IOManager iom(2, "test2", false);
  s_timer = iom.addTimer(
      1000,
      []() {
        static int i = 0;
        LOG_INFO("hello timer i=%d", i);
        if (++i == 3) {
          s_timer->reset(2000, true);
          // s_timer->cancel();
        }
      },
      true);
}

void test_condition_timer() {
  IOManager iom(2, "test_cond", false);
  
  // 创建一个可能被提前销毁的对象
  auto cond_obj = std::make_shared<int>(42);
  
  // 添加条件定时器
  auto timer = iom.addConditionTimer(500, [cond_obj]() {
    LOG_INFO("Condition timer triggered, object value: %d", *cond_obj);
  }, std::weak_ptr<int>(cond_obj), true);
  
  // 模拟工作
  iom.schedule([]() {
    sleep(2);  // 等待2秒
    LOG_INFO("Main work done");
  });
  
  // 1.5秒后销毁条件对象
  iom.addTimer(1500, [cond_obj_ptr = &cond_obj]() {
    LOG_INFO("Destroying condition object");
    *cond_obj_ptr = nullptr;  // 销毁条件对象
  }, false);
  
  iom.stop();
}

int main(int argc, char** argv) {
  Config::LoadFromDir("");
  // test1();
  test_timer();
  // test_condition_timer();
  return 0;
}