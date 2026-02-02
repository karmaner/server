#include "http_connection.h"

#include "basic/log.h"
#include "http_parser.h"

namespace http {

HttpConnection::HttpConnection(Socket::ptr sock, bool owner) : SocketStream(sock, owner) {}

HttpResponse::ptr HttpConnection::recvResponse() {
  HttpResponseParser::ptr parser(new HttpResponseParser);
  uint64_t                buff_size = HttpRequestParser::GetHttpRequestBufferSize();
  // uint64_t buff_size = 100;
  std::shared_ptr<char> buffer(new char[buff_size + 1], [](char* ptr) { delete[] ptr; });
  char*                 data   = buffer.get();
  int                   offset = 0;
  do {
    int len = read(data + offset, buff_size - offset);
    LOG_DEBUG_STREAM << "recvResponse: read header len=" << len;
    if (len <= 0) {
      LOG_ERROR_STREAM << "recvResponse: read header failed, len=" << len << ", errno=" << errno;
      close();
      return nullptr;
    }
    len += offset;
    data[len]     = '\0';
    size_t nparse = parser->execute(data, len, false);
    LOG_DEBUG_STREAM << "recvResponse: execute nparse=" << nparse << ", len=" << len;
    if (parser->hasError()) {
      LOG_ERROR_STREAM << "recvResponse: parser error";
      close();
      return nullptr;
    }
    offset = len - nparse;
    if (offset == (int)buff_size) {
      LOG_ERROR_STREAM << "recvResponse: buffer full";
      close();
      return nullptr;
    }
    if (parser->isFinished()) { break; }
  } while (true);
  auto& client_parser = parser->getParser();
  LOG_INFO_STREAM << "recvResponse: header done, chunked=" << client_parser.chunked 
                  << ", content_len=" << parser->getContentLength()
                  << ", status=" << client_parser.status;
  std::string body;
  if (client_parser.chunked) {
    int len = offset;
    LOG_DEBUG_STREAM << "recvResponse: start chunked, offset=" << offset;
    do {
      bool begin = true;
      do {
        if (!begin || len == 0) {
          int rt = read(data + len, buff_size - len);
          LOG_DEBUG_STREAM << "recvResponse: read chunk rt=" << rt;
          if (rt <= 0) {
            LOG_ERROR_STREAM << "recvResponse: read chunk failed, rt=" << rt << ", errno=" << errno;
            close();
            return nullptr;
          }
          len += rt;
        }

        data[len]     = '\0';
        size_t nparse = parser->execute(data, len, true);
        LOG_DEBUG_STREAM << "recvResponse: chunk execute nparse=" << nparse << ", len=" << len;
        if (parser->hasError()) {
          LOG_ERROR_STREAM << "recvResponse: chunk parser error, data=" << std::string(data, std::min(len, 50));
          close();
          return nullptr;
        }
        len -= nparse;
        if (len == (int)buff_size) {
          LOG_ERROR_STREAM << "recvResponse: chunk buffer full";
          close();
          return nullptr;
        }
        begin = false;
      } while (!parser->isFinished());

      LOG_INFO_STREAM << "content_len=" << client_parser.content_len << ", chunks_done=" << client_parser.chunks_done;
      if (client_parser.content_len <= len) {
        body.append(data, client_parser.content_len);
        memmove(data, data + client_parser.content_len, len - client_parser.content_len);
        len -= client_parser.content_len;
      } else {
        body.append(data, len);
        int left = client_parser.content_len - len;
        while (left > 0) {
          int rt = read(data, left > (int)buff_size ? (int)buff_size : left);
          if (rt <= 0) {
            LOG_ERROR_STREAM << "recvResponse: read chunk body failed, rt=" << rt;
            close();
            return nullptr;
          }
          body.append(data, rt);
          left -= rt;
        }
        len = 0;
      }
    } while (!client_parser.chunks_done);
    LOG_INFO_STREAM << "recvResponse: chunked done, body size=" << body.size();
    parser->getData()->setBody(body);
  } else {
    int64_t length = parser->getContentLength();
    LOG_DEBUG_STREAM << "recvResponse: content-length mode, length=" << length;
    if (length > 0) {
      std::string body;
      body.resize(length);

      int len = 0;
      if (length >= offset) {
        memcpy(&body[0], data, offset);
        len = offset;
      } else {
        memcpy(&body[0], data, length);
        len = length;
      }
      length -= offset;
      if (length > 0) {
        if (readFixSize(&body[len], length) <= 0) {
          LOG_ERROR_STREAM << "recvResponse: readFixSize failed";
          close();
          return nullptr;
        }
      }
      parser->getData()->setBody(body);
    }
  }

  LOG_INFO_STREAM << "recvResponse: success";
  return parser->getData();
}

int HttpConnection::sendRequest(HttpRequest::ptr rsp) {
  std::stringstream ss;
  ss << *rsp;
  std::string data = ss.str();
  return writeFixSize(data.c_str(), data.size());
}

}  // namespace http