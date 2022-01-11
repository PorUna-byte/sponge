#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    uint64_t checkpoint=_reassembler.getFirstUnassembled()>0?_reassembler.getFirstUnassembled()-1:
    _reassembler.getFirstUnassembled();
    uint64_t stream_index;
    if(seg.header().syn&&!ISN_rcv)
    {
       ISN_rcv=true;
       ISN_NO=seg.header().seqno; //record the initial seqno for later usage.
       offset++;
       stream_index=0;  //if a segment with syn,the segment data's stream index should be zero.
    }
    else if(ISN_rcv)
      stream_index=unwrap(seg.header().seqno,ISN_NO,checkpoint)-1;//absolute seqno-1 is stream index.
    else
      return ;  
    if(seg.header().fin&&!FIN_rcv)
    {  
       _reassembler.push_substring(seg.payload().copy(),
       stream_index,true);
       FIN_rcv=true;
    }
    else
    {
       _reassembler.push_substring(seg.payload().copy(),
       stream_index,false);
    }
    if(ISN_rcv&&FIN_rcv&&unassembled_bytes()==0&&offset<2)
      offset++;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if(ISN_rcv)
       return  wrap(_reassembler.getFirstUnassembled()+offset,ISN_NO);
    else
       return std::nullopt; 
    }

size_t TCPReceiver::window_size() const {
     return _reassembler.getFirstUnacc()-_reassembler.getFirstUnassembled(); }
