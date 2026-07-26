#ifndef MB_HTTP_UTILITY_HPP
#define MB_HTTP_UTILITY_HPP

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

struct ParsedURL {
  std::string host;
  uint16_t port;
  std::string path;
  bool is_https = false;

  bool operator==(const ParsedURL &other) const;
};

struct HttpResponse {
  std::string status_line;
  std::multimap<std::string, std::string> headers;
  std::string body;

  bool is_transfer_chunked() const;
  std::optional<int> get_status_code() const;
  std::optional<int> get_icy_metaint() const;
  std::vector<std::string> get_cookie_strings() const;
  std::optional<std::string> get_url() const;
};

ParsedURL parse_url(const std::string &url,
                    std::optional<ParsedURL> defaults_opt);

std::string build_http_get_request(
    const ParsedURL &url, const std::vector<std::string> &extra_headers,
    bool want_metadata = false);

#endif  // MB_HTTP_UTILITY_HPP