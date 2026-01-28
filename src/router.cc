#include "router.hh"
#include "debug.hh"

#include <iostream>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  // cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
  //      << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
  //      << " on interface " << interface_num << "\n";

  // debug( "unimplemented add_route() called" );

  
  route_table_.push_back(Route_Entry{route_prefix, prefix_length, std::move(next_hop), interface_num});
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  // debug( "unimplemented route() called" );

  for (auto& InterfacePtr: interfaces_) {
    std::queue<InternetDatagram>& queue = InterfacePtr->datagrams_received();
    while (!queue.empty()) {
      InternetDatagram dgram = std::move(queue.front());
      queue.pop();

      if (dgram.header.ttl <= 1 ) {
        continue;
      }
      dgram.header.ttl--;
      dgram.header.compute_checksum();

      const uint32_t dst_ip = dgram.header.dst;
      std::optional<Route_Entry> best = helper(dst_ip);
      
      if (!best.has_value()) {
        continue;
      }
      const Address next_hop_addr = best->next_hop.has_value() 
                                          ? best->next_hop.value() 
                                          : Address::from_ipv4_numeric(dgram.header.dst);
      interface(best->interface_num)->send_datagram(dgram, next_hop_addr);
    }
  }
}

std::optional<Router::Route_Entry> Router::helper(uint32_t dst_ip) {
  // 1. 计算目的 IP 和路由前缀这个，prefix_length 越大越好
  // 2. 选出 prefix_length 最大的，
  // 3. 看 next_hop 有无，没有的话，直接发送给目的，有的话，发送到下一跳
  int best_len = -1;
  std::optional<Route_Entry> best;
  for (const auto& entry: route_table_) {
    uint32_t mask = entry.prefix_length == 0
      ? 0 : 0xFFFFFFFF << ( 32 - entry.prefix_length);
    
    if ( (dst_ip & mask) != (entry.route_prefix & mask) ) {
      continue;
    }

    if (entry.prefix_length > best_len) {
      best_len = entry.prefix_length;
      best = entry;
    }
  }
  return best;

}
