# 📻 C++20 Internet Radio Client (Icecast/Shoutcast)

A robust, event-driven HTTP/TCP audio streaming client built from scratch in modern C++20. 
Unlike standard players, this project implements its own HTTP/1.1 client, TLS/SSL security, and protocol parsers without relying on high-level web libraries (like libcurl).

Developed as part of the **Computer Networks** course at the University of Warsaw (MIMUW).

## ✨ Engineering Highlights

* **Event-Driven Architecture**: Uses POSIX `poll()` in a single-threaded event loop to handle non-blocking I/O multiplexing between network streams and user input simultaneously.
* **Custom Protocol Parsers (FSM)**: 
  * Implements a strict **Finite State Machine (FSM)** to parse `Transfer-Encoding: chunked` byte streams on the fly.
  * Demultiplexes ICY stream metadata (track names) interleaved within the binary audio stream.
* **Secure & Dual-Stack**: Fully supports **TLS/SSL** encrypted streams via OpenSSL (`SecureConnection`). Implements robust hostname resolution supporting both **IPv4 and IPv6** seamlessly.
* **Session & State Management**: Handles HTTP 3xx redirects and fully parses/stores HTTP Cookies (domain and path matching) for persistent session state.
* **Memory Safety**: Strictly adheres to RAII principles, utilizing C++20 `std::unique_ptr` and zero raw `new/delete` allocations to ensure a leak-free runtime.

## 🏗️ System Architecture

The application is strictly decoupled into modular components:

1. **`Controller`**: The orchestrator running the `poll()` event loop, routing data between network sockets and standard pipelines.
2. **`HttpClient` & `Connection`**: A custom HTTP/1.1 client managing TCP connections, TLS handshakes (via OpenSSL), redirects, headers, and cookies.
3. **`StreamProcessor`**: The data pipeline that decodes chunked transfer encoding and demuxes ICY metadata, outputting raw binary audio to `stdout`.

## 🚀 Getting Started

### Prerequisites
* A C++20 compliant compiler (GCC / Clang)
* `make` and `clang-format` (optional)
* **OpenSSL** development libraries (`libssl-dev` / `openssl-devel`)
* `play` installed (or other simillar program)

### Building
The project includes a `Makefile` for easy compilation:
```bash
make all
```

### Usage
The client reads an audio stream from a provided URL and writes the raw binary audio data to stdout.

#### Basic usage (HTTP)
```bash
./sikradio -u http://stream3.polskieradio.pl:8904 | play -q -t mp3 -
```

#### Secure usage (HTTPS) with ICY Metadata enabled
```bash
./sikradio -u http://an04.cdn.eurozet.pl/ant-web.mp3 -mq | play -q -t mp3 -
```

#### Force IPv6 and set custom timeout (ms)
```bash
./sikradio -u https://stream.nowyswiat.online/mp3 -m -t 3500 | play -q -t mp3 -
```


### Command-line Arguments:
```bash
-u <url> : The URL of the radio stream (required).

-m       : Enable ICY metadata multiplexing (extracts track info).

-4 / -6: Force IPv4 or IPv6 resolution.

-t <ms>  : Set network timeout in milliseconds.

-v <lvl> : Set verbosity level (0-4) for internal debugging.

-q       : Shortcut for "-v0"
```