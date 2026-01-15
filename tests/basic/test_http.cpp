#include "http/http.h"

using namespace http;

void test_request() {
  HttpRequest::ptr req(new http::HttpRequest);
  req->setHeader("host", "www.ttfkyk.top");
  req->setBody("hello server");
  req->dump(std::cout) << std::endl;
}

void test_response() {
  HttpResponse::ptr rsp(new HttpResponse);
  rsp->setHeader("X-X", "server");
  rsp->setBody("hello server");
  rsp->setStatus((HttpStatus)400);
  rsp->setClose(false);

  rsp->dump(std::cout) << std::endl;
}

int main(int argc, char* argv[]) {
  test_request();
  test_response();
  return 0;
}