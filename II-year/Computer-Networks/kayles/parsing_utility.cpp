#include "parsing_utility.hpp"

namespace {
const std::string client_options = "a:p:m:t:";
const std::string server_options = "r:a:p:t:";

constexpr bool ALLOW_ZERO_PORT = true;
constexpr bool DISALLOW_ZERO_PORT = false;

constexpr uint32_t MIN_SERVER_PORT = 1;
constexpr uint32_t ZERO_PORT = 0;
constexpr uint32_t MAX_SERVER_PORT = UINT16_MAX;

constexpr uint8_t MIN_TIMEOUT = 1;
constexpr uint8_t MAX_TIMEOUT = 99;

constexpr char MSG_DELIMITER = '/';

constexpr uint16_t MIN_PAWN_ROW_LENGTH = 1;
constexpr uint16_t MAX_PAWN_ROW_LENGTH = 256;
constexpr char PAWN_CHAR = '1';
constexpr char EMPTY_CHAR = '0';

constexpr uint8_t MIN_PAWN_POSITION = 0;
constexpr uint8_t MAX_PAWN_POSITION = UINT8_MAX;

constexpr uint32_t MIN_PLAYER_ID = 1;
constexpr uint32_t MAX_PLAYER_ID = UINT32_MAX;

constexpr uint32_t MIN_GAME_ID = 0;
constexpr uint32_t MAX_GAME_ID = UINT32_MAX;

const std::string client_usage = ("Proper usage: "
                                  "./kayles_client -a <address> -p <port>"
                                  " -m <message> -t <client_timeout>");

const std::string server_usage = "Proper usage: "
                                 "./kayles_server -r <pawn_row> -a <address>"
                                 " -p <port> -t <server_timeout>";

[[noreturn]] void fatal_client_usage(const std::string &error_log) {
  fatal((error_log + client_usage).c_str());
}

[[noreturn]] void fatal_server_usage(const std::string &error_log) {
  fatal((error_log + server_usage).c_str());
}

std::vector<std::string> split_string(const char *str, char delimiter) {
  if (!str) {
    fatal("Input string is null.\n");
  }

  std::string s(str);
  std::vector<std::string> tokens;
  size_t start = 0, end = 0;
  while ((end = s.find(delimiter, start)) != std::string::npos) {
    tokens.push_back(s.substr(start, end - start));
    start = end + 1;
  }
  tokens.push_back(s.substr(start));
  return tokens;
}

bool validate_message_type_and_fields(const ClientMessage &message,
                                      size_t num_fields) {
  switch (message.msg_type) {
  case MessageType::MSG_JOIN:
    return num_fields == 2;
  case MessageType::MSG_MOVE_2:
    if (message.pawn == MAX_PAWN_POSITION) {
      return false; // Invalid pawn value, so the message is invalid.
    }
    return num_fields == 4;
  case MessageType::MSG_MOVE_1:
    return num_fields == 4;
  case MessageType::MSG_KEEP_ALIVE:
  case MessageType::MSG_GIVE_UP:
    return num_fields == 3;
  default:
    return false;
  }
}

unsigned long long number_from_str(const std::string &str,
                                   const char *error_message,
                                   unsigned long long min_value,
                                   unsigned long long max_value) {
  if (str.empty() || str[0] == '-') {
    fatal(error_message);
  }

  char *end = nullptr;
  errno = 0;
  unsigned long long value = std::strtoull(str.c_str(), &end, 10);
  if (errno == ERANGE || *end != '\0') {
    fatal(error_message);
  }

  if (value < min_value || value > max_value) {
    fatal(error_message);
  }

  return value;
}

ClientMessage parse_message(const char *message_str) {
  if (!message_str) {
    fatal_client_usage("Message string is null.\n");
  }

  auto parts = split_string(message_str, MSG_DELIMITER);
  if (parts.empty()) {
    fatal_client_usage("Message string is empty.\n");
  }

  ClientMessage message{};

  errno = 0;
  message.msg_type = static_cast<MessageType>(number_from_str(
      parts[0], "Invalid message type.\n", MIN_MESSAGE_TYPE, MAX_MESSAGE_TYPE));
  if (parts.size() > 1) {
    message.player_id = static_cast<player_id_t>(number_from_str(
        parts[1], "Invalid player ID.\n", MIN_PLAYER_ID, MAX_PLAYER_ID));
  }
  if (parts.size() > 2) {
    message.game_id = static_cast<game_id_t>(number_from_str(
        parts[2], "Invalid game ID.\n", MIN_GAME_ID, MAX_GAME_ID));
  }
  if (parts.size() > 3) {
    message.pawn = static_cast<uint8_t>(
        number_from_str(parts[3], "Invalid pawn value.\n", MIN_PAWN_POSITION,
                        MAX_PAWN_POSITION));
  }

  if (errno == ERANGE) {
    fatal_client_usage("Numeric value in the message is out of range.\n");
  }

  if (!validate_message_type_and_fields(message, parts.size())) {
    fatal_client_usage("Invalid message type or invalid number of fields.\n");
  }

  return message;
}

uint16_t parse_port(const char *port_str, bool allow_zero) {
  if (!port_str) {
    fatal("Port string is null.\n");
  }
  std::string s(port_str);

  uint32_t port_min_val = allow_zero ? ZERO_PORT : MIN_SERVER_PORT;
  return static_cast<uint16_t>(number_from_str(s, "Invalid port number.\n",
                                               port_min_val, MAX_SERVER_PORT));
}

struct sockaddr_in get_server_address(char const *host, uint16_t port) {
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;      // IPv4
  hints.ai_socktype = SOCK_DGRAM; // UDP
  hints.ai_protocol = IPPROTO_UDP;

