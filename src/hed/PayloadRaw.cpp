#include "PayloadRaw.h"

namespace Arc {

PayloadRaw::~PayloadRaw(void) {
  for(std::vector<PayloadRaw::Buf>::iterator b = buf_.begin();b!=buf_.end();++b) {
    if(b->allocated) free(b->data);
  };
};

static bool BufferAtPos(const std::vector<PayloadRaw::Buf>& buf_,int pos,int& bufnum,int& bufpos) {
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

static bool BufferAtPos(std::vector<PayloadRaw::Buf>& buf_,int pos,std::vector<PayloadRaw::Buf>::iterator& bufref,int& bufpos) {
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
  int bufnum;
  int bufpos;
  if(!BufferAtPos(buf_,pos,bufnum,bufpos)) return NULL;
  return buf_[bufnum].data+bufpos;
}

char PayloadRaw::operator[](int pos) const {
  int bufnum;
  int bufpos;
  if(!BufferAtPos(buf_,pos,bufnum,bufpos)) return 0;
  return buf_[bufnum].data[bufpos];
}

int PayloadRaw::Size(void) const {
  int cpos = 0;
  for(int bufnum = 0;bufnum<buf_.size();bufnum++) {
    cpos+=buf_[bufnum].length;
  };
  return cpos;
}

char* PayloadRaw::Insert(int pos,int size) {
  std::vector<PayloadRaw::Buf>::iterator bufref;
  int bufpos;
  if(!BufferAtPos(buf_,pos,bufref,bufpos)) {
    bufref=buf_.end(); bufpos=0;
  };
  PayloadRaw::Buf buf;
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
  return buf.data;
}

char* PayloadRaw::Insert(const char* s,int pos,int size) {
  if(size <= 0) size=strlen(s);
  char* s_ = Insert(pos,size);
  if(s_) memcpy(s_,s,size);
  return s_;
}

char* PayloadRaw::Buffer(int num) {
  if((num<0) || (num>=buf_.size())) return NULL;
  return buf_[num].data;
}

int PayloadRaw::BufferSize(int num) const {
  if((num<0) || (num>=buf_.size())) return 0;
  return buf_[num].length;
}

const char* ContentFromPayload(const MessagePayload& payload) {
  try {
    const PayloadRawInterface& buffer = dynamic_cast<const PayloadRawInterface&>(payload);
    return ((PayloadRawInterface&)buffer).Content();
  } catch(std::exception& e) { };
  return "";
}

} // namespace Arc
