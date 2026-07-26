#ifndef MB_SERVER_UTILITY_HPP
#define MB_SERVER_UTILITY_HPP

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <set>
#include <utility>

#include "connection_utility.hpp"

using Clock = std::chrono::steady_clock;
using Validator = ErrorIndex (*)(ClientMessage &, ssize_t);
using Handler = std::optional<game_id_t> (*)(const ClientMessage &);

struct PlayerActivity {
  Clock::time_point player_a;
  Clock::time_point player_b = Clock::time_point::max();
  // This means that the player is not participating in the game.
  // In this way game in state `WAITING_FOR_OPPONENT` will be archieved
  // only if player A doesn't make a move.

  Clock::time_point earliest() const { return std::min(player_a, player_b); }

  Clock::time_point latest() const { return std::max(player_a, player_b); }

  bool operator<(const PlayerActivity &other) const {
    if (earliest() == other.earliest()) {
      return player_a < other.player_a;
    }
    return earliest() < other.earliest();
  }

  bool operator==(const PlayerActivity &other) const {
    return earliest() == other.earliest();
  }

  PlayerActivity(Clock::time_point create_time) { player_a = create_time; };

  PlayerActivity() = default;
};

struct GameTimestampRecord {
  game_id_t game_id = 0;
  PlayerActivity last_activity;

  bool operator<(const GameTimestampRecord &other) const {
    if (last_activity == other.last_activity) {
      return game_id < other.game_id;
    }
    return last_activity < other.last_activity;
  }

  bool operator==(const GameTimestampRecord &other) const {
    return last_activity == other.last_activity;
  }

  GameStatus status_after_timeout() const {
    if (last_activity.player_a < last_activity.player_b) {
      return GameStatus::WIN_B;
    }
    return GameStatus::WIN_A;
  }

  Clock::time_point latest() const { return last_activity.latest(); }

  Clock::time_point earliest() const { return last_activity.earliest(); }
};

struct GameArchiveRecord {
  game_id_t game_id;
  Clock::time_point delete_time;

  bool operator<(const GameArchiveRecord &other) const {
    if (delete_time == other.delete_time) {
      return game_id < other.game_id;
    }
    return delete_time < other.delete_time;
  }

  bool operator==(const GameArchiveRecord &other) const {
    return delete_time == other.delete_time;
  }
};

struct GameRecord {
  GameState state;
  GameArchiveRecord archive_record;
  GameTimestampRecord timestamp_record;

  bool is_player_participating(const player_id_t &player) const {
    return (state.header.player_a_id == player) ||
           (state.header.player_b_id == player);
  }

  bool can_player_move(const player_id_t &player) const {
    if (!is_player_participating(player)) {
      return false;
    }

    if (state.header.status == GameStatus::TURN_A) {
      return player == state.header.player_a_id;
    }
    else if (state.header.status == GameStatus::TURN_B) {
      return player == state.header.player_b_id;
    }
    return false;
  }

  bool is_pawn_position_valid(const pawn_index_t pawn) const {
    if (pawn > state.header.max_pawn) {
      return false;
    }
    // Here `pawn / 8` is the index of the byte in `pawn_row`,
    //  and `7 - pawn % 8` is the index of the bit in that byte.
    // In this case pawn with number `0` corresponds
    //  to the most significant bit of `pawn_row[0]`,
    uint8_t _pawn = state.pawn_row[pawn / 8] & (1 << (7 - (pawn % 8)));

    if (_pawn == 0) {
      return false;
    }
    return true;
  }

  void knock_pawn(const pawn_index_t pawn) {
    state.pawn_row[pawn / 8] &= ~(1 << (7 - (pawn % 8)));
  }

  bool has_any_pawn_left() const {
    return std::any_of(state.pawn_row.begin(), state.pawn_row.end(),
                       [](uint8_t byte) { return byte != 0; });
  }

  GameStatus status_after_move() {
    if (state.header.status != GameStatus::TURN_A &&
        state.header.status != GameStatus::TURN_B) {
      return state.header.status;
    }

    if (!has_any_pawn_left()) {
      if (state.header.status == GameStatus::TURN_A) {
        return GameStatus::WIN_A;
      }
      else {
        return GameStatus::WIN_B;
      }
    }
    else if (state.header.status == GameStatus::TURN_A) {
      return GameStatus::TURN_B;
    }
    else if (state.header.status == GameStatus::TURN_B) {
      return GameStatus::TURN_A;
    }

    return state.header.status;
  }

  void set_new_status(GameStatus new_status) {
    state.header.status = new_status;
  }

  GameStatus status_after_give_up() {
    if (state.header.status == GameStatus::TURN_A) {
      return GameStatus::WIN_B;
    }
    else if (state.header.status == GameStatus::TURN_B) {
      return GameStatus::WIN_A;
    }

    return state.header.status;
  }
};

extern bool was_zero_game_id;
extern game_id_t next_game_id;
extern std::map<game_id_t, GameRecord> games;
extern std::set<GameTimestampRecord> game_timestamps;
extern std::set<GameArchiveRecord> archived_games;
extern ServerConfig server_config;

extern const std::map<MessageType, Validator> type_to_validator;
extern const std::map<MessageType, Handler> type_to_handler;

void print_server_config();

void send_msg_game_state(const int socket_fd, const game_id_t &game_id,
                         const struct sockaddr_in &client_address);
void send_msg_wrong_msg(
    const int socket_fd,
    std::array<uint8_t, CLIENT_MSG_PREVIEW_SIZE> &client_message,
    const struct sockaddr_in &client_address, const __ErrorIndex error_index);

ErrorIndex validate_msg_join(ClientMessage &message, ssize_t received_length);
ErrorIndex validate_msg_move(ClientMessage &message, ssize_t received_length);
ErrorIndex validate_msg_keep_alive(ClientMessage &message,
                                   ssize_t received_length);
ErrorIndex validate_msg_give_up(ClientMessage &message,
                                ssize_t received_length);

std::optional<game_id_t> handle_msg_join(const ClientMessage &message);
std::optional<game_id_t> handle_msg_move(const ClientMessage &message);
std::optional<game_id_t> handle_msg_keep_alive(const ClientMessage &message);
std::optional<game_id_t> handle_msg_give_up(const ClientMessage &message);

void archive_timed_out_games(Clock::time_point now);
void cleanup_archived_games(Clock::time_point now);

void update_game_timestamp(const game_id_t game_id,
                           const ClientMessage &message,
                           Clock::time_point receive_time);

#endif // MB_SERVER_UTILITY_HPP