  struct addrinfo *address_result;
  int errcode = getaddrinfo(host, NULL, &hints, &address_result);
  if (errcode != 0) {
    fatal("getaddrinfo: %s", gai_strerror(errcode));
  }

  struct sockaddr_in send_address;
  send_address.sin_family = AF_INET; // IPv4
  send_address.sin_addr.s_addr =     // IP address
      ((struct sockaddr_in *)(address_result->ai_addr))->sin_addr.s_addr;
  send_address.sin_port = htons(port); // port from the command line

  freeaddrinfo(address_result);

  return send_address;
}

uint8_t parse_timeout(const char *timeout_str) {
  if (!timeout_str) {
    fatal("Timeout string is null.\n");
  }
  std::string s(timeout_str);
  return static_cast<uint8_t>(
      number_from_str(s, "Invalid timeout value.\n", MIN_TIMEOUT, MAX_TIMEOUT));
}

std::pair<std::vector<uint8_t>, uint8_t>
parse_pawn_row(const char *pawn_row_str) {
  if (!pawn_row_str) {
    fatal("Pawn row string is null.\n");
  }

  std::string s(pawn_row_str);
  if (s.length() < MIN_PAWN_ROW_LENGTH || s.length() > MAX_PAWN_ROW_LENGTH) {
    fatal("Pawn row string has invalid length.\n");
  }

  if (s.front() != PAWN_CHAR || s.back() != PAWN_CHAR) {
    fatal("Pawn row string must begin and end with a '1'.\n");
  }

  size_t num_bytes = (s.length() - 1) / 8 + 1;
  std::vector<uint8_t> pawn_row(num_bytes, 0);
  int bit = 7;
  int byte_index = 0;
  for (char c : s) {
    if (c != PAWN_CHAR && c != EMPTY_CHAR) {
      fatal("Pawn row string contains invalid characters.\n");
    }
    if (c == PAWN_CHAR) {
      pawn_row[byte_index] |= (1 << bit);
    }
    if (--bit < 0) {
      bit = 7;
      byte_index++;
    }
  }

  // Note that the last byte may have unused bits, but they will be zeroed out
  // by default since we initialized the vector with zeros.
  // I return s.length() - 1 as the max pawn position because
  // the positions are 0-indexed.
  return std::make_pair(pawn_row, static_cast<uint8_t>(s.length() - 1));
}
}; // namespace

