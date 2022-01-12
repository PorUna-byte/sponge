#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
#include <iostream>
// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout},time_left{retx_timeout},RTO{retx_timeout}
    , _stream(capacity),_consecutive_retransmissions(0){}

uint64_t TCPSender::bytes_in_flight() const { 
    return _bytes_in_flight;
     }

void TCPSender::fill_window(bool positive) {
    size_t payload_size=1;
    while(_next_seqno<=_winright&&payload_size>0) //Keep sending
    {
        TCPSegment tcpseg;
        if(!syn_sent&&positive) //we need to send SYN to build the connection between receiver,That is the begin of _stream
        {
            tcpseg.header().syn=true;
            payload_size=0; 
            syn_sent=true;
        }
        else
        {   
            payload_size=min(TCPConfig::MAX_PAYLOAD_SIZE,min(_winright-_next_seqno+1,_stream.buffer_size()));         
            tcpseg.payload()=Buffer(_stream.read(payload_size));

            if(!fin_sent&&_stream.eof()&&payload_size<=min(TCPConfig::MAX_PAYLOAD_SIZE,_winright-_next_seqno))
            {  //piggyback a fin if necessary.
               tcpseg.header().fin=true; 
               fin_sent=true;
            }                             
        }
        tcpseg.header().seqno=wrap(_next_seqno,_isn);
        _next_seqno+=tcpseg.length_in_sequence_space();
        _bytes_in_flight+=tcpseg.length_in_sequence_space();
        if(tcpseg.length_in_sequence_space()>0)
        {
            clock_running=true;
            no_backoff=false;
            outstanding_segments.push_back(tcpseg); //record the outstanding TCPsegment for retransmission.
            _segments_out.push(tcpseg);  //send the segment.
        }
    }
}
void TCPSender::fill_window()
{
    fill_window(true);
} 
//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
     uint64_t absolute_ackno=unwrap(ackno,_isn,_next_seqno); //_next_seqno,we hope, will be the proper checkpoint to unwrap.
     if(absolute_ackno>_next_seqno) //Impossible ackno (beyond next seqno) is ignored
       return ;
     bool ACKed=false;
     _winleft=absolute_ackno;   //only by acknowledging at least one segment,can we update the window.
     _winright=_winleft+window_size-1;
     uint64_t absolute_oldseqno;
     while(!outstanding_segments.empty())
     {
         absolute_oldseqno=unwrap(outstanding_segments.front().header().seqno,_isn,absolute_ackno);
         if(absolute_oldseqno+outstanding_segments.front().length_in_sequence_space()<=absolute_ackno)
         {
            _bytes_in_flight-=outstanding_segments.front().length_in_sequence_space();
            outstanding_segments.pop_front();//remove already fully acknowledged outstanding segments.
            ACKed=true;
         }
         else
          break;
     }
     if(outstanding_segments.empty())
     {
       RTO=_initial_retransmission_timeout;
       time_left=RTO;
       _consecutive_retransmissions=0;
       clock_running=false;
     }
     else if(ACKed)
     {
        RTO=_initial_retransmission_timeout;
        time_left=RTO;
        clock_running=true;
        _consecutive_retransmissions=0;
     }  
     if(_winright>=_next_seqno)
        fill_window();
     else if(outstanding_segments.empty())//Keep sending a single byte until more window space are opened
     {  
        TCPSegment tcpseg;
        tcpseg.header().seqno=wrap(_next_seqno,_isn);
        if(_stream.buffer_size()>0)
            tcpseg.payload()=Buffer(_stream.read(1));    //provoke the receiver into sending a new acknowledgment segment
        else if(_stream.eof())                         //where it reveals that more space has opened up in its window.
        {                                    
            tcpseg.header().fin=true;   
            fin_sent=true;
        }
        _next_seqno+=tcpseg.length_in_sequence_space();
        if(tcpseg.length_in_sequence_space()>0)          
        {                                     
            _bytes_in_flight+=tcpseg.length_in_sequence_space();
            no_backoff=true;
            outstanding_segments.push_front(tcpseg);   //push front, so it can be retransmitted again and again. 
            clock_running=true;  //Don't forget to run this clock.                                          
            _segments_out.push(tcpseg);
        }                                
     }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
     if(!clock_running) //if the clock is not running,tick method do nothing.
        return ;
     if(time_left>ms_since_last_tick)
     time_left-=ms_since_last_tick;
     else  //timeout occurs,we need to retransmit.
     {
        if(!outstanding_segments.empty()) //This condition should be always true,if the clock is running.
        {
            _segments_out.push(outstanding_segments.front());
            _consecutive_retransmissions++;
            if(!no_backoff)
            {
                RTO*=2;
            }
            time_left=RTO;
            clock_running=true;
        }
     }
}

unsigned int TCPSender::consecutive_retransmissions() const {
     return _consecutive_retransmissions; 
}

void TCPSender::send_empty_segment() {
    TCPSegment empty_segment;
    empty_segment.header().seqno=wrap(_next_seqno,_isn);
    _segments_out.push(empty_segment);
}

