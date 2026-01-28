#pragma once

#include "http/servlet.h"

namespace http {

class StatusServlet : public Servlet {
public:
  StatusServlet();
  virtual int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response,
                         HttpSession::ptr session) override;
};

}  // namespace http
