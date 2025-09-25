#pragma once

#include "byte_stream.hh"
#include <map>
#include <optional>

class Reassembler
{
public:
  // Construct Reassembler to write into given ByteStream.
  explicit Reassembler( ByteStream&& output ) : 
  output_( std::move( output ) ),
  bigcapacity_(1046),
  str_capacity_(output_.writer().available_capacity()),
  next_index(0),
  final_index_(0),
  strs()
   {}

  /*
   * Insert a new substring to be reassembled into a ByteStream.
   *   `first_index`: the index of the first byte of the substring
   *   `data`: the substring itself
   *   `is_last_substring`: this substring represents the end of the stream
   *   `output`: a mutable reference to the Writer
   *
   * The Reassembler's job is to reassemble the indexed substrings (possibly out-of-order
   * and possibly overlapping) back into the original ByteStream. As soon as the Reassembler
   * learns the next byte in the stream, it should write it to the output.
   *
   * If the Reassembler learns about bytes that fit within the stream's available capacity
   * but can't yet be written (because earlier bytes remain unknown), it should store them
   * internally until the gaps are filled in.
   *
   * The Reassembler should discard any bytes that lie beyond the stream's available capacity
   * (i.e., bytes that couldn't be written even if earlier gaps get filled in).
   *
   * The Reassembler should close the stream after writing the last byte.
   */
  void insert( uint64_t first_index, std::string data, bool is_last_substring );

  // How many bytes are stored in the Reassembler itself?
  // This function is for testing only; don't add extra state to support it.
  uint64_t count_bytes_pending() const;

  // Access output stream reader
  Reader& reader() { return output_.reader(); }
  const Reader& reader() const { return output_.reader(); }

  // Access output stream writer, but const-only (can't write from outside)
  const Writer& writer() const { return output_.writer(); }
  bool is_avilable(uint64_t index) const {return str_capacity_ > index;}
  void append_to_map(uint64_t first_index,  const std::string &data);
  void check_and_close();
  void push_and_trim();
private:
  ByteStream output_;
  uint64_t bigcapacity_;
  uint64_t str_capacity_;
  uint64_t next_index;
  std::optional<uint64_t> final_index_;

  std::map<uint64_t, std::string> strs;
};
