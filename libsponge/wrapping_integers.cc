#include "wrapping_integers.hh"
#include <iostream>

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
uint64_t dis(uint64_t a,uint64_t b)
{
    if(a>b)
    return a-b;
    else
    return b-a;
}
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    
    return WrappingInt32{static_cast<uint32_t>(isn.raw_value()+n)};
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    uint64_t two_32=1ul<<32;
    uint64_t i=0;
    uint64_t ans=two_32;
    if(isn.raw_value()<=n.raw_value())
      ans=n.raw_value()-isn.raw_value();
    else
      ans=ans+n.raw_value()-isn.raw_value();
    i=checkpoint/two_32>=1?checkpoint/two_32-1:checkpoint/two_32;  
    while(true)  //Find the nearest absolute seqno to checkpoint.
    {
      if(dis(ans+i*two_32,checkpoint)<=dis(ans+(i+1)*two_32,checkpoint))
      {
        ans=ans+i*two_32;
        break;
      }
      else
        i++;
    }  
    return {ans};
}
