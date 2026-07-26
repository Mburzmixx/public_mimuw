#ifndef MB_CONTROLLER_HPP
#define MB_CONTROLLER_HPP

#include <poll.h>

#include <csignal>
#include <memory>

#include "arg_parsing.hpp"
#include "http_client.hpp"
#include "input_handler.hpp"
#include "stream_processor.hpp"

inline volatile std::sig_atomic_t sigint_received = 0;

class Controller {
 public:
  explicit Controller(const RadioConfig &config);
  ~Controller() = default;

  int run() noexcept;

 private:
  RadioConfig config;
  bool finish = false;
  struct pollfd poll_descriptors[2];

  std::unique_ptr<HttpClient> client = nullptr;
  std::unique_ptr<StreamProcessor> stream_processor = nullptr;
  std::unique_ptr<InputHandler> input_handler = nullptr;

  void flush_input_buffer();

  void establish_connection();
  void close_connection();

  void handle_server_response();
  void handle_user_input();
  void handle_client_input_error();

  void run_event_loop();
};

#endif  // MB_CONTROLLER_HPP