ClientConfig parse_client_arguments(int argc, char *argv[]) {
  int opt;
  optind = 1; // reset optind in case of multiple calls to this function
  bool seen_a = false, seen_p = false, seen_m = false, seen_t = false;

  std::string host;
  ClientConfig config{};
  while ((opt = getopt(argc, argv, client_options.c_str())) != -1) {
    switch (opt) {
    case 'a':
      if (seen_a)
        fatal_client_usage("Duplicate -a argument.\n");
      seen_a = true;
      host = std::string(optarg);
      break;
    case 'p':
      if (seen_p)
        fatal_client_usage("Duplicate -p argument.\n");
      seen_p = true;
      config.port = parse_port(optarg, DISALLOW_ZERO_PORT);
      break;
    case 'm':
      if (seen_m)
        fatal_client_usage("Duplicate -m argument.\n");
      seen_m = true;
      config.message_to_send = parse_message(optarg);
      break;
    case 't':
      if (seen_t)
        fatal_client_usage("Duplicate -t argument.\n");
      seen_t = true;
      config.client_timeout = parse_timeout(optarg);
      break;
    case 'h':
      fatal_client_usage("");
    default:
      syserr("getopt error");
    }
  }

  if (!seen_a || !seen_p || !seen_m || !seen_t) {
    fatal_client_usage("Missing required arguments.\n");
  }
  if (optind < argc) {
    fatal_client_usage("Unexpected arguments.\n");
  }

  config.server_address = get_server_address(host.c_str(), config.port);
  return config;
}

ServerConfig parse_server_arguments(int argc, char *argv[]) {
  int opt;
  optind = 1; // reset optind in case of multiple calls to this function
  bool seen_r = false, seen_a = false, seen_p = false, seen_t = false;

  std::string host;
  ServerConfig config{};
  while ((opt = getopt(argc, argv, server_options.c_str())) != -1) {
    switch (opt) {
    case 'r':
      if (seen_r)
        fatal_server_usage("Duplicate -r argument.\n");
      seen_r = true;
      std::tie(config.pawn_row, config.max_pawn) = parse_pawn_row(optarg);
      break;
    case 'a':
      if (seen_a)
        fatal_server_usage("Duplicate -a argument.\n");
      seen_a = true;
      host = std::string(optarg);
      break;
    case 'p':
      if (seen_p)
        fatal_server_usage("Duplicate -p argument.\n");
      seen_p = true;
      config.port = parse_port(optarg, ALLOW_ZERO_PORT);
      break;
    case 't':
      if (seen_t)
        fatal_server_usage("Duplicate -t argument.\n");
      seen_t = true;
      config.server_timeout = std::chrono::seconds(parse_timeout(optarg));
      break;
    case 'h':
      fatal_server_usage("");
    default:
      syserr("getopt error");
    }
  }

  if (!seen_r || !seen_a || !seen_p || !seen_t) {
    fatal_server_usage("Missing required arguments.\n");
  }
  if (optind < argc) {
    fatal_server_usage("Unexpected arguments.\n");
  }

  config.server_address = get_server_address(host.c_str(), config.port);
  return config;
}

void print_pawn_row(const std::vector<uint8_t> &pawn_row, uint8_t max_pawn,
                    std::ostream &out) {
  constexpr int BITS_PER_BYTE = 8;
  int printed_pawns = 0;
  for (const uint8_t &byte : pawn_row) {
    for (int bit = BITS_PER_BYTE - 1; bit >= 0; --bit) {
      out << ((byte >> bit) & 1);
      if (printed_pawns == max_pawn) {
        break;
      }
      printed_pawns++;
    }
    // This check is needed in case the last byte has unused bits.
    if (printed_pawns == max_pawn) {
      break;
    }
  }
  out << "\n";
}
