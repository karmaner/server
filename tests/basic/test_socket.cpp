#include "basic/iomanager.h"
#include "basic/log.h"
#include "basic/socket.h"
#include "basic/utils.h"
#include "server.h"

using namespace Basic;

void test_socket() {
  std::vector<Address::ptr> addrs;
  Address::Lookup(addrs, "www.baidu.com", AF_INET);
  IPAddress::ptr addr;
  for (auto& i : addrs) {
    LOG_INFO_STREAM << i->to_string();
    addr = std::dynamic_pointer_cast<IPAddress>(i);
    if (addr) { break; }
  }

  addr = Address::LookupAnyIPAddress("www.baidu.com");
  if (addr) {
    LOG_INFO_STREAM << "get address: " << addr->to_string();
  } else {
    LOG_INFO_STREAM << "get address fail";
    return;
  }

  Socket::ptr sock = Socket::CreateTCP(addr);
  addr->setPort(80);
  LOG_INFO_STREAM << "addr=" << addr->to_string();
  if (!sock->connect(addr)) {
    LOG_INFO_STREAM << "connect " << addr->to_string() << " fail";
    return;
  } else {
    LOG_INFO_STREAM << "connect " << addr->to_string() << " connected";
  }

  const char buff[] = "GET / HTTP/1.0\r\n\r\n";
  int        rt     = sock->send(buff, sizeof(buff));
  if (rt <= 0) {
    LOG_INFO_STREAM << "send fail rt=" << rt;
    return;
  }

  std::string buffs;
  buffs.resize(4096);
  rt = sock->recv(&buffs[0], buffs.size());

  if (rt <= 0) {
    LOG_INFO_STREAM << "recv fail rt=" << rt;
    return;
  }

  buffs.resize(rt);
  LOG_INFO_STREAM << buffs;
}

void test2() {
  IPAddress::ptr addr = Address::LookupAnyIPAddress("www.baidu.com:80");
  if (addr) {
    LOG_INFO_STREAM << "get address: " << addr->to_string();
  } else {
    LOG_INFO_STREAM << "get address fail";
    return;
  }

  Socket::ptr sock = Socket::CreateTCP(addr);
  if (!sock->connect(addr)) {
    LOG_INFO_STREAM << "connect " << addr->to_string() << " fail";
    return;
  } else {
    LOG_INFO_STREAM << "connect " << addr->to_string() << " connected";
  }

  uint64_t ts = get_current_us();
  for (size_t i = 0; i < 10000000000ul; ++i) {
    if (int err = sock->getError()) {
      LOG_INFO_STREAM << "err=" << err << " errstr=" << strerror(err);
      break;
    }

    static int batch = 10000000;
    if (i && (i % batch) == 0) {
      uint64_t ts2 = get_current_us();
      LOG_INFO_STREAM << "i=" << i << " used: " << ((ts2 - ts) * 1.0 / batch) << " us";
      ts = ts2;
    }
  }
}

int main(int argc, char** argv) {
  IOManager iom;
  // iom.schedule(&test_socket);
  iom.schedule(&test2);
  return 0;
}
