#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"

using namespace std;

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::sequence_numbers_in_flight() const {
    // uint64_t total = 0;
    // auto q = outstanding_;  // 拷贝一份，不破坏原队列
    // // printf("outstanding is empty?\n", outstanding_.empty());
    // while (!q.empty()) {
    //     total += q.front().sequence_length();
    //     std::cout << "q.sequence_length() = " << q.front().sequence_length() << std::endl;
    //     q.pop();
    // }
    // // printf("outstanding is empty %d ? total = %ld\n", outstanding_.empty(), total);

    // return total;
  return window_used_;
}

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::consecutive_retransmissions() const
{
  return consecutive_retransmissions_;
}

// void TCPSender::push( const TransmitFunction& transmit )
// {
//   if (!timer_.running())
//     timer_.start();

//   if (window_size_ == 0) {        // 第一次 push

//     TCPSenderMessage sender;
//     if (!absolute_seqno_) {
//       sender.SYN = true;
//     }
//                   // ✅ 设置 SYN
//     sender.seqno = Wrap32::wrap(absolute_seqno_, isn_);
//     if (input_.reader().is_finished()) {
//       sender.FIN = true;
//       fin_sent_ = true;
//     }
//     transmit(sender);
//     absolute_seqno_ += sender.sequence_length();  // ✅ 用这句统一更新
//     window_used_ += sender.sequence_length();
//     outstanding_.push(sender);      // ✅ 加入队列
    
//     // if (!timer_.running())
//     //     timer_.start();
//     return;

//   }
//   if (window_full()) {
//     return;
//   }
//     // return;
//   // const uint16_t window_left = window_size_ - window_used_;

//   // if (!input_.empty()) {
//     // uint64_t i = 0;
//     // while (!window_full()) {
//     //   size_t message_len = std::min((size_t)window_left, TCPConfig::MAX_PAYLOAD_SIZE);
//     //   uint64_t byte_size = input_.reader().bytes_buffered();
//     //   message_len = std::min(message_len, byte_size);

//     //   TCPSenderMessage sender;
//     //   sender.seqno = Wrap32::wrap(absolute_seqno_, isn_);
//     //   sender.payload = input_.reader().peek().substr(i, message_len );
//     //   i += message_len;
//     //   input_.reader().pop(message_len);
          
      
//     // if (input_.reader().is_finished() && !fin_sent_ && !window_full()) {
//     //   sender.FIN = true;
//     //   fin_sent_ = true;
//     //   message_len++;
//     //   }


//     //   window_used_ += message_len;
//     //   absolute_seqno_ += message_len;
//     //   transmit(sender);
//     //   outstanding_.push(sender);

//     //   if (fin_sent_)
//     //     break;
//     // }
//     uint16_t window_left = window_size_ - window_used_;
//     while (window_left > 0 && !input_.empty() && !fin_sent_) {
//       size_t message_len = std::min(
//           static_cast<size_t>(window_left),
//           std::min(input_.reader().bytes_buffered(), TCPConfig::MAX_PAYLOAD_SIZE)
//       );

//       if (message_len == 0 && !(input_.reader().is_finished() && !fin_sent_))
//           break;

//       TCPSenderMessage msg;
//       msg.seqno = Wrap32::wrap(absolute_seqno_, isn_);
//       msg.payload = input_.reader().peek().substr(0, message_len);
//       input_.reader().pop(message_len);
//       if (absolute_seqno_ == 0) {
//         msg.SYN = true;
//       } 
//       std::cout << "fin_sent_: " << fin_sent_ << " window_left > message_len " << (window_left < message_len) << std::endl;
//       if (input_.reader().is_finished() && !fin_sent_ && window_left > message_len) {
          
//           msg.FIN = true;
//           fin_sent_ = true;
//       }
      
//       transmit(msg);
//       outstanding_.push(msg);

//       absolute_seqno_ += msg.sequence_length();
//       window_used_ += msg.sequence_length();
//       window_left = window_size_ - window_used_;
//   }
    
    
//   // }

//   if (input_.reader().is_finished() && !fin_sent_ && !window_full()) {
//     TCPSenderMessage fin_seg;
//     fin_seg.seqno = Wrap32::wrap(absolute_seqno_, isn_);
//     fin_seg.FIN = true;
//     transmit(fin_seg);
//     outstanding_.push(fin_seg);
//     fin_sent_ = true;
//     absolute_seqno_ += 1; // FIN 占一个序号
//     // last_ack_ = fin_seg.seqno.unwrap(isn_, 0);
//     window_used_ += 1;
//   }
  
