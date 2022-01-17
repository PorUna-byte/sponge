#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    IP2ETH::const_iterator it=ip2Eth.find(next_hop_ip);
    if(it==ip2Eth.end())  //Ethernet address not found,Enqueue the datagram and send an ARP request if necessary.
    {   
        datagram_wait2send.push_back(make_pair(dgram,next_hop_ip));
        IP2TIME::const_iterator it1=arpsent.find(next_hop_ip);
        if(it1==arpsent.end()) //send an ARP request
        {
            EthernetFrame frame;
            ARPMessage arpmsg;
            frame.header().type=EthernetHeader::TYPE_ARP;
            frame.header().dst=ETHERNET_BROADCAST;
            frame.header().src=_ethernet_address;
            arpmsg.opcode=ARPMessage::OPCODE_REQUEST;
            arpmsg.sender_ethernet_address=_ethernet_address;
            arpmsg.sender_ip_address=_ip_address.ipv4_numeric();
            //target_ethernet_address is what we want to learn about.
            arpmsg.target_ip_address=next_hop_ip;
            frame.payload()=arpmsg.serialize();  //now the ARP frame has been constructed properly.
            arpsent[next_hop_ip]=0;  //record 
            _frames_out.push(frame);
        }  
    }
    else //Ethernet address is already known,just send out the frame.
    {
        EthernetFrame frame;
        frame.header().type=EthernetHeader::TYPE_IPv4;
        frame.header().dst=ip2Eth[next_hop_ip].first;
        frame.header().src=_ethernet_address;
        frame.payload()=dgram.serialize();
        _frames_out.push(frame);
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    if(frame.header().dst==ETHERNET_BROADCAST||frame.header().dst==_ethernet_address)
    {
        if(frame.header().type==EthernetHeader::TYPE_IPv4)
        {
            InternetDatagram res;
            if(res.parse(frame.payload())==ParseResult::NoError)
              return res;
        }
        else if(frame.header().type==EthernetHeader::TYPE_ARP)
        {
            ARPMessage arp;
            if(arp.parse(frame.payload())==ParseResult::NoError)
            {
                if(arp.opcode==ARPMessage::OPCODE_REQUEST&&arp.target_ip_address==_ip_address.ipv4_numeric())
                //send an ARP reply
                {
                   ARPMessage reply;
                   reply.opcode=ARPMessage::OPCODE_REPLY;
                   reply.sender_ethernet_address=_ethernet_address;
                   reply.sender_ip_address=_ip_address.ipv4_numeric();
                   reply.target_ethernet_address=arp.sender_ethernet_address;
                   reply.target_ip_address=arp.sender_ip_address;
                   EthernetFrame send_frame;
                   send_frame.header().type=EthernetHeader::TYPE_ARP;
                   send_frame.header().dst=arp.sender_ethernet_address;
                   send_frame.header().src=_ethernet_address;
                   send_frame.payload()=reply.serialize();
                   _frames_out.push(send_frame);
                }
                ip2Eth[arp.sender_ip_address]=make_pair(arp.sender_ethernet_address,0);
               //since we have learned a mapping from ip address to ethernet address
               //,we should check datagram to send
               for(auto it=datagram_wait2send.begin();it!=datagram_wait2send.end();)
               {
                   if((it->second)==arp.sender_ip_address)
                   {
                        EthernetFrame send_frame;
                        send_frame.header().src=_ethernet_address;
                        send_frame.header().dst=arp.sender_ethernet_address;
                        send_frame.header().type=EthernetHeader::TYPE_IPv4;
                        send_frame.payload()=(it->first).serialize();
                        _frames_out.push(send_frame);
                        datagram_wait2send.erase(it);
                   }
                   else
                        it++;
               }
            }
        }
    }
    return std::nullopt;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    auto pre=ip2Eth.begin(); 
    for(auto it=ip2Eth.begin();it!=ip2Eth.end();)
    {   
        (it->second).second+=ms_since_last_tick;
        if((it->second).second>=30000)
        {
            if(it==ip2Eth.begin())
            {
                ip2Eth.erase(it);
                it=ip2Eth.begin();
            }
            else
            {
                ip2Eth.erase(it);
                it=pre;
                it++;
            }
        }
        else
        {
            pre=it;
            it++;  
        }  
    }
    auto pre_=arpsent.begin();
    for(auto it=arpsent.begin();it!=arpsent.end();)
    {
        (it->second)+=ms_since_last_tick;
        if(it->second>=5000)
        {
            if(it==arpsent.begin())
            {
                arpsent.erase(it);
                it=arpsent.begin();
            }
            else
            {
                arpsent.erase(it);
                it=pre_;
                it++;
            }
        }
        else
        {
          pre_=it;
          it++; 
        } 
    }
 }
