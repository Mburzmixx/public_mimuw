#ifndef MB_STREAM_PROCESSOR_HPP
#define MB_STREAM_PROCESSOR_HPP

#include <cstddef>
#include <functional>
#include <memory>
#include <string>

class IcyDemuxer {
 public:
  IcyDemuxer() = delete;
  IcyDemuxer(int icy_metaint,
             std::function<void(const char *, std::size_t)> _on_demuxed_data);
  ~IcyDemuxer() = default;

  void demux_data(const char *buffer, std::size_t length);

 private:
  int _icy_metaint = 0;
  std::size_t _audio_bytes_rem = 0;
  std::size_t _meta_bytes_rem = 0;
  std::string _metadata_buffer = "";
  bool is_reading_metadata = false;

  std::function<void(const char *, std::size_t)> _on_demuxed_data;
};

enum class ChunkState {
  READING_SIZE,
  IGNORING_EXTENSION,
  EXPECTING_SIZE_LF,
  READING_DATA,
  EXPECTING_DATA_CR,
  EXPECTING_DATA_LF
};

class ChunkDecoder {
 public:
  ChunkDecoder() = delete;
  ChunkDecoder(
      std::function<void(const char *, std::size_t)> _on_dechunked_data);
  ~ChunkDecoder() = default;

  // This function returns `false` if the server closed the connection
  // while sending data, and `true` otherwise.
  // It can happen due to chunked transfer encoding.
  bool dechunk_data(const char *buffer, std::size_t length);

 private:
  std::size_t remaining_chunk_size = 0;
  ChunkState chunk_state = ChunkState::READING_SIZE;

  std::function<void(const char *, std::size_t)> _on_dechunked_data;

  // This function updates `remaining_chunk_size`
  // and `chunk_state` according to the character `c`.
  // (may set state to `IGNORING_EXTENSION` if read a ";")
  // It should be called only when `chunk_state` is `READING_SIZE`.
  void update_chunk_size(char c);
};

class StreamProcessor {
 public:
  StreamProcessor() = default;
  ~StreamProcessor() = default;

  void enable_multiplexing(int icy_metaint);
  void enable_chunking_transfer();

  void process_dechunked_data(const char *buffer, std::size_t length);

  // This function returns `false` if the server closed the connection.
  // See `ChunkDecoder::dechunk_data`.
  bool process_data(const char *buffer, std::size_t length);

 private:
  std::unique_ptr<IcyDemuxer> _icy_demuxer = nullptr;
  std::unique_ptr<ChunkDecoder> _chunk_decoder = nullptr;

  bool multiplexing_enabled = false;
  bool chunked_transfer_encoding_enabled = false;

  void write_raw_audio(const char *buffer, std::size_t length);
};

#endif  // MB_STREAM_PROCESSOR_HPP