// }
void TCPSender::push( const TransmitFunction& transmit )
{
    // 如果 outstanding 队列为空，且有数据/FIN要发送，启动定时器
    if (outstanding_.empty() && (!input_.reader().is_finished() || !fin_sent_)) {
        timer_.start();
    }
    
    // 确定当前可用的窗口大小。
    // 如果接收方报告 window_size = 0，则 CS144 要求我们将其视为 1，
    // 以允许发送窗口探测（probe）段或 1 字节的 SYN/FIN。
    const uint16_t current_window_size = window_size_ == 0 ? 1 : window_size_;
    
    // 循环发送，直到窗口用尽或所有内容发送完毕
    while (window_used_ < current_window_size) {
        
        // 1. 构造一个消息
        TCPSenderMessage msg;
        
        // 计算当前剩余窗口空间
        uint64_t window_left = current_window_size - window_used_;
        
        // 2. 设置 SYN（仅在第一个段发送）
        if (absolute_seqno_ == 0) {
            msg.SYN = true;
            window_left--; // SYN 占用一个序号
        }
        
        // 3. 计算 Payload 长度
        size_t available_bytes = input_.reader().bytes_buffered();
        
        // 实际有效载荷大小：取三者的最小值
        size_t payload_size = std::min({
            available_bytes,
            (size_t)window_left,
            (size_t)TCPConfig::MAX_PAYLOAD_SIZE
        });

        // 4. 填充 Payload
        if (payload_size > 0) {
            msg.payload = input_.reader().peek().substr(0, payload_size);
            input_.reader().pop(payload_size);
            window_left -= payload_size; // 减去 Payload 占用的空间
        }

        // 5. 设置 FIN（在数据流结束、FIN 尚未发送、且窗口允许发送 FIN 的 1 个序号时）
        bool can_send_fin = input_.reader().is_finished() && !fin_sent_ && (window_left > 0);
        if (can_send_fin) {
            msg.FIN = true;
            fin_sent_ = true;
        }
        
        // 6. 检查是否需要发送。
        // 如果此段没有 SYN，没有 FIN，也没有 Payload，则停止发送
        if (msg.sequence_length() == 0) {
            break;
        }

        // 7. 设置 Sequence Number
        msg.seqno = Wrap32::wrap(absolute_seqno_, isn_);
        msg.RST = input_.has_error();
        // 8. 更新状态并发送
        uint64_t len = msg.sequence_length();
        
        transmit(msg);
        outstanding_.push(msg);

        absolute_seqno_ += len;
        window_used_ += len;

        // 如果 FIN 已经发送，或者窗口满了，则退出循环
        if (msg.FIN || window_used_ >= current_window_size) {
            break;
        }
    }
    
    // 如果 outstanding 队列不为空，确保定时器运行
    if (!outstanding_.empty() && !timer_.running()) {
        timer_.start();
    }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // debug( "unimplemented make_empty_message() called" );
  // return {};
  TCPSenderMessage sender;
  sender.seqno = Wrap32::wrap(absolute_seqno_, isn_);
  sender.RST = input_.has_error();
  // std::cerr << "absolute_seqno_ " << absolute_seqno_ << std::endl; 
  return sender;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  window_size_ = msg.window_size;
  window_set_ = true;
  if (msg.RST) {
    input_.set_error();
    return;
  }
  if (!msg.ackno.has_value()) {
        return;
  }
  

  uint64_t abs_no = msg.ackno.value().unwrap(isn_, absolute_seqno_);
  if (abs_no <= last_ack_ || abs_no > absolute_seqno_) {
    return;
  }
 
  last_ack_ = abs_no;
  timer_.reset_rto();
  consecutive_retransmissions_ = 0;
 
  while (!outstanding_.empty()) {

    const TCPSenderMessage &m = outstanding_.front();
    uint64_t seg_start = m.seqno.unwrap(isn_, absolute_seqno_);
    uint64_t seg_len = m.sequence_length();
    uint64_t seg_end = seg_start + seg_len; // 末尾的下一个序号

    if (seg_end <= abs_no) {
      window_used_ -= seg_len;      
      outstanding_.pop();
    }
    else break;
  }

  if (outstanding_.empty()) {
    timer_.stop();
  }
  else timer_.start();

  

}

void TCPSender::retransmit_one(uint64_t abs_no, const TransmitFunction& transmit ) {
  TCPSenderMessage m1 = outstanding_.front();
  uint64_t m1_seq_no = m1.seqno.unwrap(isn_, absolute_seqno_);
  if (m1_seq_no + m1.sequence_length() <= abs_no) {
    outstanding_.pop();
    transmit(m1);
  }
}

void TCPSender::tick(uint64_t ms_since_last_tick, const TransmitFunction& transmit) {
    // 1. 如果没有未确认的数据，停止定时器并返回
    if (outstanding_.empty()) {
        timer_.stop();
        return;
    }

    // 2. 更新计时器
    timer_.tick(ms_since_last_tick);

    // 3. 如果未到期，返回
    if (!timer_.expired()) return;

    // 4. 定时器到期：重传
    
    // **a. 检查窗口和 RTO 翻倍**
    // 仅当窗口大小 > 0 时，才翻倍 RTO
    if (window_size_ > 0 || !window_set_) {
        timer_.double_rto();
    }
    
    // **b. 增加重传计数**
    consecutive_retransmissions_++;

    // **c. 重传最早的未确认段**
    transmit(outstanding_.front());

    // **d. 重置/重新启动定时器**
    timer_.start();
}

void RetransmissionTimer::start() {
  running_ = true;
  elapsed_ = 0;
}

void RetransmissionTimer::stop() {
  running_ = false;
  elapsed_ = 0;
}

void RetransmissionTimer::tick(uint64_t ms) {
  if (running_) {
    elapsed_ += ms;
  }
}
// 判断是否超时
bool RetransmissionTimer::expired() {
  std::cout << "RUNNING: " << running_ << " elapsed_ >= curr_rto_ " << (elapsed_ >= curr_rto_) << std::endl;
  return running_ && elapsed_ >= curr_rto_;
}

void RetransmissionTimer::double_rto() {
  curr_rto_ *= 2;
}

void RetransmissionTimer::reset_rto() {
  curr_rto_ = init_rto_;
}
