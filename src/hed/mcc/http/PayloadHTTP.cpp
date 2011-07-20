// The code in this file is quate a mess. It is subject to cleaning 
// and simplification as soon as possible. Functions must be simplified
// and functionality to be split into more functions.
// Some methods of this class must be called in proper order to have
// it function properly. A lot of proptections to be added. 

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include "PayloadHTTP.h"
#include <arc/StringConv.h>

namespace Arc {

static std::string empty_string("");

static bool ParseHTTPVersion(const std::string& s,int& major,int& minor) {
  major=0; minor=0;
  const char* p = s.c_str();
  if(strncasecmp(p,"HTTP/",5) != 0) return false;
  p+=5;
  char* e;
  major=strtol(p,&e,10);
  if(*e != '.') { major=0; return false; };
  p=e+1;
  minor=strtol(p,&e,10);
  if(*e != 0) { major=0; minor=0; return false; };
  return true;
}

// Assuming there are no zeroes in stream
bool PayloadHTTP::readline(std::string& line) {
  line.resize(0);
  for(;;) {
    char* p = strchr(tbuf_,'\n');
    if(p) {
      *p=0;
      line+=tbuf_;
      tbuflen_-=(p-tbuf_)+1;
      memmove(tbuf_,p+1,tbuflen_+1); 
      if(line[line.length()-1] == '\r') line.resize(line.length()-1);
      return true;
    };
    line+=tbuf_;
    tbuflen_=sizeof(tbuf_)-1;
    if(!stream_->Get(tbuf_,tbuflen_)) break;
    tbuf_[tbuflen_]=0;
  }; 
  tbuf_[tbuflen_]=0;
  return false;
}

bool PayloadHTTP::parse_header(void) {
  method_.resize(0);
  code_=0;
  keep_alive_=false;
  // Skip empty lines
  std::string line;
  for(;line.empty();) if(!readline(line)) {
    method_="END";  // Special name to repsent closed connection
    chunked_=false; length_=0;
    return true;
  };
  // Parse request/response line
  std::string::size_type pos2 = line.find(' ');
  if(pos2 == std::string::npos) return false;
  std::string word1 = line.substr(0,pos2);
  // Identify request/response
  if(ParseHTTPVersion(line.substr(0,pos2),version_major_,version_minor_)) {
    // Response
    std::string::size_type pos3 = line.find(' ',pos2+1);
    if(pos3 == std::string::npos) return false;
    code_=strtol(line.c_str()+pos2+1,NULL,10);
    reason_=line.substr(pos3+1);
  } else {
    // Request
    std::string::size_type pos3 = line.rfind(' ');
    if((pos3 == pos2) || (pos2 == std::string::npos)) return false;
    if(!ParseHTTPVersion(line.substr(pos3+1),version_major_,version_minor_)) return false;
    method_=line.substr(0,pos2);
    uri_=line.substr(pos2+1,pos3-pos2-1);
  };
  if((version_major_ > 1) || ((version_major_ == 1) && (version_minor_ >= 1))) {
    keep_alive_=true;
  };
  // Parse header lines
  for(;readline(line) && (!line.empty());) {
    std::string::size_type pos = line.find(':');
    if(pos == std::string::npos) continue;
    std::string name = line.substr(0,pos);
    for(++pos;pos<line.length();++pos) if(!isspace(line[pos])) break;
    if(pos<line.length()) {
      std::string value = line.substr(pos);
      // for(std::string::size_type p=0;p<value.length();++p) value[p]=tolower(value[p]);
      Attribute(name,value);
    } else {
      Attribute(name,"");
    };
  };
  length_=-1; chunked_=false;
  std::map<std::string,std::string>::iterator it;
  it=attributes_.find("content-length");
  if(it != attributes_.end()) {
    length_=strtoll(it->second.c_str(),NULL,10);
  };
  it=attributes_.find("content-range");
  if(it != attributes_.end()) {
    const char* token = it->second.c_str();
    const char* p = token; for(;*p;p++) if(isspace(*p)) break;
    int64_t range_start,range_end,entity_size;
    if(strncasecmp("bytes",token,p-token) == 0) {
      for(;*p;p++) if(!isspace(*p)) break;
      char *e;
      range_start=strtoull(p,&e,10);
      if((*e) == '-') {
        p=e+1; range_end=strtoull(p,&e,10); p=e;
        if(((*e) == '/') || ((*e) == 0)) {
          if(range_start <= range_end) {
            offset_=range_start;
          };
          if((*p) == '/') {
            p++; entity_size=strtoull(p,&e,10);
            if((*e) == 0) {
              size_=entity_size;
            };
          };
        };
      };
    };
  };
  it=attributes_.find("transfer-encoding");
  if(it != attributes_.end()) {
    if(strcasecmp(it->second.c_str(),"chunked") != 0) {
      // Non-implemented encoding
      return false;
    };
    chunked_=true;
  };
  it=attributes_.find("connection");
  if(it != attributes_.end()) {
    if(strcasecmp(it->second.c_str(),"keep-alive") == 0) {
      keep_alive_=true;
    } else {
      keep_alive_=false;
    };
  };
  // In case of keep_alive (HTTP1.1) there must be length specified
  if(keep_alive_ && (length_ == -1)) length_=0;
  // If size of object was not reported then try to deduce it.
  if((size_ == 0) && (length_ != -1)) size_=offset_+length_;
  return true;
}

bool PayloadHTTP::read(char* buf,int64_t& size) {
//char* buff = buf;
//memset(buf,0,size);
  if(tbuflen_ >= size) {
    memcpy(buf,tbuf_,size);
    memmove(tbuf_,tbuf_+size,tbuflen_-size+1);
    tbuflen_-=size;
  } else {
    memcpy(buf,tbuf_,tbuflen_);
    buf+=tbuflen_; 
    int64_t l = size-tbuflen_;
    size=tbuflen_; tbuflen_=0; tbuf_[0]=0; 
    for(;l;) {
      int l_ = (l>INT_MAX)?INT_MAX:l;
      if(!stream_->Get(buf,l_)) return (size>0);
      size+=l_; buf+=l_; l-=l_;
    };
  };
  return true;
}

bool PayloadHTTP::get_body(void) {
  if(fetched_) return true; // Already fetched body
  fetched_=true; // Even attempt counts
  valid_=false; // But object is invalid till whole body is available
  // TODO: Check for methods and responses which can't have body
  char* result = NULL;
  int64_t result_size = 0;
  if(chunked_) {
    for(;;) {
      std::string line;
      if(!readline(line)) return false;
      char* e;
      int64_t chunk_size = strtoll(line.c_str(),&e,16);
      if((*e != ';') && (*e != 0)) { free(result); return false; };
      if(e == line.c_str()) { free(result); return false; };
      if(chunk_size == 0) break;
      char* new_result = (char*)realloc(result,result_size+chunk_size+1);
      if(new_result == NULL) { free(result); return false; };
      result=new_result;
      if(!read(result+result_size,chunk_size)) { free(result); return false; };
      result_size+=chunk_size;
      if(!readline(line)) return false;
      if(!line.empty()) return false;
    };
  } else if(length_ == 0) {
    valid_=true;
    return true;
  } else if(length_ > 0) {
    result=(char*)malloc(length_+1);
    if(!read(result,length_)) { free(result); return false; };
    result_size=length_;
  } else {
    // Read till connection closed
    for(;;) {
      int64_t chunk_size = 4096;
      char* new_result = (char*)realloc(result,result_size+chunk_size+1);
      if(new_result == NULL) { free(result); return false; };
      result=new_result;
      if(!read(result+result_size,chunk_size)) break;
      result_size+=chunk_size;
    };
  };
  if (result == NULL) {
    return false;
  }
  result[result_size]=0;
  // Attach result to buffer exposed to user
  PayloadRawBuf b;
  b.data=result; b.size=result_size; b.length=result_size; b.allocated=true;
  buf_.push_back(b);
  // If size of object was not reported then try to deduce it.
  if(size_ == 0) size_=offset_+result_size;
  valid_=true;
  return true;
}

const std::string& PayloadHTTP::Attribute(const std::string& name) {
  std::multimap<std::string,std::string>::iterator it = attributes_.find(name);
  if(it == attributes_.end()) return empty_string;
  return it->second;
}

const std::multimap<std::string,std::string>& PayloadHTTP::Attributes(void) {
  return attributes_;
}

void PayloadHTTP::Attribute(const std::string& name,const std::string& value) {
  attributes_.insert(std::pair<std::string,std::string>(lower(name),value));
}

PayloadHTTP::PayloadHTTP(PayloadStreamInterface& stream,bool own):
    valid_(false),fetched_(false),stream_(&stream),stream_own_(own),
    rbody_(NULL),sbody_(NULL),body_own_(false),keep_alive_(true),
    stream_offset_(0),chunked_size_(0),chunked_offset_(0),head_response_(false) {
  tbuf_[0]=0; tbuflen_=0;
  if(!parse_header()) return;
  // If stream_ is owned then body can be fetched later
  // if(!stream_own_) if(!get_body()) return;
  valid_=true;
}

PayloadHTTP::PayloadHTTP(const std::string& method,const std::string& url,PayloadStreamInterface& stream):
    valid_(true),fetched_(true),stream_(&stream),stream_own_(false),
    rbody_(NULL),sbody_(NULL),body_own_(false),uri_(url),method_(method),
    code_(0),chunked_(false),keep_alive_(true),stream_offset_(0),
    chunked_size_(0),chunked_offset_(0),head_response_(false) {
  tbuf_[0]=0; tbuflen_=0;
  version_major_=1; version_minor_=1;
  // TODO: encode URI properly
}

PayloadHTTP::PayloadHTTP(int code,const std::string& reason,PayloadStreamInterface& stream,bool head_response):
    valid_(true),fetched_(true),stream_(&stream),stream_own_(false),
    rbody_(NULL),sbody_(NULL),body_own_(false),code_(code),
    reason_(reason),chunked_(false),keep_alive_(true),stream_offset_(0),
    chunked_size_(0),chunked_offset_(0),head_response_(head_response) {
  tbuf_[0]=0; tbuflen_=0;
  version_major_=1; version_minor_=1;
  if(reason_.empty()) reason_="OK";
}

PayloadHTTP::PayloadHTTP(const std::string& method,const std::string& url):
    valid_(true),fetched_(true),stream_(NULL),stream_own_(false),
    rbody_(NULL),sbody_(NULL),body_own_(false),uri_(url),method_(method),
    code_(0),chunked_(false),keep_alive_(true),stream_offset_(0),
    chunked_size_(0),chunked_offset_(0),head_response_(false) {
  tbuf_[0]=0; tbuflen_=0;
  version_major_=1; version_minor_=1;
  // TODO: encode URI properly
}

PayloadHTTP::PayloadHTTP(int code,const std::string& reason,bool head_response):
    valid_(true),fetched_(true),stream_(NULL),stream_own_(false),
    rbody_(NULL),sbody_(NULL),body_own_(false),code_(code),
    reason_(reason),chunked_(false),keep_alive_(true),stream_offset_(0),
    chunked_size_(0),chunked_offset_(0),head_response_(head_response) {
  tbuf_[0]=0; tbuflen_=0;
  version_major_=1; version_minor_=1;
  if(reason_.empty()) reason_="OK";
}

PayloadHTTP::~PayloadHTTP(void) {
  if(rbody_ && body_own_) delete rbody_;
  if(sbody_ && body_own_) delete sbody_;
  if(stream_ && stream_own_) delete stream_;
}

bool PayloadHTTP::Flush(void) {
  std::string header;
  bool to_stream = (stream_ != NULL);
  if(method_.empty() && (code_ == 0)) return false;
  // Computing length of Body part
  length_=0;
  std::string range_header;
  if((method_ != "GET") && (method_ != "HEAD")) {
    int64_t start = 0;
    if(head_response_ && (code_==HTTP_OK)) {
      length_ = Size();
    } else {
      if(sbody_) {
        if(sbody_->Limit() > sbody_->Pos()) {
          length_ = sbody_->Limit() - sbody_->Pos();
        };
        start=sbody_->Pos();
      } else {
        for(int n=0;;++n) {
          if(Buffer(n) == NULL) break;
          length_+=BufferSize(n);
        };
        start=BufferPos(0);
      };
    };
    if(length_ != Size()) {
      // Add range definition if Body represents part of logical buffer size
      // and adjust HTTP code accordingly
      int64_t end = start+length_;
      std::string length_str;
      std::string range_str;
      if(end <= Size()) {
        length_str=tostring(Size());
      } else {
        length_str="*";
      }; 
      if(end > start) {
        range_str=tostring(start)+"-"+tostring(end-1);
        if(code_ == HTTP_OK) {
          code_=HTTP_PARTIAL;
          reason_="Partial content";
        };
      } else {
        range_str="*";
        if(code_ == HTTP_OK) {
          code_=HTTP_RANGE_NOT_SATISFIABLE;
          reason_="Range not satisfiable";
        };
      };
      range_header="Content-Range: bytes "+range_str+"/"+length_str+"\r\n";
    };
    range_header+="Content-Length: "+tostring(length_)+"\r\n";
  };
  // Starting header
  if(!method_.empty()) {
    header=method_+" "+uri_+
           " HTTP/"+tostring(version_major_)+"."+tostring(version_minor_)+"\r\n";
  } else if(code_ != 0) {
    char tbuf[256]; tbuf[255]=0;
    snprintf(tbuf,255,"HTTP/%i.%i %i",version_major_,version_minor_,code_);
    header="HTTP/"+tostring(version_major_)+"."+tostring(version_minor_)+" "+
           tostring(code_)+" "+reason_+"\r\n";
  };
  if((version_major_ == 1) && (version_minor_ == 1) && (!method_.empty())) {
    std::map<std::string,std::string>::iterator it = attributes_.find("host");
    if(it == attributes_.end()) {
      std::string host;
      if(!uri_.empty()) {
        std::string::size_type p1 = uri_.find("://");
        if(p1 != std::string::npos) {
          std::string::size_type p2 = uri_.find('/',p1+3);
          if(p2 == std::string::npos) p2 = uri_.length();
          host=uri_.substr(p1+3,p2-p1-3);
        };
      };
      header+="Host: "+host+"\r\n";
    };
  };
  // Adding previously generated range specifier
  header+=range_header;
  bool keep_alive = false;
  if((version_major_ == 1) && (version_minor_ == 1)) keep_alive=keep_alive_;
  if(keep_alive) {
    header+="Connection: keep-alive\r\n";
  } else {
    header+="Connection: close\r\n";
  };
  for(std::map<std::string,std::string>::iterator a = attributes_.begin();a!=attributes_.end();++a) {
    header+=(a->first)+": "+(a->second)+"\r\n";
  };
  header+="\r\n";
  if(to_stream) {
    if(!stream_->Put(header)) return false;
    if(length_ > 0) {
      if(sbody_) {
        // stream to stream transfer
        // TODO: choose optimal buffer size
        // TODO: parallel read and write for better performance
        int tbufsize = (length_>1024*1024)?(1024*1024):length_;
        char* tbuf = new char[tbufsize];
        if(!tbuf) return false;
        for(;;) {
          int lbuf = tbufsize;
          if(!sbody_->Get(tbuf,lbuf)) break;
          if(!stream_->Put(tbuf,lbuf)) { delete[] tbuf; return false; };
        };
        delete[] tbuf;
      } else {
        for(int n=0;;++n) {
          char* tbuf = Buffer(n);
          if(tbuf == NULL) break;
          int64_t lbuf = BufferSize(n);
          if(lbuf > 0) if(!stream_->Put(tbuf,lbuf)) return false;
        };
      };
    };
    //if(!keep_alive) stream_->Close();
  } else {
    Insert(header.c_str(),0,header.length());
  };
  return true;
}

void PayloadHTTP::Body(PayloadRawInterface& body,bool ownership) {
  if(rbody_ && body_own_) delete rbody_;
  if(sbody_ && body_own_) delete sbody_;
  sbody_ = NULL;
  rbody_=&body; body_own_=ownership;
}

void PayloadHTTP::Body(PayloadStreamInterface& body,bool ownership) {
  if(rbody_ && body_own_) delete rbody_;
  if(sbody_ && body_own_) delete sbody_;
  rbody_ = NULL;
  sbody_=&body; body_own_=ownership;
}

char PayloadHTTP::operator[](PayloadRawInterface::Size_t pos) const {
  if(!((PayloadHTTP*)this)->get_body()) return 0;
  if(pos < PayloadRaw::Size()) {
    return PayloadRaw::operator[](pos);
  };
  if(rbody_) {
    return rbody_->operator[](pos-Size());
  };
  if(sbody_) {
    // Not supporting direct read from stream body
  };
  return 0;
}

char* PayloadHTTP::Content(PayloadRawInterface::Size_t pos) {
  if(!get_body()) return NULL;
  if(pos < PayloadRaw::Size()) {
    return PayloadRaw::Content(pos);
  };
  if(rbody_) {
    return rbody_->Content(pos-Size());
  };
  if(sbody_) {
    // Not supporting content from stream body
  };
  return NULL;
}

PayloadRawInterface::Size_t PayloadHTTP::Size(void) const {
  if(!((PayloadHTTP*)this)->get_body()) return 0;
  if(rbody_) {
    return PayloadRaw::Size() + (rbody_->Size());
  };
  if(sbody_) {
    return PayloadRaw::Size() + (sbody_->Size());
  };
  return PayloadRaw::Size();
}

char* PayloadHTTP::Insert(PayloadRawInterface::Size_t pos,PayloadRawInterface::Size_t size) {
  if(!get_body()) return NULL;
  return PayloadRaw::Insert(pos,size);
}

char* PayloadHTTP::Insert(const char* s,PayloadRawInterface::Size_t pos,PayloadRawInterface::Size_t size) {
  if(!get_body()) return NULL;
  return PayloadRaw::Insert(s,pos,size);
}

char* PayloadHTTP::Buffer(unsigned int num) {
  if(!get_body()) return NULL;
  if(num < buf_.size()) {
    return PayloadRaw::Buffer(num);
  };
  if(rbody_) {
    return rbody_->Buffer(num-buf_.size());
  };
  if(sbody_) {
    // Not supporting buffer access to stream body
  };
  return NULL;
}

PayloadRawInterface::Size_t PayloadHTTP::BufferSize(unsigned int num) const {
  if(!((PayloadHTTP*)this)->get_body()) return 0;
  if(num < buf_.size()) {
    return PayloadRaw::BufferSize(num);
  };
  if(rbody_) {
    return rbody_->BufferSize(num-buf_.size());
  };
  if(sbody_) {
    // Not supporting buffer access to stream body
  };
  return 0;
}

PayloadRawInterface::Size_t PayloadHTTP::BufferPos(unsigned int num) const {
  if(!((PayloadHTTP*)this)->get_body()) return 0;
  if(num < buf_.size()) {
    return PayloadRaw::BufferPos(num);
  };
  if(rbody_) {
    return rbody_->BufferPos(num-buf_.size())+PayloadRaw::BufferPos(num);
  };
  if(sbody_) {
    // Not supporting buffer access to stream body
  };
  return PayloadRaw::BufferPos(num);
}

bool PayloadHTTP::Truncate(PayloadRawInterface::Size_t size) {
  if(!get_body()) return false;
  if(size < PayloadRaw::Size()) {
    if(rbody_ && body_own_) delete rbody_;
    if(sbody_ && body_own_) delete sbody_;
    rbody_=NULL; sbody_=NULL;
    return PayloadRaw::Truncate(size);
  };
  if(rbody_) {
    return rbody_->Truncate(size-Size());
  };
  if(sbody_) {
    // Stream body does not support Truncate yet
  };
  return false;
}

bool PayloadHTTP::Get(char* buf,int& size) {
  if(fetched_) {
    // Read from buffers
    uint64_t bo = 0;
    for(unsigned int num = 0;num<buf_.size();++num) {
      uint64_t bs = PayloadRaw::BufferSize(num);
      if((bo+bs) > stream_offset_) {
        char* p = PayloadRaw::Buffer(num);
        p+=(stream_offset_-bo);
        bs-=(stream_offset_-bo);
        if(bs>size) bs=size;
        memcpy(buf,p,bs);
        size=bs; stream_offset_+=bs;
        return true;
      };
      bo+=bs;
    };
    if(rbody_) {
      for(unsigned int num = 0;;++num) {
        char* p = PayloadRaw::Buffer(num);
        if(!p) break;
        uint64_t bs = PayloadRaw::BufferSize(num);
        if((bo+bs) > stream_offset_) {
          p+=(stream_offset_-bo);
          bs-=(stream_offset_-bo);
          if(bs>size) bs=size;
          memcpy(buf,p,bs);
          size=bs; stream_offset_+=bs;
          return true;
        };
        bo+=bs;
      };
    } else if(sbody_) {
      if(sbody_->Get(buf,size)) {
        stream_offset_+=size;
        return true;
      };
    };
    return false;
  };
    // TODO: Check for methods and responses which can't have body
  if(chunked_) {
    if(chunked_size_ == -1) { // chunked stream is over
      size=0;
      return false;
    };
    if(chunked_size_ == chunked_offset_) {
      // read chunk size
      std::string line;
      if(!readline(line)) return false;
      char* e;
      chunked_size_ = strtoll(line.c_str(),&e,16);
      if(((*e != ';') && (*e != 0)) || (e == line.c_str())) {
        chunked_size_=-1; // No more stream
        valid_=false; // Object becomes invalid
        size=0;
        return false;
      };
      chunked_offset_=0;
      if(chunked_size_ == 0) {
        // end of stream
        chunked_size_=-1; size=0;
        return false;
      };
    };
    // read chunk content
    int64_t bs = chunked_size_-chunked_offset_;
    if(bs > size) bs=size;
    if(!read(buf,bs)) { size=bs; return false; };
    chunked_offset_+=bs;
    size=bs; stream_offset_+=bs;
    return true;
  };
  if(length_ == 0) {
    // No body
    size=0;
    return false;
  };
  if(length_ > 0) {
    // Ordinary stream with known length
    int64_t bs = length_-stream_offset_;
    if(bs == 0) { size=0; return false; }; // End of content
    if(bs > size) bs=size;
    if(!read(buf,bs)) {
      valid_=false; // This is not expected, hence invalidate object
      size=bs; return false;
    };
    size=bs; stream_offset_+=bs;
    return true;
  };
  // Ordinary stream with no length known
  int64_t tsize = size;
  bool r = read(buf,tsize);
  if(r) stream_offset_+=tsize;
  size=tsize;
  // TODO: adjust logical parameters of buffers
  return r;
}

bool PayloadHTTP::Get(std::string& buf) {
  char tbuf[1024];
  int l = sizeof(tbuf);
  bool result = Get(tbuf,l);
  buf.assign(tbuf,l);
  return result;
}

std::string PayloadHTTP::Get(void) {
  std::string s;
  Get(s);
  return s;
}

// Stream interface is meant to be used only
// for reading HTTP body.
bool PayloadHTTP::Put(const char* /* buf */,PayloadStreamInterface::Size_t /* size */) {
  return false;
}

bool PayloadHTTP::Put(const std::string& /* buf */) {
  return false;
}

bool PayloadHTTP::Put(const char* /* buf */) {
  return false;
}

int PayloadHTTP::Timeout(void) const {
  if(!stream_) return 0;
  return stream_->Timeout();
}

void PayloadHTTP::Timeout(int to) {
  if(stream_) stream_->Timeout(to);
}

PayloadStreamInterface::Size_t PayloadHTTP::Pos(void) const {
  if(!stream_) return 0;
  return offset_+stream_offset_;
}

PayloadStreamInterface::Size_t PayloadHTTP::Limit(void) const {
  return Size();
}

} // namespace Arc

