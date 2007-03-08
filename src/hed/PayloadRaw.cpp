#include "PayloadRaw.h"

DataPayloadRaw::~DataPayloadRaw(void) {
  for(std::vector<DataPayloadRaw::Buf>::iterator b = buf_.begin();b!=buf_.end();++b) {
    if(b->allocated) free(b->data);
  };
};

static bool BufferAtPos(const std::vector<DataPayloadRaw::Buf>& buf_,int pos,int& bufnum,int& bufpos) {
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

static bool BufferAtPos(std::vector<DataPayloadRaw::Buf>& buf_,int pos,std::vector<DataPayloadRaw::Buf>::iterator& bufref,int& bufpos) {
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

char* DataPayloadRaw::Content(int pos) {
  int bufnum;
  int bufpos;
  if(!BufferAtPos(buf_,pos,bufnum,bufpos)) return NULL;
  return buf_[bufnum].data+bufpos;
}

char DataPayloadRaw::operator[](int pos) const {
  int bufnum;
  int bufpos;
  if(!BufferAtPos(buf_,pos,bufnum,bufpos)) return 0;
  return buf_[bufnum].data[bufpos];
}

int DataPayloadRaw::Size(void) const {
  int cpos = 0;
  for(int bufnum = 0;bufnum<buf_.size();bufnum++) {
    cpos+=buf_[bufnum].length;
  };
  return cpos;
}

char* DataPayloadRaw::Insert(int pos,int size) {
  std::vector<DataPayloadRaw::Buf>::iterator bufref;
  int bufpos;
  if(!BufferAtPos(buf_,pos,bufref,bufpos)) {
    bufref=buf_.end(); bufpos=0;
  };
  DataPayloadRaw::Buf buf;
  if(bufpos != 0) {
    // Need to split buffers
    buf.size=bufref->length - bufpos;
    buf.data=(char*)malloc(buf.size);
    if(!buf.data) return NULL;
    memcpy(buf.data,bufref->data+bufpos,buf.size);
    buf.length=buf.size;
    buf.allocated=true;
    bufref->length=bufpos;
    ++bufref;
    bufref=buf_.insert(bufref,buf);
  };
  // Inserting between buffers
  buf.data=(char*)malloc(size);
  if(!buf.data) return NULL;
  buf.size=size;
  buf.length=size;
  buf.allocated=true;
  buf_.insert(bufref,buf);
  return buf.data;
}

char* DataPayloadRaw::Insert(const char* s,int pos,int size) {
  if(size <= 0) size=strlen(s);
  char* s_ = Insert(pos,size);
  if(s_) memcpy(s_,s,size);
  return s_;
}

char* DataPayloadRaw::Buffer(int num) {
  if((num<0) || (num>=buf_.size())) return NULL;
  return buf_[num].data;
}

int DataPayloadRaw::BufferSize(int num) const {
  if((num<0) || (num>=buf_.size())) return 0;
  return buf_[num].length;
}

const char* ContentFromPayload(const DataPayload& payload) {
  try {
    const DataPayloadRaw& buffer = dynamic_cast<const DataPayloadRaw&>(payload);
    return ((DataPayloadRaw&)buffer).Content();
  } catch(std::exception& e) { };
  return "";
}

