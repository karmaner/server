#include "basic/log.h"
#include "http/http_server.h"

using namespace http;

void run() {
  HttpServer::ptr server(new HttpServer);
  Address::ptr    addr = Address::LookupAnyIPAddress("0.0.0.0:8020");
  while (!server->bind(addr, false)) {
    sleep(2);
  }
  auto sd = server->getServletDispatch();
  sd->addServlet("/server/xx",
                 [](HttpRequest::ptr req, HttpResponse::ptr rsp, HttpSession::ptr session) {
                   rsp->setBody(req->to_string());
                   return 0;
                 });

  sd->addGlobServlet("/server/*",
                     [](HttpRequest::ptr req, HttpResponse::ptr rsp, HttpSession::ptr session) {
                       rsp->setBody("Glob:\r\n" + req->to_string());
                       return 0;
                     });
  server->start();
}

int main(int argc, char* argv[]) {
  IOManager iom(2);
  iom.schedule(run);
  return 0;
}