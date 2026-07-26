#include "http_client.hpp"

#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>

#include "logger.hpp"

namespace {
constexpr int OK_STATUS_CODE = 200;
constexpr int MIN_INFO_STATUS_CODE = 100;
constexpr std::size_t MAX_INFO_RESPONSES = 20;
const std::set<int> REDIRECTION_STATUS_CODES = {300, 301, 302, 303, 307, 308};
constexpr std::string_view DATE_FORMAT = "{:%Y.%m.%d %H.%M.%S}";
constexpr std::string DEFAULT_PATH = "/";

bool is_informational_status_code(int status_code) {
  return MIN_INFO_STATUS_CODE <= status_code && status_code < OK_STATUS_CODE;
}

std::optional<std::string> addrinfo_to_string(const addrinfo *info) {
  char host[NI_MAXHOST];
  char service[NI_MAXSERV];

  int errcode =
      getnameinfo(info->ai_addr, info->ai_addrlen, host, sizeof(host), service,
                  sizeof(service), NI_NUMERICHOST | NI_NUMERICSERV);
  if (errcode != 0) {
    return std::nullopt;
  }

  if (info->ai_family == AF_INET6) {
    return std::string("[") + host + "]:" + std::string(service);
  }
  return std::string(host) + ":" + std::string(service);
}

void divide_cookie_string_into_parts(std::string cookie_string,
                                     std::vector<std::string> &result) {
  std::stringstream ss(cookie_string);
  std::string part;

  while (getline(ss, part, ';')) {
    // Remove leading and trailing whitespace from the part.
    part.erase(0, part.find_first_not_of(" \t"));
    part.erase(part.find_last_not_of(" \t") + 1);

    if (!part.empty()) {
      result.emplace_back(part);
    }
  }
}
}  // namespace

Cookie::Cookie(std::string cookie_string, ParsedURL current_url) {
  set_domain(current_url.host);
  std::string default_path = DEFAULT_PATH;
  std::size_t last_slash_pos = current_url.path.find_last_of('/');
  if (last_slash_pos != std::string::npos && last_slash_pos > 0) {
    default_path = current_url.path.substr(0, last_slash_pos);
  }
  set_path(default_path);

  std::vector<std::string> parts;
  divide_cookie_string_into_parts(cookie_string, parts);
  if (parts.empty()) {
    throw std::invalid_argument("invalid cookie string: " + cookie_string);
  }
  std::string name_value_part = parts[0];
  set_name_and_value(name_value_part);

  for (size_t i = 1; i < parts.size(); i++) {
    std::string attribute = parts[i];
    std::size_t equals_pos = attribute.find('=');
    std::string attr_name = "";
    if (equals_pos != std::string::npos) {
      attr_name = attribute.substr(0, equals_pos);
    } else {
      attr_name = attribute;
    }
    std::transform(attr_name.begin(), attr_name.end(), attr_name.begin(),
                   ::tolower);

    if (attr_name == "domain") {
      // If there was no "=" in the attribute, we use default.
      if (equals_pos != std::string::npos) {
        std::string explicit_domain = attribute.substr(equals_pos + 1);
        if (!explicit_domain.empty() && explicit_domain[0] == '.') {
          explicit_domain = explicit_domain.substr(1);
        }

        if (!explicit_domain.empty()) {
          host_only = false;
          set_domain(explicit_domain);
        }
      }
    } else if (attr_name == "path") {
      // If there was no "=" in the attribute, we use default.
      if (equals_pos != std::string::npos) {
        set_path(attribute.substr(equals_pos + 1));
      }
    } else if (attr_name == "secure") {
      secure = true;
    } else {
      Logger::log(LogLevel::NON_FATAL,
                  "ignoring unrecognized(not supported) cookie attribute: " +
                      attribute);
    }
  }
}

void Cookie::set_name_and_value(const std::string &name_value_part) {
  std::size_t equals_pos = name_value_part.find('=');
  if (equals_pos == std::string::npos) {
    name = name_value_part;
    value = std::nullopt;
  } else {
    name = name_value_part.substr(0, equals_pos);
    value = name_value_part.substr(equals_pos + 1);
  }
}

