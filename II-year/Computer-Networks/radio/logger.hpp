#ifndef MB_RADIO_LOGGER_HPP
#define MB_RADIO_LOGGER_HPP

#include <unistd.h>

#include <cstdint>
#include <string>

enum class LogLevel : uint8_t {
  ZERO = (0),
  INFO = (1),
  FATAL = (2),
  NON_FATAL = (3),
  DEBUG = (4)
};

class Logger {
 public:
  static void set_verbosity(LogLevel level);
  static int get_verbosity();

  static void log(LogLevel level, const std::string &message);

 private:
  static LogLevel verbosity;
};

ssize_t writen_to_fd(int fd, const void *vptr, std::size_t n);

#endif  // MB_RADIO_LOGGER_HPP