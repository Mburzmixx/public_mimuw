#include "server_utility.hpp"

const std::map<MessageType, Validator> type_to_validator = {
    {MessageType::MSG_JOIN, &validate_msg_join},
    {MessageType::MSG_MOVE_1, &validate_msg_move},
    {MessageType::MSG_MOVE_2, &validate_msg_move},
    {MessageType::MSG_KEEP_ALIVE, &validate_msg_keep_alive},
    {MessageType::MSG_GIVE_UP, &validate_msg_give_up}};

const std::map<MessageType, Handler> type_to_handler = {
    {MessageType::MSG_JOIN, &handle_msg_join},
    {MessageType::MSG_MOVE_1, &handle_msg_move},
    {MessageType::MSG_MOVE_2, &handle_msg_move},
    {MessageType::MSG_KEEP_ALIVE, &handle_msg_keep_alive},
    {MessageType::MSG_GIVE_UP, &handle_msg_give_up}};

namespace {
void end_game_due_to_timeout(const game_id_t game_id) {
  auto game_it = games.find(game_id);
  if (game_it == games.end()) {
    // Shouldn't happen, because we check it in `archive_timed_out_games()`.
    fatal("Game not found.");
  }

  // I delete the game from `game_timestamps` in `archive_timed_out_games()`,
  // so I don't have to do it here.
  GameRecord &game_record = game_it->second;
  if (game_record.state.header.status == GameStatus::WAITING_FOR_OPPONENT) {
    // In this case, the game will be deleted.
    games.erase(game_it);
  }
  else {
    GameStatus end_status = game_record.timestamp_record.status_after_timeout();
    game_record.set_new_status(end_status);
    game_record.archive_record = GameArchiveRecord{
        .game_id = game_id,
        .delete_time = game_record.timestamp_record.last_activity.latest() +
                       server_config.server_timeout};

    archived_games.insert(game_record.archive_record);
  }
}

void delete_game(const game_id_t game_id) {
  auto game_it = games.find(game_id);
  if (game_it == games.end()) {
    // Shouldn't happen, because we check it in `cleanup_archived_games()`.
    fatal("Game not found.");
  }

  // I delete the game from `archived_games` in `cleanup_archived_games()`,
  // so I don't have to do it here.
  games.erase(game_it);
}

void archive_game(const game_id_t game_id) {
  auto game_it = games.find(game_id);
  if (game_it == games.end()) {
    // Shouldn't happen, because we check it in `handle_msg_move()`.
    fatal("Game not found.");
  }

  GameRecord &game_record = game_it->second;
  game_record.archive_record = GameArchiveRecord{
      .game_id = game_id,
      .delete_time = Clock::now() + server_config.server_timeout};

  game_timestamps.erase(game_record.timestamp_record);
  archived_games.insert(game_record.archive_record);
}

std::optional<game_id_t> create_new_game(const player_id_t player_id) {
  if (next_game_id == 0) {
    if (was_zero_game_id) {
      // This means that we have already created `UINT32_MAX` games,
      // so we cannot create a new game.
      return std::nullopt;
    }
    was_zero_game_id = true;
  }

  GameHeader header;
  header.game_id = next_game_id;
  header.player_a_id = player_id;
  header.status = GameStatus::WAITING_FOR_OPPONENT;
  header.max_pawn = server_config.max_pawn;

  const game_id_t game_id = header.game_id;

  GameState state;
  state.header = header;
  state.pawn_row = server_config.pawn_row;

  PlayerActivity player_activity(Clock::now());

  GameTimestampRecord timestamp_record;
  timestamp_record.game_id = header.game_id;
  timestamp_record.last_activity = player_activity;

  GameRecord new_game_record;
  new_game_record.state = state;
  new_game_record.timestamp_record = timestamp_record;

  try {
    game_timestamps.insert(new_game_record.timestamp_record);
    games.insert({game_id, std::move(new_game_record)});
  } catch (const std::exception &e) {
    // If we cannot create a new game,
    // we return `std::nullopt` and ignore the `MSG_JOIN`.
    game_timestamps.erase(timestamp_record);
    games.erase(game_id);
    return std::nullopt;
  }

  next_game_id++;
  return game_id;
}
}; // namespace

void print_server_config() {
  std::cerr << "Server configuration:\n";
  std::cerr << "\tServer address: "
            << inet_ntoa(server_config.server_address.sin_addr) << "\n";
  std::cerr << "\tPort: " << server_config.port << "\n";
  std::cerr << "\tServer Timeout: " << server_config.server_timeout.count()
            << " seconds\n";
  std::cerr << "\tMax Pawn Position: "
            << static_cast<uint32_t>(server_config.max_pawn) << "\n";
  std::cerr << "\tPawn Row: ";
  print_pawn_row(server_config.pawn_row, server_config.max_pawn, std::cerr);
  std::cerr.flush();
}

