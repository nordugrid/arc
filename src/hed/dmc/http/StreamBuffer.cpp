// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>

#include "StreamBuffer.h"

namespace ArcDMCHTTP {

using namespace Arc;

  StreamBuffer::StreamBuffer(DataBuffer& buffer):buffer_(buffer) {
    buffer_handle_ = -1;
    buffer_length_ = 0;
    buffer_offset_ = 0;
    current_offset_ = 0;
    current_size_ = 0;
  }

  StreamBuffer::~StreamBuffer(void) {
    if(buffer_handle_ >= 0) {
      buffer_.is_notwritten(buffer_handle_); buffer_handle_ = -1;
    };
  }

  bool StreamBuffer::Get(char* buf,int& size) {
    if(buffer_handle_ < 0) {
      // buffer need to be obtained 
      bool r = buffer_.for_write(buffer_handle_,buffer_length_,buffer_offset_,true);
      if(!r) return false;
      if(buffer_offset_ != current_offset_) {
        buffer_.is_notwritten(buffer_handle_); buffer_handle_ = -1;
        buffer_.error_write(true);
        return false;
      };
    };
    // buffer is already obtained
    unsigned long long int bufend = buffer_offset_ + buffer_length_;
    unsigned long long int bufsize = bufend - current_offset_;
    if(bufend > current_size_) current_size_ = bufend;
    if(bufsize > size) bufsize = size;
    ::memcpy(buf,buffer_[buffer_handle_],bufsize);
    size = bufsize; current_offset_ += bufsize;
    if(current_offset_ >= bufend) {
      buffer_.is_written(buffer_handle_); buffer_handle_ = -1;
    }
    return true;
  }

  bool StreamBuffer::Put(const char* buf,Size_t size) {
    // This implementation is unidirectonal (yet?)
    return false;
  }

  StreamBuffer::operator bool(void) {
    return (bool)buffer_;
  }

  bool StreamBuffer::operator!(void) {
    return !(bool)buffer_;
  }

  int StreamBuffer::Timeout(void) const {
    return -1;
  }

  void StreamBuffer::Timeout(int /*to*/) {
  }

  PayloadStreamInterface::Size_t StreamBuffer::Pos(void) const {
    return (PayloadStreamInterface::Size_t)current_offset_;
  }

  PayloadStreamInterface::Size_t StreamBuffer::Size(void) const {
    return (PayloadStreamInterface::Size_t)current_size_;
  }
  PayloadStreamInterface::Size_t StreamBuffer::Limit(void) const {
    return (PayloadStreamInterface::Size_t)current_size_;
  }

  void StreamBuffer::Size(PayloadStreamInterface::Size_t size) {
    if(size > current_size_) current_size_ = size;
  }

} // namespace ArcDMCHTTP

