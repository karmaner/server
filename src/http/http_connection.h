#pragma once

#include "http/http.h"
#include "stream/socket_stream.h"
using namespace Basic;
namespace http {

class HttpConnection : public SocketStream {
public:
  typedef std::shared_ptr<HttpConnection> ptr;
  HttpConnection(Socket::ptr sock, bool owner = true);
  HttpResponse::ptr recvResponse();
  int               sendRequest(HttpRequest::ptr req);
};
}  // namespace http