void send_msg_wrong_msg(
    const int socket_fd,
    std::array<uint8_t, CLIENT_MSG_PREVIEW_SIZE> &client_message,
    const struct sockaddr_in &client_address, __ErrorIndex error_index) {
  WrongMessage response{};

  std::memcpy(response.client_msg_preview, &client_message,
              CLIENT_MSG_PREVIEW_SIZE);
  // respose.status is set to 255 by default
  response.error_index = error_index;

  const int send_flags = 0;
  size_t response_size = sizeof(response);
  socklen_t address_length = (socklen_t)sizeof(client_address);
  ssize_t sent_length =
      sendto(socket_fd, &response, response_size, send_flags,
             (struct sockaddr *)&client_address, address_length);

  if (sent_length < 0) {
    syserr("Sendto failed.");
  }
  else if (sent_length != static_cast<ssize_t>(response_size)) {
    fatal("Incomplete sending.");
  }
}

void send_msg_game_state(const int socket_fd, const game_id_t &game_id,
                         const struct sockaddr_in &client_address) {
  auto game_it = games.find(game_id);
  if (game_it == games.end()) {
    // Shouldn't happen, because we check it in `is_message_valid()`.
    fatal("Game not found.");
  }

  GameState state = game_it->second.state;
  convert_game_state_to_net_order(state);
  GameHeader header = state.header;

  static std::array<uint8_t, BUFFER_SIZE> buffer = {0};
  std::memcpy(buffer.data(), &header, sizeof(header));
  std::memcpy(buffer.data() + sizeof(header), state.pawn_row.data(),
              state.pawn_row.size());

  const int send_flags = 0;
  size_t message_length = sizeof(header) + state.pawn_row.size();
  if (message_length > BUFFER_SIZE) {
    // Shouldn't happen.
    fatal("Message is too large to send.");
  }

  socklen_t address_length = (socklen_t)sizeof(client_address);
  ssize_t sent_length =
      sendto(socket_fd, buffer.data(), message_length, send_flags,
             (struct sockaddr *)&client_address, address_length);

  if (sent_length < 0) {
    syserr("Sendto failed.");
  }
}

ErrorIndex validate_msg_join(ClientMessage &message, ssize_t received_length) {
  if (received_length > message_size(MessageType::MSG_JOIN)) {
    return __ErrorIndex::JOIN_SIZE_TOO_LONG;
  }

  if (received_length < message_size(MessageType::MSG_JOIN)) {
    // This means client sent a message that is too short.
    // It is the correct byte, since they are numbered from 0.
    return static_cast<__ErrorIndex>(received_length);
  }

  if (message.player_id == 0) {
    return __ErrorIndex::INVALID_PLAYER_ID;
  }
  // These fields will not be used.
  // I make them equal to ZERO to standardize the message.
  message.game_id = 0;
  message.pawn = 0;

  return std::nullopt;
}

ErrorIndex validate_msg_move(ClientMessage &message, ssize_t received_length) {
  if (received_length > message_size(MessageType::MSG_MOVE_1)) {
    return __ErrorIndex::MOVE_SIZE_TOO_LONG;
  }

  if (received_length < message_size(MessageType::MSG_MOVE_1)) {
    // This means client sent a message that is too short.
    return static_cast<__ErrorIndex>(received_length);
  }

  auto game_it = games.find(message.game_id);
  if (game_it == games.end()) {
    return __ErrorIndex::INVALID_GAME_ID;
  }

  if (!game_it->second.is_player_participating(message.player_id)) {
    return __ErrorIndex::INVALID_PLAYER_ID;
  }

  return std::nullopt;
}

ErrorIndex validate_msg_keep_alive(ClientMessage &message,
                                   ssize_t received_length) {
  if (received_length > message_size(MessageType::MSG_KEEP_ALIVE)) {
    return __ErrorIndex::KEEP_ALIVE_SIZE_TOO_LONG;
  }

  if (received_length < message_size(MessageType::MSG_KEEP_ALIVE)) {
    // This means client sent a message that is too short.
    return static_cast<__ErrorIndex>(received_length);
  }

  auto game_it = games.find(message.game_id);
  if (game_it == games.end()) {
    return __ErrorIndex::INVALID_GAME_ID;
  }

  if (!game_it->second.is_player_participating(message.player_id)) {
    return __ErrorIndex::INVALID_PLAYER_ID;
  }
  // These fields will not be used.
  // I make them equal to ZERO to standardize the message.
  message.pawn = 0;

  return std::nullopt;
}

ErrorIndex validate_msg_give_up(ClientMessage &message,
                                ssize_t received_length) {
  if (received_length > message_size(MessageType::MSG_GIVE_UP)) {
    return __ErrorIndex::GIVE_UP_SIZE_TOO_LONG;
  }

  if (received_length < message_size(MessageType::MSG_GIVE_UP)) {
    // This means client sent a message that is too short.
    return static_cast<__ErrorIndex>(received_length);
  }

  auto game_it = games.find(message.game_id);
  if (game_it == games.end()) {
    return __ErrorIndex::INVALID_GAME_ID;
  }

  if (!game_it->second.is_player_participating(message.player_id)) {
    return __ErrorIndex::INVALID_PLAYER_ID;
  }
  // These fields will not be used.
  // I make them equal to ZERO to standardize the message.
  message.pawn = 0;

  return std::nullopt;
}

// Below are functions related to game logic.