void Cookie::set_domain(const std::string &domain_part) {
  if (domain_part.empty()) {
    // If domain attribute has no value, I use the default one.
    return;
  }
  if (domain_part[0] == '.') {
    domain = domain_part.substr(1);
  } else {
    domain = domain_part;
  }
}

void Cookie::set_path(const std::string &path_part) {
  if (path_part.empty() || path_part[0] != '/') {
    // If path attribute has no value or doesnt start with '/',
    // I use the default one.
    return;
  }
  path = path_part;
}

bool Cookie::is_cookie_applicable(const ParsedURL &url) const {
  if (secure && !url.is_https) {
    return false;
  }
  if (host_only) {
    if (url.host != domain) {
      return false;
    }
  } else {
    if (url.host != domain && !url.host.ends_with("." + domain)) {
      return false;
    }
  }
  if (path == url.path) {
    return true;
  } else {
    if (url.path.starts_with(path)) {
      // Here I validate to avoid situations like:
      // `path` = "/abc" and `url.path` = "/abcd", which should not match.
      if (path.ends_with('/')) {
        return true;
      } else {
        return url.path[path.size()] == '/';
      }
    } else {
      return false;
    }
    return false;
  }
}

HttpClient::HttpClient(RadioConfig config) { this->config = config; }

HttpClient::~HttpClient() {
  if (this->is_connected()) {
    this->disconnect();
  }
}

bool HttpClient::is_connected() const { return connection != nullptr; }

int HttpClient::get_server_fd() const {
  if (!is_connected()) {
    throw std::runtime_error(
        "tried to get server file descriptor while not being connected");
  }
  try {
    return connection->get_fd();
  } catch (const std::exception &e) {
    // Shouldn't happen, since `establish_connection` should exit on failure.
    throw std::runtime_error("failed to get server file descriptor: " +
                             std::string(e.what()));
  }
}

int HttpClient::get_icy_metaint() const {
  if (!is_connected()) {
    throw std::runtime_error(
        "tried to get icy-metaint while not being connected");
  }
  return this->icy_metaint;
}

bool HttpClient::is_transfer_chunked() const {
  return chunked_transfer_encoding_enabled;
}

bool HttpClient::has_pending_data() const {
  if (!is_connected()) {
    throw std::runtime_error(
        "tried to check for pending data while not being connected");
  }
  return connection->has_pending_data();
}

void HttpClient::flush_in_buffer(
    std::function<void(const char *, std::size_t)> consumer) {
  if (!is_connected()) {
    throw std::runtime_error(
        "tried to flush input buffer while not being connected");
  }
  connection->flush_input_buffer(consumer);
}

void HttpClient::establish_connection() {
  if (is_connected()) {
    Logger::log(LogLevel::NON_FATAL,
                "error while establishing connection: already connected");
    return;
  }

  ParsedURL parsed_url = parse_url(config.url, std::nullopt);
  session_cookies.clear();
  bool received_OK = false;
  while (!received_OK) {
    log_current_time();
    Logger::log(LogLevel::INFO, "resolving name " + parsed_url.host);

    connect_to_server(parsed_url, config.use_ipv4, config.use_ipv6);
    std::vector<std::string> extra_headers = {cookies_to_attach(parsed_url)};
    std::string request =
        build_http_get_request(parsed_url, extra_headers, config.multiplex);

    HttpResponse response = send_and_receive_http_request(request);
    update_session_cookies(response, parsed_url);

    std::optional<int> opt_status_code = response.get_status_code();
    if (!opt_status_code.has_value()) {
      throw std::runtime_error("failed to parse status code from response");
    }

    int status_code = opt_status_code.value();
    if (status_code == OK_STATUS_CODE) {
      Logger::log(LogLevel::DEBUG, "received OK response from server");
      received_OK = true;
      handle_OK_status_code(response);
    } else if (REDIRECTION_STATUS_CODES.contains(status_code)) {
      parsed_url = handle_redirect(response, parsed_url);
    } else {
      throw std::runtime_error("received status code: " +
                               std::to_string(status_code));
    }
  }
}

void HttpClient::disconnect() {
  if (!is_connected()) {
    Logger::log(LogLevel::NON_FATAL,
                "error while disconnecting: not connected");
    return;
  }

  connection.reset(nullptr);
  icy_metaint = 0;
  Logger::log(LogLevel::DEBUG, "disconnected from server");
}

