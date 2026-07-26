#include "connection.hpp"

#include <errno.h>

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace {
constexpr std::size_t ONE = 1;
}

Connection::Connection(int fd) : _fd(fd) {}

Connection::~Connection() {
  if (_fd != -1) {
    close(_fd);
  }
}

void Connection::flush_input_buffer(
    std::function<void(const char *, std::size_t)> consumer) {
  if (_in_buf_head < _in_buf_tail) {
    consumer(_in_buffer + _in_buf_head, _in_buf_tail - _in_buf_head);
    _in_buf_head = _in_buf_tail = 0;
  }
}

ssize_t Connection::read_data(char *buffer, std::size_t length) {
  std::size_t bytes_copied = 0;

  if (_in_buf_head >= _in_buf_tail) {
    ssize_t bytes_read = fill_buffer();
    if (bytes_read <= 0) {
      return bytes_read;  // Error while reading from socket or end of stream
    }
  }
  std::size_t bytes_available = _in_buf_tail - _in_buf_head;
  std::size_t bytes_to_copy = std::min(bytes_available, length - bytes_copied);
  std::memcpy(buffer + bytes_copied, _in_buffer + _in_buf_head, bytes_to_copy);
  _in_buf_head += bytes_to_copy;
  bytes_copied += bytes_to_copy;

  return static_cast<ssize_t>(bytes_copied);
}

ssize_t Connection::read_byte(char *byte) { return read_data(byte, ONE); }

ssize_t Connection::writen(const void *vptr, std::size_t n) {
  size_t nleft = n;
  ssize_t nwritten;
  const char *ptr = static_cast<const char *>(vptr);

  while (nleft > 0) {
    if ((nwritten = write_data(ptr, nleft)) <= 0) {
      if (nwritten < 0 && errno == EINTR) {
        nwritten = 0;  // Retry the write operation
      } else {
        return nwritten;  // An error occurred
      }
    }
    nleft -= nwritten;
    ptr += nwritten;
  }
  return n;
}

bool Connection::has_pending_data() const { return false; }

int Connection::get_fd() { return _fd; }

ssize_t Connection::fill_buffer() {
  if (_in_buf_head < _in_buf_tail) {
    return 0;
  }
  ssize_t bytes_read = read(_fd, _in_buffer, BUFFER_SIZE);
  if (bytes_read > 0) {
    _in_buf_head = 0;
    _in_buf_tail = static_cast<std::size_t>(bytes_read);
  }
  return bytes_read;
}

ssize_t Connection::write_data(const void *buffer, std::size_t length) {
  return write(_fd, buffer, length);
}

SecureConnection::SecureConnection(int fd, std::string host) : Connection(fd) {
  SSL_load_error_strings();
  OpenSSL_add_ssl_algorithms();

  _ctx = SSL_CTX_new(TLS_client_method());
  if (!_ctx) {
    throw std::runtime_error("SSL_CTX_new failed");
  }

  SSL_CTX_set_verify(_ctx, SSL_VERIFY_PEER, nullptr);
  if (!SSL_CTX_set_default_verify_paths(_ctx)) {
    throw std::runtime_error("SSL_CTX_set_default_verify_paths failed");
  }
  _ssl = SSL_new(_ctx);
  if (!_ssl) {
    SSL_CTX_free(_ctx);
    throw std::runtime_error("SSL_new failed");
  }

  SSL_set_fd(_ssl, _fd);

  SSL_set_tlsext_host_name(_ssl, host.c_str());
  SSL_set1_host(_ssl, host.c_str());
  int error_code = SSL_connect(_ssl);
  if (error_code <= 0) {
    int ssl_error = SSL_get_error(_ssl, error_code);
    SSL_free(_ssl);
    SSL_CTX_free(_ctx);
    throw std::runtime_error("SSL connection failed" +
                             std::to_string(ssl_error));
  }
}

SecureConnection::~SecureConnection() {
  if (_ssl) {
    SSL_shutdown(_ssl);
    SSL_free(_ssl);
  }
  if (_ctx) {
    SSL_CTX_free(_ctx);
  }
}

void SecureConnection::flush_input_buffer(
    std::function<void(const char *, std::size_t)> consumer) {
  Connection::flush_input_buffer(consumer);

  // This will end when we consume all data buffered by OpenSSL.
  while (has_pending_data()) {
    int read_bytes = SSL_read(_ssl, _in_buffer, BUFFER_SIZE);
    if (read_bytes > 0) {
      consumer(_in_buffer, read_bytes);
    } else {
      break;
    }
  }
}

bool SecureConnection::has_pending_data() const {
  return SSL_has_pending(_ssl) > 0;
}

ssize_t SecureConnection::fill_buffer() {
  if (_in_buf_head < _in_buf_tail) {
    return 0;
  }

  int bytes_read = SSL_read(_ssl, _in_buffer, BUFFER_SIZE);
  if (bytes_read > 0) {
    _in_buf_head = 0;
    _in_buf_tail = static_cast<std::size_t>(bytes_read);
  }
  return bytes_read;
}

ssize_t SecureConnection::write_data(const void *buffer, size_t length) {
  return static_cast<ssize_t>(
      SSL_write(_ssl, buffer, static_cast<int>(length)));
}
