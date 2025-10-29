#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{

  if (message.RST) {
    this->RST = true;
    reassembler_.set_error();
    connected = false;
    return;
  }
  if (message.SYN && !connected) {
    zero_point = message.seqno;
    connected = true;
  }
  if (!connected) {
    return;
  }
  // size_t ideal_no = reassembler_.next_no();
  if (!message.SYN && message.sequence_length() > 0 && message.seqno == zero_point) {
    ;
  }
  else {
    uint64_t absolute_no = message.seqno.unwrap(zero_point, reassembler_.writer().bytes_pushed());
    if (!message.SYN) 
      absolute_no -= 1;
    // 仅当段含有效 payload 或含 FIN 时才插入重组器
    if (message.payload.size() > 0 || message.FIN) {
      reassembler_.insert(absolute_no, message.payload, message.FIN);
    }
  }

  // reassembler_.insert(absolute_no, message.payload, message.FIN);
  // After reassembler_.insert(...)
  checkpoint = reassembler_.writer().bytes_pushed();
  if (connected)
    checkpoint += 1; // account for SYN
  if (reassembler_.writer().is_closed())
    checkpoint += 1; // account for FIN

  // printf("absoulte_no = %ld now reassembler_.writer().bytes_pushed() = %ld and checkpoint = %ld\n", absolute_no, reassembler_.writer().bytes_pushed(), checkpoint);


  }
TCPReceiverMessage TCPReceiver::send() const
{
  
  TCPReceiverMessage message;
  if (connected) {
    message.ackno = Wrap32::wrap(checkpoint, this->zero_point);
  }
  
  message.RST = reassembler_.has_error();
  
  uint64_t avail = reassembler_.available_capacity();
  message.window_size = avail > 65535 ? 65535 : avail;
  return message;

}
