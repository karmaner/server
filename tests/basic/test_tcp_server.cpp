#include "basic/tcp_server.h"
#include "server.h"

using namespace Basic;

void run() {
  auto addr = Address::LookupAny("0.0.0.0:8033");
  // auto addr2 = UnixAddress::ptr(new UnixAddress("/tmp/unix_addr"));
  std::vector<Address::ptr> addrs;
  addrs.push_back(addr);
  // addrs.push_back(addr2);

  TcpServer::ptr tcp_server(new TcpServer);

  std::vector<Address::ptr> fails;
  while (!tcp_server->bind(addrs, fails, false)) {
    sleep(2);
  }
  tcp_server->start();
}
int main(int argc, char** argv) {
  IOManager iom(2);
  iom.schedule(run);
  return 0;
}
