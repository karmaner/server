#include "basic/address.h"
#include "basic/iomanager.h"
#include "basic/log.h"
#include "basic/socket.h"
#include "http/http_server.h"

using namespace Basic;

void run() {
  IPAddress::ptr addr = Address::LookupAnyIPAddress("0.0.0.0:8020");

  Socket::ptr sock = Socket::CreateUDP(addr);
  if (sock->bind(addr)) {
    LOG_INFO_STREAM << "udp bind : " << *addr;
  } else {
    LOG_ERROR_STREAM << "udp bind : " << *addr << "fail";
    return;
  }

  http::HttpServer::ptr http_server(new http::HttpServer(true));
  while (!http_server->bind(addr, false)) {
    LOG_ERROR_STREAM << "bind " << *addr << " fail";
    sleep(1);
  }

  http_server->start();
}

int main(int argc, char* argv[]) {
  Basic::IOManager iom(1);
  iom.schedule(run);
  return 0;
}
