#include "logger.hpp"

LogLevel Logger::verbosity = LogLevel::FATAL;

void Logger::set_verbosity(LogLevel level) { Logger::verbosity = level; }

int Logger::get_verbosity() { return static_cast<int>(Logger::verbosity); }

void Logger::log(LogLevel level, const std::string &message) {
  if (static_cast<int>(level) <= static_cast<int>(Logger::verbosity)) {
    std::string log_message = message + "\n";

    if (level == LogLevel::DEBUG) {
      // Indent debug messages for better readability.
      log_message = "\t" + log_message;
    }
    writen_to_fd(STDERR_FILENO, (log_message).c_str(), log_message.length());
  }
}

ssize_t writen_to_fd(int fd, const void *vptr, std::size_t n) {
  size_t nleft = n;
  ssize_t nwritten;
  const char *ptr = static_cast<const char *>(vptr);

  while (nleft > 0) {
    if ((nwritten = write(fd, ptr, nleft)) <= 0) return nwritten;  // error

    nleft -= nwritten;
    ptr += nwritten;
  }
  return n;
}
