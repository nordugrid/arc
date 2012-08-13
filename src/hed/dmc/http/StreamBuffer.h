// -*- indent-tabs-mode: nil -*-

#ifndef __ARCDMCHTTP_STREAMBUFFER_H__
#define __ARCDMCHTTP_STREAMBUFFER_H__

#include <arc/message/PayloadStream.h>
#include <arc/data/DataBuffer.h>

namespace ArcDMCHTTP {

using namespace Arc;

class StreamBuffer: public PayloadStreamInterface {
 public:
  StreamBuffer(DataBuffer& buffer);
  virtual ~StreamBuffer(void);
  virtual bool Get(char* buf,int& size);
  virtual bool Put(const char* buf,Size_t size);
  virtual operator bool(void);
  virtual bool operator!(void);
  virtual int Timeout(void) const;
  virtual void Timeout(int to);
  virtual Size_t Pos(void) const;
  virtual Size_t Size(void) const;
  virtual Size_t Limit(void) const;
  void Size(Size_t size);
 private:
  DataBuffer& buffer_;
  int buffer_handle_;
  unsigned int buffer_length_;
  unsigned long long int buffer_offset_;
  unsigned long long int current_offset_;
  unsigned long long int current_size_;
};

} // namespace ArcDMCHTTP

#endif // __ARCDMCHTTP_STREAMBUFFER_H__

