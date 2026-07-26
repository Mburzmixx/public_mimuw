#include "stream_processor.hpp"

#include <stdexcept>

#include "logger.hpp"

namespace {
constexpr std::size_t AUDIO_FD = STDOUT_FILENO;
constexpr std::size_t ICY_METADATA_MULTIPLIER = 16;
}  // namespace

IcyDemuxer::IcyDemuxer(
    int icy_metaint,
    std::function<void(const char *, std::size_t)> _on_demuxed_data)
    : _icy_metaint(icy_metaint),
      _audio_bytes_rem(icy_metaint),
      _on_demuxed_data(_on_demuxed_data) {}

void IcyDemuxer::demux_data(const char *buffer, std::size_t length) {
  std::size_t audio_start = 0;
  std::size_t audio_len = 0;

  for (size_t i = 0; i < length; i++) {
    if (is_reading_metadata) {
      if (_metadata_buffer.empty() && _meta_bytes_rem == 0) {
        // Here I cast `buffer[i]` to `unsigned char`,
        //  since the length byte should be interpreted as an unsigned value.
        std::size_t len =
            static_cast<std::size_t>(static_cast<unsigned char>(buffer[i]));
        if (len == 0) {
          is_reading_metadata = false;
          _audio_bytes_rem = _icy_metaint;
        } else {
          _meta_bytes_rem = len * ICY_METADATA_MULTIPLIER;
        }
      } else {
        if (buffer[i] != '\0') {
          // The metadata is padded with null bytes, which should be ignored.
          _metadata_buffer += buffer[i];
        }
        _meta_bytes_rem--;

        if (_meta_bytes_rem == 0) {
          Logger::log(LogLevel::ZERO, _metadata_buffer);
          _metadata_buffer.clear();
          is_reading_metadata = false;
          _audio_bytes_rem = _icy_metaint;
        }
      }
    } else {
      // Case for reading audio.
      if (audio_len == 0) {
        audio_start = i;
      }
      audio_len++;
      _audio_bytes_rem--;

      if (_audio_bytes_rem == 0) {
        is_reading_metadata = true;
        _on_demuxed_data(buffer + audio_start, audio_len);
        audio_len = 0;
      }
    }
  }

  if (audio_len > 0) {
    _on_demuxed_data(buffer + audio_start, audio_len);
  }
}

ChunkDecoder::ChunkDecoder(
    std::function<void(const char *, std::size_t)> _on_dechunked_data)
    : _on_dechunked_data(_on_dechunked_data) {}

bool ChunkDecoder::dechunk_data(const char *buffer, std::size_t length) {
  if (length == 0) {
    Logger::log(LogLevel::DEBUG, "IcyDemuxer: no data to process");
  }

  for (size_t i = 0; i < length; i++) {
    switch (chunk_state) {
      case ChunkState::READING_SIZE:
        if (buffer[i] == '\r') {
          chunk_state = ChunkState::EXPECTING_SIZE_LF;
        } else {
          update_chunk_size(buffer[i]);
        }
        break;
      case ChunkState::IGNORING_EXTENSION:
        if (buffer[i] == '\r') {
          chunk_state = ChunkState::EXPECTING_SIZE_LF;
        }
        break;
      case ChunkState::EXPECTING_SIZE_LF:
        if (buffer[i] != '\n') {
          throw std::runtime_error(
              "expected '\\n' after '\\r' while parsing chunk size, but "
              "received: " +
              std::string(1, buffer[i]));
        }
        chunk_state = ChunkState::READING_DATA;
        break;
      case ChunkState::READING_DATA:
        if (remaining_chunk_size == 0) {
          // This means server has closed the connection.
          return false;
        }
        if (remaining_chunk_size > length - i) {
          _on_dechunked_data(buffer + i, length - i);
          remaining_chunk_size -= (length - i);
          i = length;  // We have processed all the data in the buffer.
        } else {
          _on_dechunked_data(buffer + i, remaining_chunk_size);
          i += remaining_chunk_size - 1;
          remaining_chunk_size = 0;
          chunk_state = ChunkState::EXPECTING_DATA_CR;
        }
        break;
      case ChunkState::EXPECTING_DATA_CR:
        if (buffer[i] != '\r') {
          throw std::runtime_error(
              "expected '\\r' after chunk data, but received: " +
              std::string(1, buffer[i]));
        }
        chunk_state = ChunkState::EXPECTING_DATA_LF;
        // Implementation for expecting carriage return after chunk data
        break;
      case ChunkState::EXPECTING_DATA_LF:
        if (buffer[i] != '\n') {
          throw std::runtime_error(
              "expected '\\n' after '\\r' following chunk data, but "
              "received: " +
              std::string(1, buffer[i]));
        }
        chunk_state = ChunkState::READING_SIZE;
        break;
    }
  }
  return true;
}

void ChunkDecoder::update_chunk_size(char c) {
  if (c >= '0' && c <= '9') {
    remaining_chunk_size = remaining_chunk_size * 16 + (c - '0');
  } else if (c >= 'a' && c <= 'f') {
    remaining_chunk_size = remaining_chunk_size * 16 + (c - 'a' + 10);
  } else if (c >= 'A' && c <= 'F') {
    remaining_chunk_size = remaining_chunk_size * 16 + (c - 'A' + 10);
  } else if (c == ';') {
    chunk_state = ChunkState::IGNORING_EXTENSION;
  } else {
    throw std::runtime_error("invalid character while parsing chunk size: " +
                             std::string(1, c));
  }
}

void StreamProcessor::enable_multiplexing(int icy_metaint) {
  if (icy_metaint <= 0) {
    throw std::runtime_error("icy-metaint must be a positive integer");
  }

  multiplexing_enabled = true;
  _icy_demuxer = std::make_unique<IcyDemuxer>(
      icy_metaint, [this](const char *buffer, std::size_t length) {
        this->write_raw_audio(buffer, length);
      });
}

void StreamProcessor::enable_chunking_transfer() {
  chunked_transfer_encoding_enabled = true;

  _chunk_decoder = std::make_unique<ChunkDecoder>(
      [this](const char *buffer, std::size_t length) {
        this->process_dechunked_data(buffer, length);
      });
}

void StreamProcessor::process_dechunked_data(const char *buffer,
                                             std::size_t length) {
  if (length == 0) {
    Logger::log(LogLevel::DEBUG, "StreamProcessor: no data to process");
    return;
  }

  if (buffer == nullptr) {
    throw std::runtime_error("StreamProcessor: buffer is null");
  }

  if (multiplexing_enabled) {
    _icy_demuxer->demux_data(buffer, length);
  } else {
    write_raw_audio(buffer, length);
  }
}

bool StreamProcessor::process_data(const char *buffer, std::size_t length) {
  if (chunked_transfer_encoding_enabled) {
    return _chunk_decoder->dechunk_data(buffer, length);
  } else {
    process_dechunked_data(buffer, length);
    return true;
  }
}

void StreamProcessor::write_raw_audio(const char *buffer, std::size_t length) {
  if (length == 0) {
    Logger::log(LogLevel::DEBUG, "write_raw_audio: no audio data to write");
    return;
  }

  ssize_t bytes_written = writen_to_fd(AUDIO_FD, buffer, length);
  if (bytes_written < 0) {
    throw std::runtime_error("failed to write audio data to stdout");
  } else if (bytes_written < static_cast<ssize_t>(length)) {
    throw std::runtime_error("partial write of audio data to stdout");
  }
}
