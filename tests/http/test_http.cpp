#include "http/http.h"

void test_request() {
	http::HttpRequest::ptr req(new http::HttpRequest);
	req->setHeader("host" , "www.ttfkyk.top");
	req->setVersion(0x20);
	req->setMethod(http::HttpMethod::POST);
	req->setBody("hello server");
	req->dump(std::cout) << std::endl;
}

void test_response() {
	http::HttpResponse::ptr rsp(new http::HttpResponse);
	rsp->setHeader("X-X", "ttfkyk");
	rsp->setBody("hello server");
	rsp->setStatus((http::HttpStatus)400);
	rsp->setClose(false);

	rsp->dump(std::cout) << std::endl;
}

int main(int argc, char** argv) {
	test_request();
	test_response();
	return 0;
}
