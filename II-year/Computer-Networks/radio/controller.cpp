#include "controller.hpp"

#include <unistd.h>

#include <stdexcept>

#include "connection.hpp"
#include "logger.hpp"

namespace {
constexpr std::size_t SERVER_FD_INDEX = 0;
constexpr std::size_t STDIN_FD_INDEX = 1;
constexpr std::size_t NUM_DESCRIPTORS = 2;
}  // namespace

Controller::Controller(const RadioConfig &config) : config(config) {
  this->poll_descriptors[SERVER_FD_INDEX].events = POLLIN;
  this->poll_descriptors[STDIN_FD_INDEX].fd = STDIN_FILENO;
  this->poll_descriptors[STDIN_FD_INDEX].events = POLLIN;
}

int Controller::run() noexcept {
  client = std::make_unique<HttpClient>(config);
  input_handler = std::make_unique<InputHandler>();

  Logger::log(LogLevel::DEBUG, "starting main loop");

  do {
    try {
      run_event_loop();
    } catch (const std::exception &e) {
      close_connection();

      // This should be `std::runtime_error`,
      // but I want to cover some other exceptions like `bad_alloc`.
      // These are fatal errors, so we log them and exit.
      Logger::log(LogLevel::FATAL, e.what());
      return EXIT_FAILURE;
    }
  } while (!finish);

  close_connection();
  return EXIT_SUCCESS;
}

void Controller::flush_input_buffer() {
  if (client->is_connected()) {
    Logger::log(LogLevel::DEBUG, "flushing input buffer");
    client->flush_in_buffer([this](const char *buffer, std::size_t length) {
      this->stream_processor->process_data(buffer, length);
    });
  } else {
    Logger::log(LogLevel::DEBUG, "not connected, no input buffer to flush");
  }
}

void Controller::establish_connection() {
  stream_processor = std::make_unique<StreamProcessor>();
  client->establish_connection();

  if (client->get_icy_metaint() > 0) {
    stream_processor->enable_multiplexing(client->get_icy_metaint());
  }
  if (client->is_transfer_chunked()) {
    stream_processor->enable_chunking_transfer();
  }

  Logger::log(LogLevel::DEBUG, "connection established successfully!");
}

void Controller::close_connection() {
  if (!client->is_connected()) {
    return;
  }

  flush_input_buffer();
  Logger::log(LogLevel::DEBUG, "closing connection");
  client->disconnect();
  stream_processor.reset(nullptr);
  return;
}

void Controller::handle_server_response() {
  char buffer[BUFFER_SIZE];
  ssize_t bytes_read = client->read_stream_data(buffer, BUFFER_SIZE);

  if (bytes_read < 0) {
    // Error while reading from server.
    throw std::runtime_error("failed to read from server");
  } else if (bytes_read == 0) {
    // Connection closed by server.
    Logger::log(LogLevel::DEBUG, "server closed connection");
    finish = true;
    return;
  }

  bool should_end = stream_processor->process_data(
      buffer, static_cast<std::size_t>(bytes_read));
  if (!should_end) {
    Logger::log(LogLevel::DEBUG,
                "server closed connection (chunked transfer encoding)");
    finish = true;
  }
}

void Controller::handle_user_input() {
  IOState input_state = input_handler->get_input_state();
  switch (input_state) {
    case IOState::QUIT:
      Logger::log(LogLevel::DEBUG, "quit command received, exiting");
      finish = true;
      break;
    case IOState::CLOSED_STDIN:
      Logger::log(LogLevel::NON_FATAL, "stdin closed");
      poll_descriptors[STDIN_FD_INDEX].fd = -1;
      break;
    case IOState::ERROR:
      Logger::log(LogLevel::NON_FATAL, "error while reading user input");
      poll_descriptors[STDIN_FD_INDEX].fd = -1;
      break;
    case IOState::CONTINUE:
      // No complete command received, continue waiting for input.
      break;
  }
}

void Controller::handle_client_input_error() {
  Logger::log(LogLevel::NON_FATAL, "stdin error");
  poll_descriptors[STDIN_FD_INDEX].fd = -1;
}

void Controller::run_event_loop() {
  if (!client->is_connected()) {
    Logger::log(LogLevel::DEBUG, "not connected, establishing connection");
    establish_connection();
    poll_descriptors[SERVER_FD_INDEX].fd = client->get_server_fd();
  }

  for (std::size_t i = 0; i < NUM_DESCRIPTORS; ++i) {
    poll_descriptors[i].revents = 0;
  }

  int poll_status = poll(poll_descriptors, NUM_DESCRIPTORS, config.timeout);
  if (poll_status < 0) {
    if (errno == EINTR) {
      Logger::log(LogLevel::INFO, "poll interrupted by signal");
    } else {
      throw std::runtime_error("poll failed");
    }
  } else if (poll_status == 0) {
    Logger::log(LogLevel::INFO, "data receiving timeout");
    close_connection();
  } else if (poll_status > 0) {
    if (poll_descriptors[SERVER_FD_INDEX].revents & POLLIN) {
      do {
        handle_server_response();
      } while (client->has_pending_data());
    }
    if (poll_descriptors[SERVER_FD_INDEX].revents & (POLLERR | POLLNVAL)) {
      throw std::runtime_error("server connection error, closing connection");
    }
    if (poll_descriptors[SERVER_FD_INDEX].revents & POLLHUP) {
      Logger::log(LogLevel::DEBUG, "server closed connection");
      finish = true;
    }

    if (poll_descriptors[STDIN_FD_INDEX].revents & POLLIN) {
      handle_user_input();
    }
    if (poll_descriptors[STDIN_FD_INDEX].revents &
        (POLLERR | POLLHUP | POLLNVAL)) {
      handle_client_input_error();
    }
  }

  if (sigint_received) {
    Logger::log(LogLevel::INFO, "SIGINT received, exiting");
    finish = true;
  }
}