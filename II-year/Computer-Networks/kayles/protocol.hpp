#ifndef MB_PROTOCOL_HPP
#define MB_PROTOCOL_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

using game_id_t = uint32_t;
using player_id_t = uint32_t;
using pawn_index_t = uint8_t;

enum class GameStatus : uint8_t {
  WAITING_FOR_OPPONENT      = (0),
  TURN_A                    = (1),
  TURN_B                    = (2),
  WIN_A                     = (3),
  WIN_B                     = (4)
};

enum class MessageType : uint8_t {
  MSG_JOIN                  = (0),
  MSG_MOVE_1                = (1),
  MSG_MOVE_2                = (2),
  MSG_KEEP_ALIVE            = (3),
  MSG_GIVE_UP               = (4)
};
constexpr uint8_t MIN_MESSAGE_TYPE = 0;
constexpr uint8_t MAX_MESSAGE_TYPE = 4;
constexpr size_t MESSAGE_TYPE_COUNT = 5;

enum class __ErrorIndex : uint8_t {
  INVALID_MESSAGE_TYPE      = (0),
  INVALID_PLAYER_ID         = (1),
  INVALID_GAME_ID           = (5),
  INVALID_PAWN              = (9),
  JOIN_SIZE_TOO_LONG        = (5),
  MOVE_SIZE_TOO_LONG        = (10),
  KEEP_ALIVE_SIZE_TOO_LONG  = (9),
  GIVE_UP_SIZE_TOO_LONG     = (9),
  EMPTY_DATAGRAM            = (0)
};

using ErrorIndex = std::optional<__ErrorIndex>;

struct __attribute__((__packed__)) GameHeader {
  game_id_t game_id;

  player_id_t player_a_id;
  player_id_t player_b_id = 0;

  GameStatus status;
  pawn_index_t max_pawn;
};

constexpr uint8_t STATUS_POSITION = 12;
constexpr uint8_t MAX_PAWN_POSITION = 13;

struct GameState {
  GameHeader header;
  std::vector<uint8_t> pawn_row;
};

constexpr size_t CLIENT_MSG_PREVIEW_SIZE = 12;

struct __attribute__((__packed__)) WrongMessage {
  // Prefix of the message that caused the error.
  //                (first CLIENT_MSG_PREVIEW_SIZE bytes)
  // If the message is shorter than CLIENT_MSG_PREVIEW_SIZE,
  // the remaining bytes are filled with zeros.
  uint8_t client_msg_preview[CLIENT_MSG_PREVIEW_SIZE] = {0};
  uint8_t status = 255; // This value should not be modified.

  // Index of the byte that caused the error. Numbering starts from 0.
  __ErrorIndex error_index;
};

using ServerMessage = std::variant<GameState, WrongMessage>;
// We have sizeof(WrongMessage) = sizeof(GameHeader), so the following is true:
constexpr ssize_t SERVER_MESSAGE_HEADER_SIZE = sizeof(WrongMessage);
constexpr uint8_t MSG_WRONG_MSG_STATUS = 255;

struct __attribute__((__packed__)) ClientMessage {
  MessageType msg_type = MessageType::MSG_JOIN;

  player_id_t player_id = 0;
  game_id_t game_id = 0;

  uint8_t pawn = 0;
};

#endif // MB_PROTOCOL_HPP
