#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>
#include <iostream>

#include "PayloadRaw.h"

namespace Arc {

PayloadRaw::~PayloadRaw(void) {
  for(std::vector<PayloadRawBuf>::iterator b = buf_.begin();b!=buf_.end();++b) {
    if(b->allocated) free(b->data);
  };
}

static bool BufferAtPos(const std::vector<PayloadRawBuf>& buf_,int pos,unsigned int& bufnum,int& bufpos) {
  if(pos == -1) pos=0;
  if(pos < 0) return false;
  int cpos = 0;
  for(bufnum = 0;bufnum<buf_.size();++bufnum) {
    cpos+=buf_[bufnum].length;
    if(cpos>pos) {
      bufpos=pos-(cpos-buf_[bufnum].length);
      return true;
    };
  };
  return false;
}

static bool BufferAtPos(std::vector<PayloadRawBuf>& buf_,int pos,std::vector<PayloadRawBuf>::iterator& bufref,int& bufpos) {
  if(pos == -1) pos=0;
  if(pos < 0) return false;
  int cpos = 0;
  for(bufref = buf_.begin();bufref!=buf_.end();++bufref) {
    cpos+=bufref->length;
    if(cpos>pos) {
      bufpos=pos-(cpos-bufref->length);
      return true;
    };
  };
  return false;
}

char* PayloadRaw::Content(int pos) {
  unsigned int bufnum;
  int bufpos;
  if(!BufferAtPos(buf_,pos-offset_,bufnum,bufpos)) return NULL;
  return buf_[bufnum].data+bufpos;
}

char PayloadRaw::operator[](int pos) const {
  unsigned int bufnum;
  int bufpos;
  if(!BufferAtPos(buf_,pos-offset_,bufnum,bufpos)) return 0;
  return buf_[bufnum].data[bufpos];
}

int PayloadRaw::Size(void) const {
  return size_;
  //int cpos = 0;
  //for(unsigned int bufnum = 0;bufnum<buf_.size();bufnum++) {
  //  cpos+=buf_[bufnum].length;
  //};
  //return cpos;
}

char* PayloadRaw::Insert(int pos,int size) {
  std::vector<PayloadRawBuf>::iterator bufref;
  int bufpos;
  if(!BufferAtPos(buf_,pos-offset_,bufref,bufpos)) {
    bufref=buf_.end(); bufpos=0;
    if(buf_.size() == 0) {
      offset_=pos;
    } else {
      pos = 0;
      for(unsigned int bufnum = 0;bufnum<buf_.size();bufnum++) {
        pos+=buf_[bufnum].length;
      };
    };
  };
  PayloadRawBuf buf;
  if(bufpos != 0) {
    // Need to split buffers
    buf.size=bufref->length - bufpos;
    buf.data=(char*)malloc(buf.size+1);
    if(!buf.data) return NULL;
    buf.data[buf.size]=0;
    memcpy(buf.data,bufref->data+bufpos,buf.size);
    buf.length=buf.size;
    buf.allocated=true;
    bufref->length=bufpos;
    bufref->data[bufref->length]=0;
    ++bufref;
    bufref=buf_.insert(bufref,buf);
  };
  // Inserting between buffers
  buf.data=(char*)malloc(size+1);
  if(!buf.data) return NULL;
  buf.data[size]=0;
  buf.size=size;
  buf.length=size;
  buf.allocated=true;
  buf_.insert(bufref,buf);
  if((pos+size) > size_) size_=pos+size;
  return buf.data;
}

char* PayloadRaw::Insert(const char* s,int pos,int size) {
  if(size <= 0) size=strlen(s);
  char* s_ = Insert(pos,size);
  if(s_) memcpy(s_,s,size);
  return s_;
}

char* PayloadRaw::Buffer(unsigned int num) {
  if(num>=buf_.size()) return NULL;
  return buf_[num].data;
}

int PayloadRaw::BufferSize(unsigned int num) const {
  if(num>=buf_.size()) return 0;
  return buf_[num].length;
}

int PayloadRaw::BufferPos(unsigned int num) const {
  int pos = offset_;
  std::vector<PayloadRawBuf>::const_iterator b = buf_.begin();
  for(;b!=buf_.end();++b) {
    if(!num) break;
    pos+=(b->length);
  };
  return pos;
}

bool PayloadRaw::Truncate(unsigned int size) {
  if(size_ == size) return true; // Buffer is already of right size
  if(size_ < size) { // Buffer needs to be extended
    size_=size; return true;
  };
  if(size <= offset_) {
    // All buffers must be released
    offset_=size;
    for(std::vector<PayloadRawBuf>::iterator b = buf_.begin();b!=buf_.end();) {
      if(b->allocated) free(b->data);
      b=buf_.erase(b);
    };
    size_=size;
    return true;
  };
  int l = offset_;
  for(unsigned int bufnum = 0;bufnum<buf_.size();bufnum++) {
    l+=buf_[bufnum].length;
  };
  if(l == size) {
    size_=size; return true;
  };
  // Buffer must be truncated
  std::vector<PayloadRawBuf>::iterator b;
  int p;
  if(!BufferAtPos(buf_,size-offset_,b,p)) return false;
  if(p != 0) {
    b->length=p; ++b;
  };
  for(;b!=buf_.end();) {
    if(b->allocated) free(b->data);
    b=buf_.erase(b);
  };
  size_=size;
  return true;
}

const char* ContentFromPayload(const MessagePayload& payload) {
  try {
    const PayloadRawInterface& buffer = dynamic_cast<const PayloadRawInterface&>(payload);
    return ((PayloadRawInterface&)buffer).Content();
  } catch(std::exception& e) { };
  return "";
}

} // namespace Arc
