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
  , arp_cache_()
  , datagrams_cache_()
  , arp_pending_()
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{

  EthernetFrame frame;
  frame.header.src = ethernet_address_;
  uint32_t dst_ip_ = next_hop.ipv4_numeric();
  Serializer serializer;
  if (arp_cache_.find(dst_ip_) != arp_cache_.end()) {
    frame.header.dst = arp_cache_[dst_ip_].address;
    frame.header.type = EthernetHeader::TYPE_IPv4;
    dgram.serialize(serializer);
    serializer.buffer(frame.payload);
    frame.payload = serializer.finish();
  }
  else {
    if (arp_pending_.find(dst_ip_) != arp_pending_.end()) {
      datagrams_cache_[dst_ip_].push(dgram);
      return;
    }
    arp_pending_[dst_ip_] = ARP_PENDING_TIME;
    frame.header.type = EthernetHeader::TYPE_ARP;
    frame.header.dst = ETHERNET_BROADCAST;

    // 如果目的地址不知道，先发送ARP
    ARPMessage arpmessage;

    arpmessage.opcode = ARPMessage::OPCODE_REQUEST;
    arpmessage.sender_ethernet_address = ethernet_address_;
    arpmessage.sender_ip_address = ip_address_.ipv4_numeric();
    // 广播
  
    arpmessage.target_ip_address = next_hop.ipv4_numeric();
    
    arpmessage.serialize(serializer);
    frame.payload = serializer.finish();
    std::queue<InternetDatagram> queue;
    queue.push(dgram);
    datagrams_cache_.insert({dst_ip_, queue});
  }
  transmit(frame);
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( EthernetFrame frame )
{

  Parser parser(frame.payload);

  // 如果是ipv4
  if (frame.header.type == EthernetHeader::TYPE_IPv4) {
    InternetDatagram dgram;
    dgram.parse(parser);
    if (frame.header.dst != ethernet_address_) {
      cerr << "InternetDatagram.target_ip_address != local address, just ignore\n"; 
      return;
    }
    datagrams_received_.push(dgram);
  }
  else if (frame.header.type == EthernetHeader::TYPE_ARP){
    ARPMessage arpmessage;
    arpmessage.parse(parser);

    if (arpmessage.target_ip_address != ip_address_.ipv4_numeric()) {
      cerr << "arpmessage.target_ip_address != local address, just ignore\n"; 
      return;
    }

    if (arpmessage.opcode == ARPMessage::OPCODE_REQUEST) {
      EthernetFrame frame1;
      frame1.header.src = ethernet_address_;
      frame1.header.type = EthernetHeader::TYPE_ARP;
      frame1.header.dst = arpmessage.sender_ethernet_address;

      ARPMessage arpmessage1;
      arpmessage1.opcode = ARPMessage::OPCODE_REPLY;
      arpmessage1.sender_ethernet_address = ethernet_address_;
      arpmessage1.sender_ip_address = ip_address_.ipv4_numeric();
      arpmessage1.target_ip_address = arpmessage.sender_ip_address;
      arpmessage1.target_ethernet_address = arpmessage.sender_ethernet_address;

      Serializer serializer;
      arpmessage1.serialize(serializer);
      frame1.payload = serializer.finish();
      transmit(frame1);
      EthernetAddress_and_time et(arpmessage.sender_ethernet_address, ARP_TIME_TO_LIVE);
      arp_cache_[arpmessage.sender_ip_address] = et;
    }
    else if (arpmessage.opcode == ARPMessage::OPCODE_REPLY) {
      EthernetAddress_and_time et(arpmessage.sender_ethernet_address, ARP_TIME_TO_LIVE);
      arp_cache_[arpmessage.sender_ip_address] = et;

      arp_pending_.erase(arpmessage.sender_ip_address);

      auto it = datagrams_cache_.find(arpmessage.sender_ip_address);
      if (it != datagrams_cache_.end()) {
        auto& que = it->second;
        Address next_hop = Address::from_ipv4_numeric(it->first);
        while (!que.empty()) {
          auto dgram = que.front();
          que.pop();
          send_datagram(dgram, next_hop);
        }
      }
    }
  }

}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // debug( "unimplemented tick({}) called", ms_since_last_tick );

  for (auto it = arp_cache_.begin(); it != arp_cache_.end(); ) {
    if (it->second.left_waiting_time <= ms_since_last_tick) {
      // send_datagram();//?
      it = arp_cache_.erase(it);
    }
    else {
      it->second.left_waiting_time -= ms_since_last_tick;
      ++it;
    }
  }

  for (auto it = arp_pending_.begin(); it != arp_pending_.end(); ) {
    if (it->second <= ms_since_last_tick) {
      it = arp_pending_.erase(it);  // erase 返回下一个
    }
    else {
      it->second -= ms_since_last_tick;
      ++it;
    }
  }

  for (auto it = datagrams_cache_.begin(); it != datagrams_cache_.end(); ) {
    if (arp_pending_.find(it->first) == arp_pending_.end()) {
        it = datagrams_cache_.erase(it);
    }
    else {
      ++it;
    }
  }
  
}
