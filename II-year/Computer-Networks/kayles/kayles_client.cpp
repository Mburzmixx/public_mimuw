#include "connection_utility.hpp"
#include <iostream>

namespace {
bool is_status_type_valid(const uint8_t msg_type) {
  return MIN_MESSAGE_TYPE <= msg_type && msg_type <= MAX_MESSAGE_TYPE;
}

bool parse_server_datagram(const std::array<uint8_t, BUFFER_SIZE> &buffer,
                           ssize_t length, ServerMessage &message) {
  if (length < SERVER_MESSAGE_HEADER_SIZE) {
    std::cerr << "Received message is too short. Length: " << length << "\n";
    return false;
  }

  if (buffer[STATUS_POSITION] == MSG_WRONG_MSG_STATUS) {
    WrongMessage wrong_message{};
    std::memcpy(&wrong_message, buffer.data(), sizeof(wrong_message));
    message = wrong_message;
    return true;
  }
  else if (is_status_type_valid(buffer[STATUS_POSITION])) {
    pawn_index_t max_pawn = buffer[MAX_PAWN_POSITION];
    size_t pawn_row_length = (max_pawn / 8) + 1;
    ssize_t expected_length = SERVER_MESSAGE_HEADER_SIZE + pawn_row_length;

    if (length != expected_length) {
      std::cerr << "Invalid message length. Expected: " << expected_length
                << ", Actual: " << length << "\n";
      return false;
    }

    GameHeader header{};
    size_t header_size = sizeof(header);
    std::memcpy(&header, buffer.data(), header_size);

    GameState game_state;
    game_state.header = header;
    game_state.pawn_row.resize(pawn_row_length);

    std::memcpy(game_state.pawn_row.data(), buffer.data() + header_size,
                pawn_row_length);
    convert_game_state_to_host_order(game_state);
    message = game_state;
    return true;
  }
  std::cerr << "Invalid status value in the received message: "
            << static_cast<uint32_t>(buffer[STATUS_POSITION]) << "\n";
  return false;
}

void print_clinet_config(const ClientConfig &config) {
  std::cerr << "Client configuration:\n";
  std::cerr << "\tServer address: " << inet_ntoa(config.server_address.sin_addr)
            << "\n";
  std::cerr << "\tServer Port: " << ntohs(config.server_address.sin_port)
            << "\n";
  std::cerr << "\tClient Timeout: " << config.client_timeout << " seconds\n\n";
  std::cerr.flush();
}

void print_game_state(const GameState &state) {
  std::cout << "Received game state!\n";
  std::cout << "\tGame ID: " << state.header.game_id << "\n";
  std::cout << "\tPlayer A ID: " << state.header.player_a_id << "\n";
  std::cout << "\tPlayer B ID: " << state.header.player_b_id << "\n";
  std::cout << "\tGame Status: " << static_cast<uint32_t>(state.header.status)
            << "\n";
  std::cout << "\tMax Pawn Position: "
            << static_cast<uint32_t>(state.header.max_pawn) << "\n";
  std::cout << "\tPawn Row: ";
  print_pawn_row(state.pawn_row, state.header.max_pawn, std::cout);
  std::cout.flush();
}

void print_wrong_message(const WrongMessage &wrong_message) {
  std::cout << "Received wrong message!\n";
  std::cout << "\tClient Message Preview: ";
  for (const uint8_t &byte : wrong_message.client_msg_preview) {
    std::cout << static_cast<int>(byte) << " ";
  }
  std::cout << "\n";
  std::cout << "\tError Status: " << static_cast<uint32_t>(wrong_message.status)
            << "\n";
  std::cout << "\tError Index: "
            << static_cast<uint32_t>(wrong_message.error_index) << "\n";
  std::cout.flush();
}

void print_received_message(const ServerMessage &message) {
  if (std::holds_alternative<GameState>(message)) {
    print_game_state(std::get<GameState>(message));
  }
  else if (std::holds_alternative<WrongMessage>(message)) {
    print_wrong_message(std::get<WrongMessage>(message));
  }
}
}; // namespace

int main(int argc, char *argv[]) {
  ClientConfig config = parse_client_arguments(argc, argv);

  // Called function will print an error message and exit if it fails.
  int socket_fd = create_and_bind_client_socket(config);

  // This is a diagnostic function.
  print_clinet_config(config);

  ssize_t message_length = message_size(config.message_to_send.msg_type);

  convert_client_msg_to_net_order(config.message_to_send);

  const int send_flags = 0;
  socklen_t address_length = (socklen_t)sizeof(config.server_address);

  ssize_t sent_length =
      sendto(socket_fd, &config.message_to_send, message_length, send_flags,
             (struct sockaddr *)&config.server_address, address_length);

  if (sent_length < 0) {
    syserr("send failed");
  }
  else if (sent_length != message_length) {
    fatal("Incomplete sending.");
  }

  // Sending completed, now wait for the response.

  const int recv_flags = 0;
  std::array<uint8_t, BUFFER_SIZE> buffer = {0};
  struct sockaddr_in server_address_recv;
  socklen_t address_length_recv = (socklen_t)sizeof(server_address_recv);

  ssize_t received_length =
      recvfrom(socket_fd, &buffer, BUFFER_SIZE, recv_flags,
               (struct sockaddr *)&server_address_recv, &address_length_recv);

  if (received_length < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      std::cout << "No response received within the provided timeout period."
                << std::endl;
      exit(0);
    }
    else {
      syserr("recvfrom failed");
    }
  }

  ServerMessage received_message;
  if (!parse_server_datagram(buffer, received_length, received_message)) {
    fatal("Received an invalid message from the server.");
  }
  print_received_message(received_message);

  close(socket_fd);
  exit(0);
}
