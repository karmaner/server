#include <memory>

#include "basic/socket.h"
#include "basic/stream.h"

namespace Basic {
class SocketStream : public Stream {
public:
  typedef std::shared_ptr<SocketStream> ptr;

  SocketStream(Socket::ptr socket, bool owner = true);
  ~SocketStream();

  virtual int  read(void* buffer, size_t length) override;
  virtual int  read(ByteArray::ptr ba, size_t length) override;
  virtual int  write(const void* buffer, size_t length) override;
  virtual int  write(ByteArray::ptr ba, size_t length) override;
  virtual void close() override;

  Socket::ptr getSocket() const { return m_socket; }
  bool        isConnected() const;

  Address::ptr getRemoteAddress();
  Address::ptr getLocalAddress();
  std::string  getRemoteAddressString();
  std::string  getLocalAddressString();

protected:
  Socket::ptr m_socket;
  bool        m_owner;
};
}  // namespace Basic