#pragma once

#include "byte_stream.hh"
#include <map>
#include <unordered_map>
#include <optional>
#include <concepts>
#include <vector>
#include <queue>
#include <cassert>

class Reassembler
{
public:
  // Construct Reassembler to write into given ByteStream.
  explicit Reassembler( ByteStream&& output ) 
   : unassemble_strs_()
    , next_assembled_idx_(0)
    , unassembled_bytes_num_(0)
    , eof_idx_(-1)
    , output_(std::move( output ) )
    , capacity_(output.writer().available_capacity()) {}

  // min_heap(std::greater<Interval<uint64_t>>(), std::vector<Interval<uint64_t>>())
  
  // min_heap,
  // start_to_string{}
   

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
  // bool is_avilable(uint64_t index) const {return str_capacity_ > index;}
  // void append_to_map(uint64_t first_index,  const std::string &data);
  // void check_and_close();
  // void push_and_trim();
  // void check_and_close();
private:
  // ByteStream output_;
  // // uint64_t str_capacity_;
  // uint64_t expected_next_index;
  // uint64_t final_index;
  // uint64_t bytes_pending_ ;
  // bool last_substring_seen_;
  // std::priority_queue<
  //   Interval<uint64_t>,                 // (1) 要存储的数据类型
  //   std::vector<Interval<uint64_t>>,    // (2) 存储数据的底层容器
  //   std::greater<Interval<uint64_t>>    // (3) 比较器（决定是最大堆还是最小堆）
  // > min_heap;
  // std::unordered_map<uint64_t, std::string> start_to_string;
  std::map<size_t, std::string> unassemble_strs_;
  size_t next_assembled_idx_;
  size_t unassembled_bytes_num_;
  size_t eof_idx_;

  ByteStream output_;  //!< The reassembled in-order byte stream
  size_t capacity_;    //!< The maximum number of bytes
};

