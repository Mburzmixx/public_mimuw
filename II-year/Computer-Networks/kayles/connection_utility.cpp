#include "connection_utility.hpp"

int create_and_bind_server_socket(ServerConfig &config) {
  int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (socket_fd < 0) {
    syserr("cannot create a socket");
  }

  socklen_t address_length = (socklen_t) sizeof(config.server_address);
  if (bind(socket_fd, (struct sockaddr *) &config.server_address,
           address_length) < 0) {
    syserr("cannot bind the socket");
  }

  struct sockaddr_in bound_address;
  socklen_t len = sizeof(bound_address);
  if (getsockname(socket_fd, (struct sockaddr *)&bound_address, &len) < 0) {
    syserr("getsockname failed");
  }

  // Find out the assigned port number and update the config.
  uint16_t assigned_port = ntohs(bound_address.sin_port);
  config.port = assigned_port;

  return socket_fd;
}

int create_and_bind_client_socket(const ClientConfig &config) {
  int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (socket_fd < 0) {
    syserr("cannot create a socket");
  }

  struct timeval to = {.tv_sec = config.client_timeout, .tv_usec = 0};
  if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to)) < 0) {
    syserr("cannot set socket options");
  }

  return socket_fd;
}

void convert_client_msg_to_net_order(ClientMessage &message) {
  message.player_id = htonl(message.player_id);
  message.game_id = htonl(message.game_id);
}

void convert_client_msg_to_host_order(ClientMessage &message) {
  message.player_id = ntohl(message.player_id);
  message.game_id = ntohl(message.game_id);
}

void convert_game_state_to_net_order(GameState &state) {
  state.header.game_id = htonl(state.header.game_id);
  state.header.player_a_id = htonl(state.header.player_a_id);
  state.header.player_b_id = htonl(state.header.player_b_id);
}

void convert_game_state_to_host_order(GameState &state) {
  state.header.game_id = ntohl(state.header.game_id);
  state.header.player_a_id = ntohl(state.header.player_a_id);
  state.header.player_b_id = ntohl(state.header.player_b_id);
}
