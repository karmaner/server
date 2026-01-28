#pragma once

#include "basic/iomanager.h"
#include "basic/tcp_server.h"
#include "http/servlet.h"

namespace http {

class HttpServer : public TcpServer {
public:
  typedef std::shared_ptr<HttpServer> ptr;
  HttpServer(bool keepalive = false, IOManager* worker = IOManager::GetThis(),
             IOManager* io_woker      = IOManager::GetThis(),
             IOManager* accept_worker = IOManager::GetThis());

  ServletDispatch::ptr getServletDispatch() const { return m_dispatch; }
  void                 setServletDispatch(ServletDispatch::ptr v) { m_dispatch = v; }

protected:
  virtual void handleClient(Socket::ptr client) override;

private:
  bool                 m_isKeepalive;
  ServletDispatch::ptr m_dispatch;
};
}  // namespace http