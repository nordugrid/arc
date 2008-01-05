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
    if(!stream_.Get(tbuf_,tbuflen_)) if(tbuflen_ == 0) break;
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
    for(std::string::size_type p=0;p<name.length();++p) name[p]=tolower(name[p]);
    for(++pos;pos<line.length();++pos) if(!isspace(line[pos])) break;
    if(pos<line.length()) {
      std::string value = line.substr(pos);
      // for(std::string::size_type p=0;p<value.length();++p) value[p]=tolower(value[p]);
      attributes_[name]=value;
    } else {
      attributes_[name]="";
    };
  };
  length_=-1; chunked_=false;
  std::map<std::string,std::string>::iterator it;
  it=attributes_.find("content-length");
  if(it != attributes_.end()) {
    length_=strtol(it->second.c_str(),NULL,10);
  };
  it=attributes_.find("content-range");
  if(it != attributes_.end()) {
    const char* token = it->second.c_str();
    const char* p = token; for(;*p;p++) if(isspace(*p)) break;
    int range_start,range_end,entity_size;
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
  if(keep_alive_ && (length_ == -1)) length_=0;
  if(size_ == 0) size_=offset_+length_;
  return true;
}

bool PayloadHTTP::read(char* buf,int& size) {
  if(tbuflen_ >= size) {
    memcpy(buf,tbuf_,size);
    memmove(tbuf_,tbuf_+size,tbuflen_-size+1);
    tbuflen_-=size;
  } else {
    memcpy(buf,tbuf_,tbuflen_);
    buf+=tbuflen_; 
    int l = size-tbuflen_;
    size=tbuflen_; tbuflen_=0; tbuf_[0]=0; 
    for(;l;) {
      int l_ = l;
      if(!stream_.Get(buf,l_)) return (size>0);
      size+=l_; buf+=l_; l-=l_;
    };
  };
  return true;
}

