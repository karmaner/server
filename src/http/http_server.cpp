#include "http/http_server.h"

#include "basic/iomanager.h"
#include "basic/log.h"
#include "basic/tcp_server.h"

namespace http {

HttpServer::HttpServer(bool keepalive, IOManager* worker, IOManager* io_woker,
                       IOManager* accept_worker)
    : TcpServer(worker, io_woker, accept_worker), m_isKeepalive(keepalive) {
  m_dispatch.reset(new ServletDispatch);

  m_type = "http";
  // m_dispatch->addServlet("/_/status", Servlet::ptr(new StatusServlet));
  // m_dispatch->addServlet("/_/config", Servlet::ptr(new ConfigServlet));
}

void HttpServer::handleClient(Socket::ptr client) {
  HttpSession::ptr session(new HttpSession(client));
  do {
    auto req = session->recvRequest();
    if (!req) {
      LOG_WARN_STREAM << "recv http request fail, errno=" << errno << " errstr=" << strerror(errno)
                      << " cliet:" << *client;
      break;
    }

    HttpResponse::ptr rsp(new HttpResponse(req->getVersion(), req->isClose() || !m_isKeepalive));
    rsp->setHeader("Server", getName());
    m_dispatch->handle(req, rsp, session);
    rsp->setBody("hello server");

    LOG_WARN_STREAM << "requst:" << std::endl << *req;
    LOG_WARN_STREAM << "response:" << std::endl << *rsp;
    session->sendResponse(rsp);
  } while (m_isKeepalive);
  session->close();
}

}  // namespace http