#include "input_handler.hpp"

#include <unistd.h>

#include <cerrno>
#include <cstddef>
#include <string_view>

namespace {
constexpr std::size_t BUFFER_SIZE = 4096;
constexpr std::string_view QUIT_COMMAND = "quit\n";
}  // namespace

IOState InputHandler::get_input_state() {
  char buffer[BUFFER_SIZE];
  ssize_t bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE);

  if (bytes_read < 0) {
    if (errno == EAGAIN || errno == EINTR) {
      return IOState::CONTINUE;
    }
    return IOState::ERROR;
  } else if (bytes_read == 0) {
    return IOState::CLOSED_STDIN;
  }

  this->user_input_buffer.append(buffer, static_cast<std::size_t>(bytes_read));

  if (this->user_input_buffer.find(QUIT_COMMAND) == std::string::npos) {
    if (this->user_input_buffer.size() >= 5) {
      this->user_input_buffer.erase(0, this->user_input_buffer.size() - 4);
    }
    return IOState::CONTINUE;
  }
  return IOState::QUIT;
}