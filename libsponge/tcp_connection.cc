#include "tcp_connection.hh"

#include <iostream>
#include <climits>
#define MAXU16_t 65535U
// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { 
    return _sender.stream_in().remaining_capacity();
}

size_t TCPConnection::bytes_in_flight() const { 
    return _sender.bytes_in_flight();
}

size_t TCPConnection::unassembled_bytes() const { 
    return _receiver.unassembled_bytes();
 }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

void TCPConnection::send()  //it will be called almost whatever happens.
{   
    _sender.fill_window();
    while(!_sender.segments_out().empty())
    {
       _segments_out.push(_sender.segments_out().front());
       _sender.segments_out().pop();
       if(_receiver.ackno().has_value())
       {
           _segments_out.back().header().ack=true;
           _segments_out.back().header().ackno=_receiver.ackno().value();
           _segments_out.back().header().win=_receiver.window_size()>MAXU16_t?MAXU16_t:_receiver.window_size();
       }
    }
    if(_receiver.unassembled_bytes()==0&&_receiver.stream_out().input_ended()//preq #1
    &&_sender.stream_in().eof()   //preq #2
    &&_sender.bytes_in_flight()==0)  //preq #3  
    {  
        if(!_linger_after_streams_finish)
        {
           _active=false;  //passive clean shutdown
        }
        else if(_time_since_last_segment_received>=10*_cfg.rt_timeout)
           _active=false;  //active clean shutdown   
    }
}

void TCPConnection::abort(){
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _active=false;
}
void TCPConnection::segment_received(const TCPSegment &seg) {
    if(!_active) 
       return;
    _time_since_last_segment_received=0;
    //receive segment-----
    if(seg.header().rst)
       abort();
    else
    {
        _receiver.segment_received(seg);
        if(seg.header().ack)
            _sender.ack_received(seg.header().ackno,seg.header().win);
        if((seg.length_in_sequence_space()>0&&_sender.segments_out().empty())
        ||(_receiver.ackno().has_value() && (seg.length_in_sequence_space() == 0)
                && (seg.header().seqno == _receiver.ackno().value() - 1)))
        //1.at least one segment is sent in reply, to reflect an update in the ackno and window size.
        //2.responding to a “keep-alive” segment.
            _sender.send_empty_segment();
         
        if(_receiver.stream_out().input_ended()&&!_sender.stream_in().eof())
           _linger_after_streams_finish=false;
        send();
    }
    //receive segment-----
}

bool TCPConnection::active() const { return _active; }

size_t TCPConnection::write(const string &data) {
    if(!_active)
       return 0;
    size_t bytes_written=_sender.stream_in().write(data);
    _sender.fill_window();
    send();
    return bytes_written;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    if(!_active)
       return ; 
    _time_since_last_segment_received+=ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    if(_sender.consecutive_retransmissions()>TCPConfig::MAX_RETX_ATTEMPTS)
    {
       _sender.send_empty_segment();
       _sender.segments_out().back().header().rst=true;
       abort();
    }
    send();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    send();
}

void TCPConnection::connect() {
    send();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            _sender.send_empty_segment();
            _sender.segments_out().back().header().rst=true;
            abort();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
