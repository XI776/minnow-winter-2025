#include "byte_stream.hh"
#include "iostream"
using namespace std;

ByteStream::ByteStream( uint64_t capacity) : capacity_( capacity ), buffer() {}

void Writer::push( string data )
{
  // (void)data; // Your code here.
  uint64_t space = available_capacity();
  if (!space)
    return;
  uint64_t size_ = (data.size() > space) ? space:  uint64_t(data.size());
  // buffer = buffer.substr(0, cnt) + data.substr(0, size_);
  buffer.append(data.substr(0, size_));

  index += size_;
  cnt += size_;
  total_pushed += size_;
  // uint64_t space = available_capacity();
  // uint64_t size_ = (data.size() > capacity_) ? capacity_:  uint64_t(data.size());
  // // buffer = buffer.substr(0, cnt) + data.substr(0, size_);
  // if(data.size() > capacity_) {
  //   buffer = data.substr(0, size_);
  // }
  // else {
  //   buffer.append(data.substr(0, size_));
  // }
  

  // // index += size_;
  // cnt += size_;
  // total_pushed += size_;

}

void Writer::close()
{
  // Your code here.
  
  // if(cnt == 0)
    closed_ = true;
}

bool Writer::is_closed() const
{
  return closed_; // Your code here.
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - cnt; // Your code here.
}

uint64_t Writer::bytes_pushed() const
{
  return total_pushed; // Your code here.
}

string_view Reader::peek() const
{
  // void read( Reader& reader, uint64_t max_len, std::string& out );

  // return {data[index]}; // Your code here.
  return std::string_view(buffer);
}

void Reader::pop( uint64_t len )
{
  // (void)len; // Your code here.
  uint64_t size_ = (cnt < len) ? cnt : len;
  total_poped += size_;
  cnt -= size_;
  // index += size_;
  buffer = buffer.substr(size_);

}

bool Reader::is_finished() const
{
  // std::cerr<< "closed_ = " << closed_ << "cnt = " << cnt << '\n';
  // std::cerr << "is_finished = " << (closed_ && !cnt) << '\n';
  return closed_ && !cnt; 
}

uint64_t Reader::bytes_buffered() const
{
  return cnt;
}

uint64_t Reader::bytes_popped() const
{
  return total_poped; // Your code here.
}

