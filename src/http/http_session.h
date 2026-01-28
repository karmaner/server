#pragma once

#include <memory>

#include "http/http.h"
#include "stream/socket_stream.h"
using namespace Basic;
namespace http {

class HttpSession : public SocketStream {
public:
  typedef std::shared_ptr<HttpSession> ptr;

  HttpSession(Socket::ptr sock, bool owner = true);

  HttpRequest::ptr recvRequest();

  int sendResponse(HttpResponse::ptr rsp);
};

}  // namespace http