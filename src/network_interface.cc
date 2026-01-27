#include <iostream>

#include "arp_message.hh"
#include "debug.hh"
#include "ethernet_frame.hh"
#include "exception.hh"
#include "helpers.hh"
#include "network_interface.hh"
#include <cstdint>

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

  auto it = arp_cache_.find(dst_ip_);
  if (it != arp_cache_.end()) {
    frame.header.dst = it->second;
    frame.header.type = EthernetHeader::TYPE_IPv4;

    
    dgram.serialize(serializer);
    serializer.buffer(frame.payload);
    frame.payload = serializer.finish();
  }
  else {
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

    datagrams_received_.push(dgram);
  }
  transmit(frame);
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( EthernetFrame frame )
{
  debug( "unimplemented recv_frame called" );
  (void)frame;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  debug( "unimplemented tick({}) called", ms_since_last_tick );
}
