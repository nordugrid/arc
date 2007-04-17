#include <stdio.h>

#include "PayloadHTTP.h"

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
  char buf[1024];
  method_.resize(0);
  code_=0;
  // Skip empty lines
  std::string line;
  for(;line.empty();) if(!readline(line)) return false;
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
  // Parse header lines
  for(;readline(line) && (!line.empty());) {
    std::string::size_type pos = line.find(':');
    if(pos == std::string::npos) continue;
    std::string name = line.substr(0,pos);
    for(std::string::size_type p=0;p<name.length();++p) name[p]=tolower(name[p]);
    for(++pos;pos<line.length();++pos) if(!isspace(line[pos])) break;
    if(pos<line.length()) {
      std::string value = line.substr(pos);
      for(std::string::size_type p=0;p<value.length();++p) value[p]=tolower(value[p]);
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
  it=attributes_.find("transfer-encoding");
  if(it != attributes_.end()) {
    if(strcasecmp(it->second.c_str(),"chunked") != 0) {
      // Non-implemented encoding
      return false;
    };
    chunked_=true;
  };
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
    for(;;) {
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
      char* new_result = (char*)realloc(result,result_size+chunk_size);
      if(new_result == NULL) { free(result); return false; };
      result=new_result;
      if(!read(result+result_size,chunk_size)) { free(result); return false; };
      result_size+=chunk_size;
      if(!readline(line)) return false;
      if(!line.empty()) return false;
    };
  } else if(length_ >= 0) {
    if(length_ > 0) {
      result=(char*)malloc(length_);
      if(!read(result,length_)) { free(result); return false; };
      result_size=length_;
    };
  } else {
    // Read till connection closed
    for(;;) {
      int chunk_size = 4096;
      char* new_result = (char*)realloc(result,result_size+chunk_size);
      if(new_result == NULL) { free(result); return false; };
      result=new_result;
      if(!read(result+result_size,chunk_size)) break;
      result_size+=chunk_size;
    };
  };
  // Attach result to buffer exposed to user
  Buf b;
  b.data=result; b.size=result_size; b.length=result_size; b.allocated=true;
  buf_.push_back(b);
  return true;
}

const std::string& PayloadHTTP::Attribute(const std::string& name) {
  std::map<std::string,std::string>::iterator it = attributes_.find(name);
  if(it == attributes_.end()) return empty_string;
  return it->second;
}

void PayloadHTTP::Attribute(const std::string& name,const std::string& value) {
  attributes_[name]=value;
}

PayloadHTTP::PayloadHTTP(PayloadStreamInterface& stream):stream_(stream),valid_(false) {
  tbuf_[0]=0; tbuflen_=0;
  if(!parse_header()) return;
  if(!get_body()) return;
  valid_=true;
}

PayloadHTTP::PayloadHTTP(const std::string& method,const std::string& url,PayloadStreamInterface& stream):method_(method),uri_(url),stream_(stream),valid_(true) {
  version_major_=1; version_minor_=1;
  // TODO: encode URI properly
}

PayloadHTTP::PayloadHTTP(int code,const std::string& reason,PayloadStreamInterface& stream):code_(code),reason_(reason),stream_(stream),valid_(true) {
  version_major_=1; version_minor_=1;
  if(reason_.empty()) reason_="OK";
}

PayloadHTTP::PayloadHTTP(const std::string& method,const std::string& url):method_(method),uri_(url),stream_(*((PayloadStreamInterface*)NULL)),valid_(true) {
  version_major_=1; version_minor_=1;
  // TODO: encode URI properly
}

PayloadHTTP::PayloadHTTP(int code,const std::string& reason):code_(code),reason_(reason),stream_(*((PayloadStreamInterface*)NULL)),valid_(true) {
  version_major_=1; version_minor_=1;
  if(reason_.empty()) reason_="OK";
}

PayloadHTTP::~PayloadHTTP(void) {
}

static std::string tostring(int i) {
  char tbuf[256]; tbuf[255]=0;
  snprintf(tbuf,255,"%i",i);
  return std::string(tbuf);
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
    header="HTTP/"+tostring(version_major_)+"."+tostring(version_minor_)+
           tostring(code_)+" "+reason_+"\r\n";
  } else {
    return false;
  };
  //if(!stream_.Put(line)) return false;
   std::string host;
  if((version_major_ == 1) && (version_minor_ == 1) && (!method_.empty())) {
    //line.resize(0);
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
    //if(!stream_.Put(line)) return false;
  };
  length_=Size();
  if((method_ != "GET") && (method_ != "HEAD")) {
    header+="Content-Length: "+tostring(length_)+"\r\n";
    //if(!stream_.Put(line)) return false;
  };
  bool keep_alive = false;
  if((version_major_ == 1) && (version_minor_ == 1)) keep_alive=true;
  //if(keep_alive) if(!stream_.Put("Connection: keep-alive\r\n")) return false;
  if(keep_alive) header+="Connection: keep-alive\r\n";
  //if(!stream_.Put("\r\n")) return false;
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
    //if(!keep_alive) stream.Close();
  } else {
    Insert(header.c_str(),0,header.length());
  };
  return true;
}

//  virtual char operator[](int pos) const;
//  virtual char* Content(int pos = -1);
//  virtual int Size(void) const;
//  virtual char* Insert(int pos = 0,int size = 0);
//  virtual char* Insert(const char* s,int pos = 0,int size = 0);
//  virtual char* Buffer(int num);
//  virtual int BufferSize(int num) const;

} // namespace Arc
