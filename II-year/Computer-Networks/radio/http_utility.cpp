#include "http_utility.hpp"

#include <arpa/inet.h>
#include <netdb.h>

#include <algorithm>
#include <sstream>
#include <stdexcept>

#include "logger.hpp"

namespace {
constexpr std::uint16_t DEFAULT_HTTP_PORT = 80;
constexpr std::uint16_t DEFAULT_ICY_PORT = 80;
constexpr std::uint16_t DEFAULT_HTTPS_PORT = 443;

void parse_url_scheme(const std::string &scheme, ParsedURL &parsed_url) {
  if (scheme == "http") {
    parsed_url.is_https = false;
    parsed_url.port = DEFAULT_HTTP_PORT;
  } else if (scheme == "https") {
    parsed_url.is_https = true;
    parsed_url.port = DEFAULT_HTTPS_PORT;
  } else if (scheme == "icy") {
    parsed_url.is_https = false;
    parsed_url.port = DEFAULT_ICY_PORT;
  } else {
    throw std::runtime_error("invalid URL: unsupported scheme '" + scheme +
                             "'");
  }
}

void parse_raw_ipv6_url(const std::string &authority, ParsedURL &parsed_url) {
  std::size_t host_end_pos = authority.find(']');
  if (host_end_pos == std::string::npos) {
    throw std::runtime_error(
        "invalid URL: missing closing ']' for IPv6 address");
  }
  std::size_t host_length = host_end_pos - 1;
  std::string raw_ipv6_host = authority.substr(1, host_length);
  parsed_url.host = raw_ipv6_host;

  // Since format `[host]` is reserved for hosts given in raw IPv6 format,
  // we procede with a validation.
  struct in6_addr v6_buffer;
  if ((inet_pton(AF_INET6, raw_ipv6_host.c_str(), &v6_buffer)) <= 0) {
    throw std::runtime_error(
        "invalid URL: used format \"[host]\", while "
        "host is not a valid IPv6 address.");
  }
}

void parse_port(const std::string &authority, std::size_t port_start_pos,
                ParsedURL &parsed_url) {
  if (port_start_pos >= authority.length()) {
    return;  // In such case, we use the default port.
  }

  if (authority[port_start_pos] != ':') {
    throw std::runtime_error("invalid URL: unexpected character '" +
                             std::string(1, authority[port_start_pos]) +
                             "' after host");
  }

  std::string port_str = authority.substr(
      port_start_pos + 1, authority.length() - port_start_pos - 1);
  try {
    parsed_url.port = static_cast<uint16_t>(std::stoul(port_str));
  } catch (const std::exception &) {
    throw std::runtime_error("invalid URL: invalid port number");
  }
}

void parse_authority(const std::string &authority, ParsedURL &parsed_url) {
  if (authority.empty()) {
    throw std::runtime_error("invalid URL: missing authority part");
  }

  std::size_t port_start_pos;
  if (authority[0] == '[') {  // Case if host is given by IPv6 address.
    parse_raw_ipv6_url(authority, parsed_url);

    // This will not fail, since `parse_raw_ipv6_url` will exit on failure.
    std::size_t host_end_pos = authority.find(']');
    port_start_pos = host_end_pos + 1;
  } else {  // Case if host is given by IPv4 address or hostname.
    port_start_pos = authority.find(':');

    if (port_start_pos == std::string::npos) {
      parsed_url.host = authority;
    } else {
      parsed_url.host = authority.substr(0, port_start_pos);
    }
  }
  parse_port(authority, port_start_pos, parsed_url);
}

ParsedURL parse_absolute_url(const std::string &url) {
  ParsedURL resolved_url;
  // Here we parse an absolute URL, so we assume that the scheme is present.
  std::size_t scheme_end_pos = url.find("://");
  std::string scheme = url.substr(0, scheme_end_pos);
  parse_url_scheme(scheme, resolved_url);

  std::size_t authority_start_pos =
      scheme_end_pos + 3;  // In order to skip "://".
  std::size_t authority_end_pos = url.find_first_of("/?", authority_start_pos);

  std::string authority, path_and_query;
  if (authority_end_pos == std::string::npos) {
    authority = url.substr(authority_start_pos);
    path_and_query = "/";  // If path is missing, we use "/" by default.
  } else {
    authority = url.substr(authority_start_pos,
                           authority_end_pos - authority_start_pos);
    path_and_query = url.substr(authority_end_pos);

    // If authority is followed directly by '?', we add '/' at the start.
    if (path_and_query.empty() || path_and_query[0] == '?') {
      path_and_query = "/" + path_and_query;
    }
  }

  parse_authority(authority, resolved_url);
  resolved_url.path = path_and_query;
  return resolved_url;
}
}  // namespace

