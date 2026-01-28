#pragma once

#include <memory>

#include "basic/address.h"
#include "basic/iomanager.h"
#include "basic/noncopyable.h"
#include "basic/socket.h"

namespace Basic {

class TcpServer : public std::enable_shared_from_this<TcpServer>, NonCopyable {
public:
  typedef std::shared_ptr<TcpServer> ptr;
  TcpServer(IOManager* worker = IOManager::GetThis(), IOManager* io_worker = IOManager::GetThis(),
            IOManager* accept_worker = IOManager::GetThis());
  virtual ~TcpServer();

  virtual bool bind(Address::ptr addr, bool ssl);
  virtual bool bind(const std::vector<Address::ptr>& addrs, std::vector<Address::ptr>& fails,
                    bool ssl);
  virtual bool start();
  virtual void stop();

  uint64_t    getRecvTimeout() const { return m_recvTimeout; }
  std::string getName() const { return m_name; }
  void        setRecvTimeout(uint64_t v) { m_recvTimeout = v; }
  void        setName(const std::string& v) { m_name = v; }

  bool isStop() const { return m_isStop; }

  bool loadCertificates(const std::string& cert_file, const std::string& key_file);

  virtual std::string to_string(const std::string& prefix = "");

protected:
  virtual void handleClient(Socket::ptr client);
  virtual void startAccept(Socket::ptr sock);

protected:
  std::vector<Socket::ptr> m_socks;
  IOManager*               m_worker;
  IOManager*               m_ioWorker;
  IOManager*               m_acceptWorker;
  uint64_t                 m_recvTimeout;
  std::string              m_name;
  std::string              m_type = "tcp";
  bool                     m_isStop;

  bool m_ssl = false;
};

}  // namespace Basic