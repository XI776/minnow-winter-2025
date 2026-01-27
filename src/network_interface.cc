#include <iostream>

#include "arp_message.hh"
#include "debug.hh"
#include "ethernet_frame.hh"
#include "exception.hh"
#include "helpers.hh"
#include "network_interface.hh"
#include <cstdint>
#include <algorithm>

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  // cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
  //      << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  const uint32_t next_hop_ip = next_hop.ipv4_numeric();

  // 1. 查找 ARP 缓存
  auto it = arp_table_.find(next_hop_ip);
  if ( it != arp_table_.end() ) {
    EthernetFrame frame;
    frame.header.src = ethernet_address_;
    frame.header.dst = it->second.address;
    frame.header.type = EthernetHeader::TYPE_IPv4;
    frame.payload = serialize( dgram ); // 性能优化：直接序列化到 payload
    transmit( frame );
    return;
  }

  // 2. 没找到 MAC，先存入代发队列
  waiting_packets_[next_hop_ip].push_back(dgram);

  // 3. 检查 5 秒内是否已经发过 ARP REQUEST
  if (arp_request_timer_.find(next_hop_ip) != arp_request_timer_.end()) {
      return;
  }

  // 4. 发送 ARP 广播请求
  ARPMessage arpmessage;
  arpmessage.opcode = ARPMessage::OPCODE_REQUEST;
  arpmessage.sender_ethernet_address = ethernet_address_;
  arpmessage.sender_ip_address = ip_address_.ipv4_numeric();
  arpmessage.target_ip_address = next_hop_ip;
  
  EthernetFrame frame;
  frame.header.src = ethernet_address_;
  frame.header.dst = ETHERNET_BROADCAST;
  frame.header.type = EthernetHeader::TYPE_ARP;
  frame.payload = serialize(arpmessage);
  
  transmit(frame);
  arp_request_timer_[next_hop_ip] = ARP_REQUEST_TTL;

}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( EthernetFrame frame )
{
  // 1. 基础过滤: 目标不是我，并且不是广播，直接丢弃
  if (frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST) {
    return;
  }
  // 2. 处理 IPV4 数据包
  if (frame.header.type == EthernetHeader::TYPE_IPv4) {
    InternetDatagram dgram;
    if (parse(dgram, frame.payload)) {
      datagrams_received_.push( move( dgram ) );
    }
  }
  // 3. 处理 ARP 数据包
  else if (frame.header.type == EthernetHeader::TYPE_ARP){ // ARP
    ARPMessage msg;
    if (!parse(msg, frame.payload)) {
      return;
    }
    const uint32_t sender_ip = msg.sender_ip_address;
    // 只要收到 ARP 包，就学习发送者的映射 
    arp_table_[sender_ip] = {msg.sender_ethernet_address, ARP_TTL};

    // 如果是请求我 IP的Request，回复 Reply 
    if (msg.opcode == ARPMessage::OPCODE_REQUEST && msg.target_ip_address == ip_address_.ipv4_numeric()) {
      ARPMessage reply;
      reply.opcode = ARPMessage::OPCODE_REPLY;
      reply.sender_ethernet_address = ethernet_address_;
      reply.sender_ip_address = ip_address_.ipv4_numeric();
      reply.target_ip_address = sender_ip;
      reply.target_ethernet_address = msg.sender_ethernet_address;


      EthernetFrame reply_frame;
      reply_frame.header.src = ethernet_address_;
      reply_frame.header.type = EthernetHeader::TYPE_ARP;
      reply_frame.header.dst = msg.sender_ethernet_address;
      reply_frame.payload = serialize(reply);
      transmit(reply_frame);

    }

    // 4. 检查是否有包在等待这个 IP 的 MAC 地址
      auto it = waiting_packets_.find(sender_ip);
      if (it != waiting_packets_.end()) {
        for (const auto& dgram: it->second) {
          EthernetFrame f;
          f.header.src = ethernet_address_;
          f.header.dst = msg.sender_ethernet_address;
          f.header.type = EthernetHeader::TYPE_IPv4;
          f.payload = serialize( dgram );
          transmit( f );
        }
        waiting_packets_.erase(it);
        arp_request_timer_.erase( sender_ip );
      }
    }
  }


//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{

  // 1. 清理过期的 ARP 缓存
  for (auto it = arp_table_.begin(); it != arp_table_.end(); ) {
    if (it->second.ttl <= ms_since_last_tick) {
      it = arp_table_.erase(it);
    }
    else {
      it->second.ttl -= ms_since_last_tick;
      ++it;
    }
  }
  // 2. 清理 ARP Request 计时器
  for (auto it = arp_request_timer_.begin(); it != arp_request_timer_.end(); ) {
    if (it->second <= ms_since_last_tick) {
      uint32_t ip = it->first;
      it = arp_request_timer_.erase(it);  // erase 返回下一个
      waiting_packets_.erase(ip);
    }
    else {
      it->second -= ms_since_last_tick;
      ++it;
    }
  }


}
