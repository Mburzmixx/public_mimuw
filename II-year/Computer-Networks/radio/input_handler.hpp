#ifndef MB_INPUT_HANDLER_HPP
#define MB_INPUT_HANDLER_HPP

#include <string>

enum class IOState { CONTINUE, QUIT, CLOSED_STDIN, ERROR };

class InputHandler {
 private:
  std::string user_input_buffer = "";

 public:
  InputHandler() = default;
  ~InputHandler() = default;

  IOState get_input_state();
};

#endif  // MB_INPUT_HANDLER_HPP