#include <map>

#include "basic/address.h"
#include "basic/log.h"

using namespace Basic;

void test() {
  std::vector<Address::ptr> addrs;

  LOG_INFO("begin");
  bool v = Address::Lookup(addrs, "www.baidu.com");
  LOG_INFO("end");

  if (!v) {
    LOG_ERROR("lookup fail");
    return;
  }

  for (size_t i = 0; i < addrs.size(); ++i) {
    LOG_INFO("%d - %s", i, addrs[i]->to_string().c_str());
  }
}

void test_iface() {
  std::multimap<std::string, std::pair<Address::ptr, uint32_t> > results;

  bool v = Address::GetInterfaceAddresses(results);
  if (!v) {
    LOG_ERROR("GetInterfaceAddresses fail");
    return;
  }

  for (auto& i : results) {
    LOG_INFO("%s - %s", i.first.c_str(), i.second.first->to_string().c_str());
  }
}

void test_ipv4() {
  auto addr = IPAddress::Create("127.0.0.8");
  if (addr) { LOG_INFO("%s", addr->to_string().c_str()); }
}

void test_ipv6() {
  auto addr = IPAddress::Create("::1");
  if (addr) { LOG_INFO("%s", addr->to_string().c_str()); }
}

int main(int argc, char* argv[]) {
  // test_ipv4();
  test_ipv6();
  // test_iface();
  // test();

  return 0;
}