#ifndef MB_CONNECTION_UTILITY_HPP
#define MB_CONNECTION_UTILITY_HPP

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <map>
#include <signal.h>
#include <stdlib.h>
#include <utility>

#include "err.hpp"
#include "parsing_utility.hpp"

extern ServerConfig server_config;

constexpr size_t BUFFER_SIZE = 1024;
static constexpr std::array<ssize_t, MESSAGE_TYPE_COUNT> type_to_size = {
    5, 10, 10, 9, 9};

constexpr ssize_t message_size(const MessageType type) {
  return type_to_size[static_cast<ssize_t>(type)];
}

int create_and_bind_server_socket(ServerConfig &config);
int create_and_bind_client_socket(const ClientConfig &config);

void convert_client_msg_to_net_order(ClientMessage &message);
void convert_client_msg_to_host_order(ClientMessage &message);

void convert_game_state_to_net_order(GameState &state);
void convert_game_state_to_host_order(GameState &state);

#endif // MB_CONNECTION_UTILITY_HPP
