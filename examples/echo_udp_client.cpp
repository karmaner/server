#include <stdlib.h>

#include "basic/address.h"
#include "basic/iomanager.h"
#include "basic/log.h"
#include "basic/socket.h"

using namespace Basic;

const char* ip   = nullptr;
uint16_t    port = 0;

void run() {
  IPAddress::ptr addr = Address::LookupAnyIPAddress(ip);
  if (!addr) {
    LOG_ERROR_STREAM << "invalid ip: " << ip;
    return;
  }
  addr->setPort(port);
  Socket::ptr sock = Socket::CreateUDP(addr);

  IOManager::GetThis()->schedule([sock]() {
    Address::ptr addr(new IPv4Address);
    LOG_INFO_STREAM << "begin recv";
    while (true) {
      char buff[1024];
      int  len = sock->recvFrom(buff, sizeof(buff), addr);
      if (len > 0) {
        std::cout << std::endl
                  << "recv: " << std::string(buff, len) << " form: " << *addr << std::endl;
      }
    }
  });
  sleep(1);
  while (true) {
    std::string line;
    std::cout << "input>";
    std::getline(std::cin, line);
    if (!line.empty()) {
      int len = sock->sendTo(line.c_str(), line.size(), addr);
      if (len < 0) {
        int err = sock->getError();
        LOG_ERROR_STREAM << "send error err=" << err << " errstr=" << strerror(err)
                         << " len=" << len << " addr=" << *addr << " sock=" << *sock;
      } else {
        LOG_INFO_STREAM << "send " << line << " len:" << len;
      }
    }
  }
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    LOG_INFO_STREAM << "use as[" << argv[0] << " ip port]";
    return 0;
  }
  ip   = argv[1];
  port = atoi(argv[2]);
  Basic::IOManager iom(2);
  iom.schedule(run);
  return 0;
}