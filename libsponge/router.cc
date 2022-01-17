#include "router.hh"

#include <iostream>

using namespace std;

// Dummy implementation of an IP router

// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

// For Lab 6, please replace with a real implementation that passes the
// automated checks run by `make check_lab6`.

// You will need to add private members to the class declaration in `router.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";

    routing_table.push_back(Entry(route_prefix,prefix_length,next_hop.has_value()?
    next_hop.value().ipv4_numeric():0,interface_num));
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    uint8_t prefix_length=0;
    size_t interface_num;
    uint32_t next_hop=0;
    bool matched=false;
    if(dgram.header().ttl==0||--dgram.header().ttl==0)
       return ;
    
    for(auto& it:routing_table)
    {
        if(it.match(dgram.header().dst)&&it._prefix_length>=prefix_length)
        {   
            prefix_length=it._prefix_length;
            interface_num=it._interface_num;
            next_hop=it._next_hop;
            matched=true;
        }
    }
    if(matched)
    {   
        if(next_hop==0) //directly connected.
            interface(interface_num).send_datagram(dgram,Address::from_ipv4_numeric(dgram.header().dst));
        else
            interface(interface_num).send_datagram(dgram,Address::from_ipv4_numeric(next_hop));
    }
    return ;
}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}
Entry::Entry(uint32_t route_prefix,uint8_t prefix_length,uint32_t next_hop,size_t interface_num):
_route_prefix(route_prefix),_prefix_length(prefix_length),_next_hop(next_hop),_interface_num(interface_num)
{}
bool Entry::match(uint32_t route_prefix)
{
     if(_prefix_length==0||(route_prefix>>(32-_prefix_length)==_route_prefix>>(32-_prefix_length)))
        return true;
     else
        return false;      
}