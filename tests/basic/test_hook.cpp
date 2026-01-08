#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "basic/hook.h"
#include "basic/iomanager.h"
#include "basic/log.h"

using namespace Basic;

void test_sleep() {
  IOManager iom(1);
  iom.schedule([]() {
    sleep(2);
    LOG_INFO("sleep 2");
  });

  iom.schedule([]() {
    sleep(3);
    LOG_INFO("sleep 3");
  });
  LOG_INFO("test_sleep");
}

void test_sock() {
  int sock = socket(AF_INET, SOCK_STREAM, 0);

  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port   = htons(80);
  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);

  LOG_INFO("begin connect");
  int rt = connect(sock, (const sockaddr*)&addr, sizeof(addr));
  LOG_INFO("connect rt=%d errno=", rt, errno);

  if (rt) { return; }

  const char data[] = "GET / HTTP/1.0\r\n\r\n";
  rt                = send(sock, data, sizeof(data), 0);
  LOG_INFO("send rt=%d errno=%d", rt, errno);

  if (rt <= 0) { return; }

  std::string buff;
  buff.resize(4096);

  rt = recv(sock, &buff[0], buff.size(), 0);
  LOG_INFO("recv rt=%d errno=%d", rt, errno);

  if (rt <= 0) { return; }

  buff.resize(rt);
  LOG_INFO(buff.c_str());
}

int main(int argc, char** argv) {
  // test_sleep();
  IOManager iom;
  iom.schedule(test_sock);
  return 0;
}