ssize_t HttpClient::read_stream_data(char *buffer, size_t length) {
  if (!is_connected()) {
    throw std::runtime_error(
        "tried to read stream data while not being connected");
  }
  try {
    return connection->read_data(buffer, length);
  } catch (const std::exception &e) {
    throw std::runtime_error("failed to read stream data: " +
                             std::string(e.what()));
  }
}

std::string HttpClient::cookies_to_attach(const ParsedURL &url) const {
  std::string result = "";
  if (session_cookies.empty()) {
    return "";
  }
  // In order to avoid multiple reallocations, we reserve space.
  // (might not be enough)
  result.reserve(8 * session_cookies.size());

  bool first_cookie = true;
  for (const auto &cookie : session_cookies) {
    if (cookie.second.is_cookie_applicable(url)) {
      if (first_cookie) {
        result.append("Cookie: ");
      } else {
        result.append("; ");
      }
      result.append(cookie.second.name);
      if (cookie.second.value.has_value()) {
        result.append("=");
        result.append(cookie.second.value.value());
      }
      first_cookie = false;
    }
  }
  return result;
}

void HttpClient::update_session_cookies(const HttpResponse &response,
                                        const ParsedURL &current_url) {
  std::vector<std::string> cookie_strings = response.get_cookie_strings();

  for (const std::string &cookie_string : cookie_strings) {
    try {
      Cookie cookie(cookie_string, current_url);
      CookieKey key = std::make_tuple(cookie.name, cookie.domain, cookie.path);
      session_cookies.insert_or_assign(key, cookie);
    } catch (const std::invalid_argument &e) {
      Logger::log(LogLevel::NON_FATAL, e.what());
    }
  }
}

void HttpClient::log_current_time() const {
  auto now = std::chrono::system_clock::now();
  Logger::log(
      LogLevel::INFO,
      std::format(DATE_FORMAT, std::chrono::floor<std::chrono::seconds>(now)));
}

std::string HttpClient::read_http_line() {
  if (!is_connected()) {
    // Shouldn't happen
    throw std::runtime_error(
        "tried to read HTTP line while not being connected");
  }
  std::string line;
  char c;
  while (true) {
    ssize_t bytes_read = connection->read_byte(&c);
    if (bytes_read < 0) {
      throw std::runtime_error("failed to read from server: " +
                               std::string(strerror(errno)));
    } else if (bytes_read == 0) {
      // Connection closed by server.
      throw std::runtime_error(
          "connection closed by server while reading HTTP line");
    }
    if (c == '\n') break;
    if (c != '\r') line += c;
  }
  return line;
}

HttpResponse HttpClient::read_http_response() {
  HttpResponse response{};
  Logger::log(LogLevel::DEBUG, "waiting for response from server...");
  response.status_line = read_http_line();
  Logger::log(LogLevel::INFO, response.status_line);
  if (response.status_line.empty()) {
    throw std::runtime_error("failed to read status line from server");
  }

  while (true) {
    std::string header_line = read_http_line();
    Logger::log(LogLevel::INFO, header_line);
    if (header_line.empty()) break;

    std::size_t colon_pos = header_line.find(':');
    if (colon_pos == std::string::npos) {
      throw std::runtime_error("received malformed header line: " +
                               header_line + " (missing colon)");
    }
    std::string header_name = header_line.substr(0, colon_pos);
    if (header_name.empty()) {
      Logger::log(LogLevel::NON_FATAL,
                  "received malformed header line: " + header_line +
                      " (empty header name)");
    } else {
      std::transform(header_name.begin(), header_name.end(),
                     header_name.begin(), ::tolower);

      std::string header_value = header_line.substr(colon_pos + 1);
      header_value.erase(0, header_value.find_first_not_of(" \t"));
      response.headers.insert({header_name, header_value});
    }
  }
  Logger::log(LogLevel::INFO, "");  // Log empty line after headers
  return response;
}

