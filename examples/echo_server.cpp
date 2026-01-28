#include "basic/bytearray.h"
#include "basic/log.h"
#include "basic/socket.h"
#include "basic/tcp_server.h"

class EchoServer : public Basic::TcpServer {
public:
  EchoServer(int type);
  void handleClient(Basic::Socket::ptr client);

private:
  int m_type = 0;
};

EchoServer::EchoServer(int type) : m_type(type) {}

void EchoServer::handleClient(Basic::Socket::ptr client) {
  LOG_INFO_STREAM << "handleClient " << *client;
  Basic::ByteArray::ptr ba(new Basic::ByteArray);
  while (true) {
    ba->clear();
    std::vector<iovec> iovs;
    ba->getWriteBuffers(iovs, 1024);

    int rt = client->recv(&iovs[0], iovs.size());
    if (rt == 0) {
      LOG_INFO_STREAM << "client close: " << *client;
      break;
    } else if (rt < 0) {
      LOG_INFO_STREAM << "client error rt=" << rt << " errno=" << errno
                      << " errstr=" << strerror(errno);
      break;
    }
    ba->setPosition(ba->getPosition() + rt);
    ba->setPosition(0);
    LOG_INFO_STREAM << "recv rt=" << rt << " data=" << std::string((char*)iovs[0].iov_base, rt);
    if (m_type == 1) {               // text
      std::cout << ba->to_string();  // << std::endl;
    } else {
      std::cout << ba->to_string();  // << std::endl;
    }
    std::cout.flush();
  }
}

int type = 1;

void run() {
  LOG_INFO_STREAM << "server type=" << type;
  EchoServer::ptr es(new EchoServer(type));
  auto            addr = Basic::Address::LookupAny("0.0.0.0:8020");
  while (!es->bind(addr, false)) {
    sleep(2);
  }
  es->start();
}

int main(int argc, char** argv) {
  if (argc < 2) {
    LOG_INFO_STREAM << "used as[" << argv[0] << " -t] or [" << argv[0] << " -b]";
    return 0;
  }

  if (!strcmp(argv[1], "-b")) { type = 2; }

  Basic::IOManager iom(2);
  iom.schedule(run);
  return 0;
}
