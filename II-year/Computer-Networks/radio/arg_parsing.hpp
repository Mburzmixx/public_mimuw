#ifndef MB_ARG_PARSING_HPP
#define MB_ARG_PARSING_HPP

#include <string>

#include "logger.hpp"

struct RadioConfig {
  bool multiplex = false;
  bool use_ipv6 = false;
  bool use_ipv4 = false;

  uint32_t timeout = 5000;
  LogLevel verbosity = LogLevel::FATAL;
  std::string url = "";
};

RadioConfig parse_radio_arguments(int argc, char *argv[]);

#endif  // MB_ARG_PARSING_HPP