HttpResponse HttpClient::send_and_receive_http_request(
    const std::string &request) {
  if (!is_connected()) {
    // Shouldn't happen
    throw std::runtime_error(
        "tried to send HTTP request while not being connected");
  }
  Logger::log(LogLevel::INFO, request);
  ssize_t bytes_written = connection->writen(request.c_str(), request.length());
  if (bytes_written < 0) {
    throw std::runtime_error("failed to send HTTP request: " +
                             std::string(strerror(errno)));
  } else if (bytes_written < static_cast<ssize_t>(request.length())) {
    throw std::runtime_error("failed to send complete HTTP request");
  }

  HttpResponse response;
  std::size_t info_responses_count = 0;
  do {
    response = read_http_response();
  } while (
      is_informational_status_code(response.get_status_code().value_or(0)) &&
      ++info_responses_count < MAX_INFO_RESPONSES);
  return response;
}

void HttpClient::handle_OK_status_code(const HttpResponse &response) {
  if (config.multiplex) {
    std::optional<int> opt_icy_metaint = response.get_icy_metaint();
    if (!opt_icy_metaint.has_value()) {
      icy_metaint = 0;
      Logger::log(
          LogLevel::DEBUG,
          "server does not support metadata, proceeding without metadata");
    } else {
      icy_metaint = opt_icy_metaint.value();
      if (icy_metaint <= 0) {
        throw std::runtime_error("received invalid icy-metaint value: " +
                                 std::to_string(icy_metaint));
      }
    }
  }
  if (response.is_transfer_chunked()) {
    chunked_transfer_encoding_enabled = true;
    Logger::log(LogLevel::DEBUG, "chunked transfer encoding enabled");
  }
}

// This function returns new URL to which the client was redirected.
ParsedURL HttpClient::handle_redirect(const HttpResponse &response,
                                      const ParsedURL current_url) {
  std::optional<std::string> opt_new_url = response.get_url();
  if (!opt_new_url.has_value()) {
    throw std::runtime_error(
        "failed to parse new URL from Location "
        "header while being redirected");
  }
  if (opt_new_url.value().empty()) {
    throw std::runtime_error("received invalid redirect URL: URL is empty");
  }

  ParsedURL new_url = parse_url(opt_new_url.value(), current_url);
  if (new_url == current_url) {
    throw std::runtime_error("received redirect to the same URL: " +
                             new_url.host);
  }

  return new_url;
}

void HttpClient::connect_to_server(const ParsedURL &url, bool use_ipv4,
                                   bool use_ipv6) {
  addrinfo hints{};
  hints.ai_socktype = SOCK_STREAM;  // TCP
  hints.ai_protocol = IPPROTO_TCP;

  if (use_ipv4 && !use_ipv6) {
    hints.ai_family = AF_INET;  // IPv4
  } else if (use_ipv6 && !use_ipv4) {
    hints.ai_family = AF_INET6;  // IPv6
  } else {  // If both or neither of the flags are set, allow both IPv4 and
            // IPv6.
    hints.ai_family = AF_UNSPEC;
  }

  addrinfo *result = nullptr;
  int errcode = getaddrinfo(url.host.c_str(), std::to_string(url.port).c_str(),
                            &hints, &result);
  if (errcode != 0) {
    throw std::runtime_error("getaddrinfo failed: " +
                             std::string(gai_strerror(errcode)));
  }

  int socket_fd =
      socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  try {
    if (socket_fd == -1) {
      throw std::runtime_error("socket failed: " +
                               std::string(strerror(errno)));
    }

    std::optional<std::string> addr_str = addrinfo_to_string(result);
    if (!addr_str.has_value()) {
      // Function `addrinfo_to_string` is just a wrapper around `getnameinfo`,
      // so we can assume that the error is the same as the one returned by
      // `getnameinfo`.
      throw std::runtime_error("getnameinfo failed: " +
                               std::string(strerror(errno)));
    }

    Logger::log(LogLevel::INFO, "connecting to server " + addr_str.value());
    if (connect(socket_fd, result->ai_addr, result->ai_addrlen) == -1) {
      throw std::runtime_error("connect failed: " +
                               std::string(strerror(errno)));
    }
    freeaddrinfo(result);
  } catch (const std::runtime_error &e) {
    if (socket_fd != -1) {
      close(socket_fd);
    }
    if (result != nullptr) {
      freeaddrinfo(result);
    }
    throw e;
  }

  connection.reset(nullptr);
  if (url.is_https) {
    connection = std::make_unique<SecureConnection>(socket_fd, url.host);
  } else {
    connection = std::make_unique<Connection>(socket_fd);
  }

  if (!connection) {
    throw std::runtime_error("failed to create connection object");
  }
}
