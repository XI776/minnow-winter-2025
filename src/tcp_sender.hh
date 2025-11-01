#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <functional>
#include <queue>
#include <algorithm>
#include <iostream>

class RetransmissionTimer {
  public:
    explicit RetransmissionTimer(uint64_t init_rto) :
      init_rto_(init_rto), curr_rto_(init_rto), elapsed_(0), running_(false){}
    void start();
    void stop();
    void tick(uint64_t ms);
    bool expired();
    void double_rto();
    void reset_rto();
    bool running() const {return running_;}
    uint64_t curr_rto() const {return curr_rto_;}

  private:
    uint64_t init_rto_;
    uint64_t curr_rto_;
    uint64_t elapsed_;
    bool running_;
};

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms ), 
    outstanding_(), ms_since_last_tick_(0), window_size_(0), 
    window_used_(0), timer_(initial_RTO_ms), absolute_seqno_(0), 
    consecutive_retransmissions_(0), last_ack_(0), fin_sent_(false), window_set_(false)
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // For testing: how many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // For testing: how many consecutive retransmissions have happened?
  const Writer& writer() const { return input_.writer(); }
  const Reader& reader() const { return input_.reader(); }
  Writer& writer() { return input_.writer(); }
  bool window_full() const {return window_used_ >= window_size_;}
  void retransmit_one(uint64_t abs_no, const TransmitFunction& transmit) ;
private:
  Reader& reader() { return input_.reader(); }

  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;
  std::queue<TCPSenderMessage> outstanding_; // 没有确认的message
  uint64_t ms_since_last_tick_; // 上次重传的时间
  // uint64_t RTO_ms_; // 现在使用的RTO
  uint16_t window_size_; 
  uint16_t window_used_;
  RetransmissionTimer timer_;
  uint64_t absolute_seqno_;
  uint64_t consecutive_retransmissions_;
  uint64_t last_ack_;
  bool fin_sent_;
  bool window_set_;
  // bool rst_set_;
  // std::optional<Wrap32> ackno_;
};
