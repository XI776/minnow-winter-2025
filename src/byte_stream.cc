#include "byte_stream.hh"
#include "iostream"
#include <cstring>
using namespace std;

ByteStream::ByteStream( uint64_t capacity) 
 : capacity_(capacity),
    error_(false),
    buffer_(capacity),   // 直接初始化为 capacity 大小
    size_(0),
    head_(0),
    tail_(0),
    copy_mark_(capacity / 2 + 1), // 可以设成一半，避免太频繁移动
    closed_(false),
    total_pushed_(0),
    total_poped_(0)
{}

void Writer::push( string data )
{
  uint64_t size = std::min(available_capacity(), data.size());
  if(size == 0)
    return;
  
  if(size + tail_ > capacity_) {
    std::memmove(buffer_.data(), buffer_.data() + head_, size_);
    tail_ -= head_;
    head_ = 0;
  }
  for(size_t i = 0; i < size; i++) {
    buffer_[tail_ + i] = data[i];
  }
  tail_ += size;
  size_ += size;
  total_pushed_ += size;
}

void Writer::close()
{
  closed_ = true;
}

bool Writer::is_closed() const
{
  return closed_; // Your code here.
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - size_;
}

uint64_t Writer::bytes_pushed() const
{
  return total_pushed_; // Your code here.
}

string_view Reader::peek() const
{
  return std::string_view(buffer_.data() + head_, size_);
}

void Reader::pop( uint64_t len )
{
  len = std::min(len, size_);
  head_ += len;
  size_ -= len;
  
  if(head_ >= copy_mark_) {
    std::memmove(buffer_.data(), buffer_.data() + head_, size_);
    tail_ -= head_;
    head_ = 0;
  }
  
  total_poped_ += len;

}

bool Reader::is_finished() const
{
  return closed_ && !size_; 
}

uint64_t Reader::bytes_buffered() const
{
  return size_;
}

uint64_t Reader::bytes_popped() const
{
  return total_poped_; // Your code here.
}

