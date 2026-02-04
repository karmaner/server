#include <fstream>
#include <iostream>

#include "basic/iomanager.h"
#include "basic/log.h"
#include "http/http_connection.h"

using namespace http;

void test_pool() {
  http::HttpConnectionPool::ptr pool(
      new http::HttpConnectionPool("www.baidu.com", "", 80, 10, 1000 * 30, 5));

  IOManager::GetThis()->addTimer(
      1000,
      [pool]() {
        auto r = pool->doGet("/", 300);
        LOG_INFO_STREAM << r->to_string();
      },
      false);
}

void run() {
  Address::ptr addr = Address::LookupAnyIPAddress("www.example.com:80");
  if (!addr) {
    LOG_INFO_STREAM << "get addr error";
    return;
  }

  Socket::ptr sock = Socket::CreateTCP(addr);
  bool        rt   = sock->connect(addr);
  if (!rt) {
    LOG_INFO_STREAM << "connect " << *addr << " failed";
    return;
  }

  HttpConnection::ptr conn(new HttpConnection(sock));
  HttpRequest::ptr    req(new HttpRequest);
  req->setPath("/index.html");
  req->setHeader("host", "example.com");
  LOG_INFO_STREAM << "req:" << std::endl << *req;

  conn->sendRequest(req);
  auto rsp = conn->recvResponse();

  if (!rsp) {
    LOG_INFO_STREAM << "recv response error";
    return;
  }
  LOG_INFO_STREAM << "rsp:" << std::endl << *rsp;

  std::ofstream ofs("rsp.dat");
  ofs << *rsp;

  LOG_INFO("=============================");
  test_pool();
}

int main(int argc, char* argv[]) {
  IOManager iom(2);
  iom.schedule(run);
  return 0;
}