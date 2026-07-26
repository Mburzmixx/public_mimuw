#ifndef MB_CONNECTION_HPP
#define MB_CONNECTION_HPP

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <unistd.h>

#include <cstddef>
#include <functional>
#include <string>

constexpr std::size_t BUFFER_SIZE = 4096;

class Connection {
 public:
  Connection(int fd);
  Connection(const Connection &) = delete;
  Connection &operator=(const Connection &) = delete;
  virtual ~Connection();

  virtual void flush_input_buffer(
      std::function<void(const char *, std::size_t)> consumer);
  virtual ssize_t read_data(char *buffer, std::size_t length);
  virtual ssize_t read_byte(char *byte);
  virtual ssize_t writen(const void *vptr, std::size_t n);

  // This function is used to check if some data is pending to be read.
  // For a regular `Connection`, this will always return false,
  // but for a `SecureConnection` it will call `SSL_has_pending`.
  virtual bool has_pending_data() const;
  int get_fd();

 protected:
  std::size_t _in_buf_head = 0;
  std::size_t _in_buf_tail = 0;
  char _in_buffer[BUFFER_SIZE];
  int _fd = -1;

  virtual ssize_t fill_buffer();
  virtual ssize_t write_data(const void *buffer, std::size_t length);
};

class SecureConnection : public Connection {
 public:
  SecureConnection(int fd, std::string host);
  SecureConnection(const SecureConnection &) = delete;
  SecureConnection &operator=(const SecureConnection &) = delete;
  ~SecureConnection() override;

  void flush_input_buffer(
      std::function<void(const char *, std::size_t)> consumer) override;
  bool has_pending_data() const override;

 private:
  SSL *_ssl = nullptr;
  SSL_CTX *_ctx = nullptr;

  ssize_t fill_buffer() override;
  ssize_t write_data(const void *buffer, std::size_t length) override;
};

#endif  // MB_CONNECTION_HPP