bool PayloadHTTP::get_body(void) {
  // TODO: Check for methods and responses which can't have body
  char* result = NULL;
  int result_size = 0;
  if(chunked_) {
    for(;;) {
      std::string line;
      if(!readline(line)) return false;
      char* e;
      int chunk_size = strtol(line.c_str(),&e,16);
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
    return true;
  } else if(length_ > 0) {
    result=(char*)malloc(length_+1);
    if(!read(result,length_)) { free(result); return false; };
    result_size=length_;
  } else {
    // Read till connection closed
    for(;;) {
      int chunk_size = 4096;
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
  return true;
}

const std::string& PayloadHTTP::Attribute(const std::string& name) {
  std::map<std::string,std::string>::iterator it = attributes_.find(name);
  if(it == attributes_.end()) return empty_string;
  return it->second;
}

const std::map<std::string,std::string>& PayloadHTTP::Attributes(void) {
  return attributes_;
}

void PayloadHTTP::Attribute(const std::string& name,const std::string& value) {
  attributes_[name]=value;
}

PayloadHTTP::PayloadHTTP(PayloadStreamInterface& stream):valid_(false),stream_(stream),body_(NULL),body_own_(false) {
  tbuf_[0]=0; tbuflen_=0;
  if(!parse_header()) return;
  if(!get_body()) return;
  valid_=true;
}

PayloadHTTP::PayloadHTTP(const std::string& method,const std::string& url,PayloadStreamInterface& stream):valid_(true),stream_(stream),body_(NULL),body_own_(false),uri_(url),method_(method) {
  version_major_=1; version_minor_=1;
  // TODO: encode URI properly
}

PayloadHTTP::PayloadHTTP(int code,const std::string& reason,PayloadStreamInterface& stream):valid_(true),stream_(stream),body_(NULL),body_own_(false),code_(code),reason_(reason) {
  version_major_=1; version_minor_=1;
  if(reason_.empty()) reason_="OK";
}

PayloadHTTP::PayloadHTTP(const std::string& method,const std::string& url):valid_(true),stream_(*((PayloadStreamInterface*)NULL)),body_(NULL),body_own_(false),uri_(url),method_(method) {
  version_major_=1; version_minor_=1;
  // TODO: encode URI properly
}

PayloadHTTP::PayloadHTTP(int code,const std::string& reason):valid_(true),stream_(*((PayloadStreamInterface*)NULL)),body_(NULL),body_own_(false),code_(code),reason_(reason) {
  version_major_=1; version_minor_=1;
  if(reason_.empty()) reason_="OK";
}

PayloadHTTP::~PayloadHTTP(void) {
  if(body_ && body_own_) delete body_;
}

bool PayloadHTTP::Flush(void) {
  std::string header;
  bool to_stream = ((&stream_) != NULL);
  if(!method_.empty()) {
    header=method_+" "+uri_+
           " HTTP/"+tostring(version_major_)+"."+tostring(version_minor_)+"\r\n";
  } else if(code_ != 0) {
    char tbuf[256]; tbuf[255]=0;
    snprintf(tbuf,255,"HTTP/%i.%i %i",version_major_,version_minor_,code_);
    header="HTTP/"+tostring(version_major_)+"."+tostring(version_minor_)+" "+
           tostring(code_)+" "+reason_+"\r\n";
  } else {
    return false;
  };
  std::string host;
  if((version_major_ == 1) && (version_minor_ == 1) && (!method_.empty())) {
    if(!uri_.empty()) {
      std::string::size_type p1 = uri_.find("://");
      if(p1 != std::string::npos) {
        std::string::size_type p2 = uri_.find('/',p1+3);
        if(p2 != std::string::npos) {
          host=uri_.substr(p1+3,p2-p1-3);
        };
      };
    };
    header+="Host: "+host+"\r\n";
  };
  // Computing length of Body part
  length_=0;
  if((method_ != "GET") && (method_ != "HEAD")) {
    for(int n=0;;++n) {
      if(Buffer(n) == NULL) break;
      length_+=BufferSize(n);
    };
    if(length_ != Size()) {
      // Add range definition if Body represents part of logical buffer size
      int start = BufferPos(0);
      int end = start+length_;
      if(end <= Size()) {
        header+="Content-Range: bytes "+tostring(start)+"-"+tostring(end-1)+"/"+tostring(Size())+"\r\n";
      } else {
        header+="Content-Range: bytes "+tostring(start)+"-"+tostring(end-1)+"/*\r\n";
      };
    };
    header+="Content-Length: "+tostring(length_)+"\r\n";
  };
  bool keep_alive = false;
  if((version_major_ == 1) && (version_minor_ == 1)) keep_alive=true;
  // TODO: use keep-alive from request in response as well
  if(keep_alive) header+="Connection: keep-alive\r\n";
  for(std::map<std::string,std::string>::iterator a = attributes_.begin();a!=attributes_.end();++a) {
    header+=(a->first)+": "+(a->second)+"\r\n";
  };
  header+="\r\n";
  if(to_stream) {
    if(!stream_.Put(header)) return false;
    if(length_ > 0) {
      for(int n=0;;++n) {
        char* tbuf = Buffer(n);
        if(tbuf == NULL) break;
        int lbuf = BufferSize(n);
        if(lbuf > 0) if(!stream_.Put(tbuf,lbuf)) return false;
      };
    };
    //if(!keep_alive) stream_.Close();
  } else {
    Insert(header.c_str(),0,header.length());
  };
  return true;
}

void PayloadHTTP::Body(PayloadRawInterface& body,bool ownership) {
  if(body_ && body_own_) delete body_;
  body_=&body; body_own_=ownership;
}

char PayloadHTTP::operator[](int pos) const {
  if(pos < PayloadRaw::Size()) {
    return PayloadRaw::operator[](pos);
  };
  if(body_) {
    return body_->operator[](pos-Size());
  };
  return 0;
}

char* PayloadHTTP::Content(int pos) {
  if(pos < PayloadRaw::Size()) {
    return PayloadRaw::Content(pos);
  };
  if(body_) {
    return body_->Content(pos-Size());
  };
  return NULL;
}

int PayloadHTTP::Size(void) const {
  if(body_) {
    return PayloadRaw::Size() + (body_->Size());
  };
  return PayloadRaw::Size();
}

char* PayloadHTTP::Insert(int pos,int size) {
  return PayloadRaw::Insert(pos,size);
}

char* PayloadHTTP::Insert(const char* s,int pos,int size) {
  return PayloadRaw::Insert(s,pos,size);
}

char* PayloadHTTP::Buffer(unsigned int num) {
  if(num < buf_.size()) {
    return PayloadRaw::Buffer(num);
  };
  if(body_) {
    return body_->Buffer(num-buf_.size());
  };
  return NULL;
}

int PayloadHTTP::BufferSize(unsigned int num) const {
  if(num < buf_.size()) {
    return PayloadRaw::BufferSize(num);
  };
  if(body_) {
    return body_->BufferSize(num-buf_.size());
  };
  return 0;
}

int PayloadHTTP::BufferPos(unsigned int num) const {
  if(num < buf_.size()) {
    return PayloadRaw::BufferPos(num);
  };
  if(body_) {
    return body_->BufferPos(num-buf_.size())+PayloadRaw::BufferPos(num);
  };
  return PayloadRaw::BufferPos(num);
}

bool PayloadHTTP::Truncate(unsigned int size) {
  if(size < PayloadRaw::Size()) {
    body_=NULL;
    return PayloadRaw::Truncate(size);
  };
  if(body_) {
    return body_->Truncate(size-Size());
  };
  return false;
}

} // namespace Arc

