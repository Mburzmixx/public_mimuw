#include "arg_parsing.hpp"

#include <getopt.h>

#include <cerrno>
#include <stdexcept>

namespace {
const std::string radio_options = "u:mt:46v:q";

const uint32_t MIN_TIMEOUT = 100;
const uint32_t MAX_TIMEOUT = 100000;

const uint8_t MIN_VERBOSITY = 0;
const uint8_t MAX_VERBOSITY = 4;

// This function is used to parse numbers from arguments.
// If checks if the string is a valid number and if it is in the given range.
// If any of the checks fail, it throws `std::invalid_argument` with the given
// error message.
unsigned long long number_from_str(const std::string &str,
                                   const std::string &error_message,
                                   unsigned long long min_value,
                                   unsigned long long max_value) {
  if (str.empty() || str[0] == '-') {
    throw std::invalid_argument(error_message);
  }

  char *end = nullptr;
  errno = 0;
  unsigned long long value = std::strtoull(str.c_str(), &end, 10);
  if (errno == ERANGE || *end != '\0') {
    throw std::invalid_argument(error_message);
  }

  if (value < min_value || value > max_value) {
    throw std::invalid_argument(error_message);
  }

  return value;
}

uint32_t parse_timeout(const char *timeout_str) {
  if (!timeout_str) {
    throw std::invalid_argument("timeout string is null");
  }
  std::string s(timeout_str);
  return static_cast<uint32_t>(
      number_from_str(s, "invalid timeout value\n", MIN_TIMEOUT, MAX_TIMEOUT));
}

LogLevel parse_verbosity(const char *verbosity_str) {
  if (!verbosity_str) {
    throw std::invalid_argument("verbosity string is null");
  }
  std::string s(verbosity_str);
  return static_cast<LogLevel>(number_from_str(s, "invalid verbosity value\n",
                                               MIN_VERBOSITY, MAX_VERBOSITY));
}
}  // namespace

RadioConfig parse_radio_arguments(int argc, char *argv[]) {
  int opt;
  optind = 1;

  bool seen_u = false;
  RadioConfig config{};
  while ((opt = getopt(argc, argv, radio_options.c_str())) != -1) {
    switch (opt) {
      case 'u':
        seen_u = true;
        config.url = std::string(optarg);
        break;
      case 'm':
        config.multiplex = true;
        break;
      case 't':
        config.timeout = parse_timeout(optarg);
        break;
      case '4':
        config.use_ipv4 = true;
        break;
      case '6':
        config.use_ipv6 = true;
        break;
      case 'v':
        config.verbosity = parse_verbosity(optarg);
        break;
      case 'q':
        config.verbosity = LogLevel::ZERO;
        break;
      case '?':
        throw std::invalid_argument("missing argument for option: -" +
                                    std::string(1, optopt) + "\n");
      default:
        throw std::invalid_argument("getopt error");
    }
  }
  if (!seen_u) {
    throw std::invalid_argument("missing required -u argument (url)\n");
  }
  if (config.url.empty()) {
    throw std::invalid_argument("URL cannot be empty\n");
  }
  return config;
}