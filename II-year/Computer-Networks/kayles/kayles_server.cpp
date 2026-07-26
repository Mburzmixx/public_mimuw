#include "server_utility.hpp"

namespace {
ErrorIndex is_message_valid(ClientMessage &message, ssize_t received_length) {
  if (received_length == 0) {
    return __ErrorIndex::EMPTY_DATAGRAM;
  }

  auto it = type_to_validator.find(message.msg_type);
  if (it != type_to_validator.end()) {
    return it->second(message, received_length);
  }

  return __ErrorIndex::INVALID_MESSAGE_TYPE;
}

void handle_timeouts(Clock::time_point now) {
  archive_timed_out_games(now);
  cleanup_archived_games(now);
}

void handle_valid_message(const int socket_fd, const ClientMessage &message,
                          const struct sockaddr_in &client_address,
                          Clock::time_point receive_time) {
  std::optional<game_id_t> game_id = std::nullopt;

  auto handler_it = type_to_handler.find(message.msg_type);
  if (handler_it == type_to_handler.end()) {
    // Shouldn't happen, because we check it in `is_message_valid()`.
    fatal("Invalid message type.");
  }
  game_id = handler_it->second(message);

  if (game_id.has_value()) {
    // send MSG_GAME_STATE
    update_game_timestamp(game_id.value(), message, receive_time);
    send_msg_game_state(socket_fd, game_id.value(), client_address);
  }
  // In this case, `MSG_JOIN` has been ignored,
  // because it was impossible to create a new game.
}
}; // namespace

bool was_zero_game_id = false;
game_id_t next_game_id = 0;
std::map<game_id_t, GameRecord> games;
std::set<GameTimestampRecord> game_timestamps;
std::set<GameArchiveRecord> archived_games;
ServerConfig server_config{};

int main(int argc, char *argv[]) {
  server_config = parse_server_arguments(argc, argv);
  // Now all the arguments have been parsed and stored.

  // Called function will print an error message and exit if it fails.
  int socket_fd = create_and_bind_server_socket(server_config);

  // This is a diagnostic function.
  print_server_config();

  ssize_t received_length;
  std::array<uint8_t, BUFFER_SIZE> recv_buffer = {0};
  for (;;) {
    const int recv_flags = 0;
    struct sockaddr_in client_address;
    socklen_t address_length = (socklen_t)sizeof(client_address);

    received_length =
        recvfrom(socket_fd, recv_buffer.data(), BUFFER_SIZE, recv_flags,
                 (struct sockaddr *)&client_address, &address_length);

    if (received_length < 0) {
      syserr("recvfrom failed");
    }

    Clock::time_point receive_time = Clock::now();
    handle_timeouts(receive_time);

    ClientMessage client_message{};
    size_t copy_length =
        std::min(static_cast<size_t>(received_length), sizeof(ClientMessage));
    std::memcpy(&client_message, recv_buffer.data(), copy_length);

    convert_client_msg_to_host_order(client_message);
    ErrorIndex error_index = is_message_valid(client_message, received_length);
    if (!error_index.has_value()) {
      handle_valid_message(socket_fd, client_message, client_address,
                           receive_time);
    }
    else {
      std::array<uint8_t, CLIENT_MSG_PREVIEW_SIZE> client_message_preview = {0};
      size_t preview_length = std::min(static_cast<size_t>(received_length),
                                       CLIENT_MSG_PREVIEW_SIZE);
      std::memcpy(client_message_preview.data(), recv_buffer.data(),
                  preview_length);
      send_msg_wrong_msg(socket_fd, client_message_preview, client_address,
                         error_index.value());
    }
  }

  close(socket_fd);
  exit(0);
}