std::optional<game_id_t> handle_msg_join(const ClientMessage &message) {
  auto game_it = games.find(next_game_id - 1);
  if (game_it != games.end()) {
    // Game with `game_id` equal to `next_game_id - 1` is the only game
    // that can have status `WAITING_FOR_OPPONENT`.
    GameRecord &game_record = game_it->second;

    if (game_record.state.header.status == GameStatus::WAITING_FOR_OPPONENT) {
      game_record.state.header.player_b_id = message.player_id;
      game_record.state.header.status = GameStatus::TURN_B;
      game_id_t game_id = game_record.state.header.game_id;
      return game_id;
    }
  }
  // Here we are sure that there is no game with status `WAITING_FOR_OPPONENT`,
  // so we should create a new game.
  return create_new_game(message.player_id);
}

std::optional<game_id_t> handle_msg_move(const ClientMessage &message) {
  pawn_index_t pawn = message.pawn;
  player_id_t player_id = message.player_id;
  game_id_t game_id = message.game_id;

  auto game_it = games.find(game_id);
  if (game_it == games.end()) {
    // Shouldn't happen, because we check it in `is_message_valid()`.
    fatal("Game not found.");
  }

  GameRecord &game_record = game_it->second;

  if (!game_record.can_player_move(player_id)) {
    return game_id;
  }

  if (message.msg_type == MessageType::MSG_MOVE_1) {
    if (!game_record.is_pawn_position_valid(pawn)) {
      return game_id;
    }

    game_record.knock_pawn(pawn);
  }
  else {
    // Here we have `MessageType::MSG_MOVE_2`.
    if (pawn >= game_record.state.header.max_pawn) {
      return game_id;
    }

    const pawn_index_t second_pawn = pawn + 1;
    if (!game_record.is_pawn_position_valid(pawn) ||
        !game_record.is_pawn_position_valid(second_pawn)) {
      return game_id;
    }

    game_record.knock_pawn(pawn);
    game_record.knock_pawn(second_pawn);
  }

  GameStatus new_status = game_record.status_after_move();
  game_record.set_new_status(new_status);
  if (new_status == GameStatus::WIN_A || new_status == GameStatus::WIN_B) {
    archive_game(game_id);
  }

  return game_id;
}

std::optional<game_id_t> handle_msg_keep_alive(const ClientMessage &message) {
  // Here we don't have to do anything, because we will update the timestamp
  // of the game in `handle_valid_message()`, which is called after this
  // function.
  auto game_it = games.find(message.game_id);
  if (game_it == games.end()) {
    // Shouldn't happen, because we check it in `is_message_valid()`.
    fatal("Game not found.");
  }
  return game_it->first;
}

std::optional<game_id_t> handle_msg_give_up(const ClientMessage &message) {
  auto game_it = games.find(message.game_id);
  if (game_it == games.end()) {
    // Shouldn't happen, because we check it in `is_message_valid()`.
    fatal("Game not found.");
  }

  if (!game_it->second.is_player_participating(message.player_id) ||
      !game_it->second.can_player_move(message.player_id)) {
    return message.game_id;
  }

  GameRecord &game_record = game_it->second;
  GameStatus new_status = game_record.status_after_give_up();
  game_record.set_new_status(new_status);
  archive_game(message.game_id);

  return message.game_id;
}

void archive_timed_out_games(Clock::time_point now) {
  if (game_timestamps.empty()) {
    return;
  }

  for (auto it = game_timestamps.begin(); it != game_timestamps.end();) {
    if (now - it->last_activity.earliest() > server_config.server_timeout) {
      end_game_due_to_timeout(it->game_id);
      it = game_timestamps.erase(it);
    }
    else {
      // Since the games are sorted by time, we can end here.
      break;
    }
  }
}

void cleanup_archived_games(Clock::time_point now) {
  if (archived_games.empty()) {
    return;
  }

  for (auto it = archived_games.begin(); it != archived_games.end();) {
    if (now > it->delete_time) {
      delete_game(it->game_id);
      it = archived_games.erase(it);
    }
    else {
      // Since the games are sorted by delete time, we can end here.
      break;
    }
  }
}

void update_game_timestamp(const game_id_t game_id,
                           const ClientMessage &message,
                           Clock::time_point receive_time) {
  auto game_it = games.find(game_id);
  if (game_it == games.end() ||
      !game_it->second.is_player_participating(message.player_id)) {
    // Shouldn't happen, because we check it in `is_message_valid()`.
    fatal("Game not found.");
  }

  GameRecord &game_record = game_it->second;
  if (game_timestamps.find(game_record.timestamp_record) ==
      game_timestamps.end()) {
    // This means that the game has been archived, so we don't update its
    // timestamp.
    return;
  }
  game_timestamps.erase(game_record.timestamp_record);

  if (message.player_id == game_record.state.header.player_a_id) {
    game_record.timestamp_record.last_activity.player_a = receive_time;
  }

  if (message.player_id == game_record.state.header.player_b_id) {
    game_record.timestamp_record.last_activity.player_b = receive_time;
  }

  game_timestamps.insert(game_record.timestamp_record);
}
