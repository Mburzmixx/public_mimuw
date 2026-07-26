#include <csignal>
#include <cstdlib>
#include <iostream>

#include "controller.hpp"
#include "logger.hpp"

namespace {
void handle_sigint(int) { sigint_received = 1; }
void setup_signal_handlers() {
  std::signal(SIGINT, handle_sigint);
  std::signal(SIGPIPE, SIG_IGN);
}
}  // namespace

int main(int argc, char *argv[]) {
  setup_signal_handlers();

  RadioConfig config{};
  try {
    config = parse_radio_arguments(argc, argv);
  } catch (const std::invalid_argument &e) {
    std::cerr << "argument parsing error: " << e.what();
    return EXIT_FAILURE;
  }

  Logger::set_verbosity(config.verbosity);
  Controller controller(config);
  return controller.run();
}