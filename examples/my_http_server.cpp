#include "basic/log.h"
#include "http/http_server.h"

void run() {
  // g_logger->setLevel(LogLevel::INFO);
  Address::ptr addr = Address::LookupAnyIPAddress("0.0.0.0:8020");
  if (!addr) {
    LOG_ERROR_STREAM << "get address error";
    return;
  }

  http::HttpServer::ptr http_server(new http::HttpServer(true));
  while (!http_server->bind(addr, false)) {
    LOG_ERROR_STREAM << "bind " << *addr << " fail";
    sleep(1);
  }

  http_server->start();
}

int main(int argc, char** argv) {
  IOManager iom(1);
  iom.schedule(run);
  return 0;
}