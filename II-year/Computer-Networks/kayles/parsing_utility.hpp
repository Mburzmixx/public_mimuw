#ifndef MB_PARSING_UTILITY_HPP
#define MB_PARSING_UTILITY_HPP

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>

#include "err.hpp"
#include "protocol.hpp"

using Clock = std::chrono::steady_clock;

struct ClientConfig {
  struct sockaddr_in server_address;
  uint16_t port = 0;
  time_t client_timeout = time_t{0};
  ClientMessage message_to_send{};
};

struct ServerConfig {
  std::vector<uint8_t> pawn_row;
  uint8_t max_pawn = 0;
  struct sockaddr_in server_address;
  uint16_t port = 0;
  std::chrono::seconds server_timeout;
};

ClientConfig parse_client_arguments(int argc, char *argv[]);
ServerConfig parse_server_arguments(int argc, char *argv[]);
void print_pawn_row(const std::vector<uint8_t> &pawn_row, uint8_t max_pawn,
                    std::ostream &out = std::cerr);

#endif // MB_PARSING_UTILITY_HPP