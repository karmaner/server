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
      // close(sock);
      IOManager::GetThis()->cancelEvent(sock, IOManager::READ);
      close(sock);
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
  IOManager iom(2, "test2");
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

int main(int argc, char** argv) {
  test1();
  test_timer();
  return 0;
}