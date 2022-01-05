#include "byte_stream.hh"
// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t cap): front(0),tail(0),total_read(0),total_write(0),capacity(cap+1),
buffer(new char[capacity]),first_unread(0),first_unacceptable(cap){ 
     //for circular queue,we spare one slot to distinguish full from empty. 
}
size_t ByteStream::write(const string &data) {
   size_t write_count=0;
   while(((tail+write_count+1)%capacity)!=front&&write_count<data.length())
   {
    buffer.get()[(tail+write_count)%capacity]=data[write_count];
    write_count++;
   }
   tail=(tail+write_count)%capacity;
   total_write+=write_count;
   return write_count;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string str;
    size_t safe_len=min(len,buffer_size());
    for(size_t i=0;i<safe_len;i++)
      str.push_back(buffer.get()[(front+i)%capacity]);
    return str;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t safe_len=min(len,buffer_size());
    front=(front+safe_len)%capacity;
    total_read+=safe_len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string str=peek_output(len);
    pop_output(len);
    first_unread+=len;
    first_unacceptable+=len;
    return str;
}

void ByteStream::end_input() {
    _eofin=true;
}

bool ByteStream::input_ended() const { return _eofin; }

size_t ByteStream::buffer_size() const { return (tail-front+capacity)%capacity;}

bool ByteStream::buffer_empty() const { return tail==front; }

bool ByteStream::eof() const { return _eofin&&(total_read==total_write); }

size_t ByteStream::bytes_written() const { return total_write; }

size_t ByteStream::bytes_read() const { return total_read; }

size_t ByteStream::remaining_capacity() const { return (front+capacity-1-tail)%capacity;}

size_t ByteStream::getFirstUnacc() const {return first_unacceptable;}