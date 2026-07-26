#ifndef MB_HTTP_CLIENT_HPP
#define MB_HTTP_CLIENT_HPP

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "arg_parsing.hpp"
#include "connection.hpp"
#include "http_utility.hpp"

using CookieKey = std::tuple<std::string, std::string, std::string>;
// Strings in `CookieKey` represent the following: name, domain, path
// of a cookie.

struct Cookie {
  std::string name = "";
  std::optional<std::string> value = std::nullopt;
  std::string domain = "";
  std::string path = "";
  bool secure = false;
  bool host_only = true;

  Cookie(std::string cookie_string, const ParsedURL current_url);
  void set_name_and_value(const std::string &name_value_part);
  void set_domain(const std::string &domain_part);
  void set_path(const std::string &path_part);
  bool is_cookie_applicable(const ParsedURL &url) const;
};

class HttpClient {
 public:
  explicit HttpClient(RadioConfig config);
  ~HttpClient();

  bool is_connected() const;
  int get_server_fd() const;
  int get_icy_metaint() const;
  bool is_transfer_chunked() const;
  bool has_pending_data() const;
  void flush_in_buffer(std::function<void(const char *, std::size_t)> consumer);

  void establish_connection();
  void disconnect();

  ssize_t read_stream_data(char *buffer, size_t length);

 private:
  RadioConfig config;

  // If multiplexing was requested but server does not support it,
  // `icy_metaint` will be set to 0.
  int icy_metaint = 0;
  bool chunked_transfer_encoding_enabled = false;
  std::unique_ptr<Connection> connection = nullptr;
  std::map<CookieKey, Cookie> session_cookies = {};

  std::string cookies_to_attach(const ParsedURL &url) const;
  void update_session_cookies(const HttpResponse &response,
                              const ParsedURL &current_url);

  void log_current_time() const;
  std::string read_http_line();
  HttpResponse read_http_response();
  HttpResponse send_and_receive_http_request(const std::string &request);
  void handle_OK_status_code(const HttpResponse &response);
  ParsedURL handle_redirect(const HttpResponse &response,
                            const ParsedURL current_url);
  void connect_to_server(const ParsedURL &url, bool use_ipv4, bool use_ipv6);
};

#endif  // MB_HTTP_CLIENT_HPP