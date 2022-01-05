#include "stream_reassembler.hh"
// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), 
_capacity(capacity),buffer(new char[_capacity]),first_unassembled(0),_eof(false),stored() {}
//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if(data.empty())  //Handle empty string.
    {
       if(eof)
       {
        _eof=true;
        if(stored.empty()&&_eof)
           _output.end_input();
       }
       return ;
    }
    if(index+data.length()-1<first_unassembled)  //ignore already assembled string.
       return ;
    list<pair<size_t,size_t>>::iterator pre;
    list<pair<size_t,size_t>>::iterator last;   
    size_t maxoffront=max(index,first_unassembled);   
    size_t minoftail=min(index+data.length()-1,_output.getFirstUnacc()-1);
    if(minoftail<maxoffront)
       return ;
    auto it=stored.begin();
    while(it!=stored.end()&&maxoffront>=(*it).first)  //find the right position to store the node.
        it++;
    if(it==stored.begin())        //Handle border case i.e. when there is no pre.By creating a pseudo pre.
    {
        stored.push_front(pair<size_t,size_t>(maxoffront,maxoffront));
        pre=stored.begin();
    }
    else
       pre=--it;
    it=stored.begin();
    while(it!=stored.end()&&minoftail>=(*it).second)
        it++;
    if(it==stored.end())    //Handle border case when there is no last.By creating a pseudo last.
    {
        stored.push_back(pair<size_t,size_t>(minoftail,minoftail));
        last=stored.end();
        last--;
    }
    else
        last=it; 
    for(size_t i=maxoffront;i<=minoftail;i++)
       buffer.get()[i%_capacity]=data[i-index];
    if(maxoffront>(*pre).second+1)
    {  
        pre++;
        if(pre!=last||(*last).first==0||minoftail>=(*last).first-1)
           (*pre).first=maxoffront;
    } 
    if((*last).first==0||minoftail>=(*last).first-1) //removing nodes from last backward,take care of overflow case!!!
    {
        (*pre).second=(*last).second;
        while(last!=pre)
        {   
            last=stored.erase(last);
            last--;
        }
    }
    else           //removing nodes from last-1 backward
    {   
        if(pre==last)  //special case.
        {
           stored.insert(pre,pair<size_t,size_t>(maxoffront,minoftail));
        }
        else
        {
            last--;
            (*pre).second=minoftail;
            while(last!=pre)
            {
                last=stored.erase(last);
                last--;
            }
        }
    }
    if(minoftail==index+data.length()-1&&eof) //Indicates that the last byte of the whole stream has been received.
      _eof=true;
    //Now the information has been stored,it's time to write as soon as possible
    if(!stored.empty()&&(*stored.begin()).first==first_unassembled)
       {
         string temp;
         size_t len=(*stored.begin()).second-(*stored.begin()).first+1;
         for(size_t i=(*stored.begin()).first;i<=(*stored.begin()).second;i++)
            temp.push_back(buffer.get()[i%_capacity]);
         _output.write(temp);
         first_unassembled+=len;
         stored.erase(stored.begin());
         if(stored.empty()&&_eof)  //The reassembler has finished its work !
           _output.end_input();
       }
}

size_t StreamReassembler::unassembled_bytes() const { 
     size_t count=0;
     for(auto it=stored.cbegin();it!=stored.cend();it++)
              count+=(*it).second-(*it).first+1;
     return count;         
 }

bool StreamReassembler::empty() const { 
    return stored.empty();
}
size_t StreamReassembler::getFirstUnassembled() const
{
    return first_unassembled;
}
size_t StreamReassembler::getFirstUnacc() const
{
    return _output.getFirstUnacc();
}