bool ParsedURL::operator==(const ParsedURL &other) const {
  return host == other.host && port == other.port && path == other.path &&
         is_https == other.is_https;
}

bool HttpResponse::is_transfer_chunked() const {
  auto it = headers.find("transfer-encoding");
  if (it != headers.end()) {
    std::string transfer_encoding_value = it->second;
    std::transform(transfer_encoding_value.begin(),
                   transfer_encoding_value.end(),
                   transfer_encoding_value.begin(), ::tolower);
    return transfer_encoding_value == "chunked";
  }
  return false;
}

std::optional<int> HttpResponse::get_status_code() const {
  size_t first_space = status_line.find(' ');
  if (first_space == std::string::npos) {
    return std::nullopt;
  }

  size_t second_space = status_line.find(' ', first_space + 1);
  if (second_space == std::string::npos) {
    return std::nullopt;
  }

  std::string status_code_str =
      status_line.substr(first_space + 1, second_space - first_space - 1);
  try {
    return std::stoi(status_code_str);
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

std::optional<int> HttpResponse::get_icy_metaint() const {
  auto it = headers.find("icy-metaint");
  if (it != headers.end()) {
    try {
      return std::stoi(it->second);
    } catch (const std::exception &) {
      return std::nullopt;
    }
  }

  return std::nullopt;
}

std::vector<std::string> HttpResponse::get_cookie_strings() const {
  std::vector<std::string> cookies;

  auto range = headers.equal_range("set-cookie");
  for (auto it = range.first; it != range.second; ++it) {
    cookies.emplace_back(it->second);
  }
  return cookies;
}

std::optional<std::string> HttpResponse::get_url() const {
  auto it = headers.find("location");
  if (it != headers.end()) {
    return it->second;
  }

  return std::nullopt;
}

ParsedURL parse_url(const std::string &raw_url,
                    std::optional<ParsedURL> defaults_opt) {
  std::string url = raw_url;
  std::size_t hash_pos = raw_url.find('#');
  if (hash_pos != std::string::npos) {
    // Since the fragment identifier is not used in HTTP requests, we can just
    // ignore it.
    url = raw_url.substr(0, hash_pos);
  }

  std::size_t scheme_end_pos = url.find("://");
  if (scheme_end_pos == std::string::npos) {
    if (defaults_opt.has_value()) {
      const ParsedURL defaults = defaults_opt.value();

      if (url.length() >= 2 && url[0] == '/' && url[1] == '/') {
        // Case if `url` starts with "//". We use scheme from `defaults`.
        url = (defaults.is_https ? "https:" : "http:") + url;
      } else {
        ParsedURL resolved_url = defaults;

        if (url.empty()) return resolved_url;

        if (url[0] == '/') {
          // Case if `url` starts with "/". We override `path`.
          resolved_url.path = url;
        } else {
          // Case if `url` does not start with "/". We update `path`.
          std::size_t last_slash_pos = defaults.path.rfind('/');
          if (last_slash_pos == std::string::npos) {
            resolved_url.path = "/" + url;
          } else {
            resolved_url.path =
                defaults.path.substr(0, last_slash_pos + 1) + url;
          }
        }
        return resolved_url;
      }
    } else {
      throw std::runtime_error("invalid URL: missing scheme");
    }
  }

  return parse_absolute_url(url);
}

std::string build_http_get_request(
    const ParsedURL &url, const std::vector<std::string> &extra_headers,
    bool want_metadata) {
  std::string refactored_host = url.host;
  if (url.host.find(':') != std::string::npos) {
    // If `host` contains ':', it means that it's an IPv6 address, so we need
    // to wrap it in square brackets in order to avoid ambiguity.
    refactored_host = "[" + url.host + "]";
  }

  std::string request = "GET " + url.path + " HTTP/1.1\r\n";
  request += "Host: " + refactored_host + "\r\n";
  request += "Connection: Keep-Alive\r\n";
  for (const auto &header : extra_headers) {
    if (header.empty()) {
      continue;
    }
    request += header + "\r\n";
  }

  if (want_metadata) {
    Logger::log(LogLevel::DEBUG, "requesting metadata from server");
    request += "Icy-MetaData: 1\r\n";
  }
  request += "\r\n";
  return request;
}