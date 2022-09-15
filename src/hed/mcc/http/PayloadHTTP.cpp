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

namespace ArcMCCHTTP {

using namespace Arc;

Arc::Logger PayloadHTTP::logger(Arc::Logger::getRootLogger(), "MCC.HTTP");

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

// -------------------- PayloadHTTP -----------------------------

const std::string& PayloadHTTP::Attribute(const std::string& name) const {
  std::multimap<std::string,std::string>::const_iterator it = attributes_.find(name);
  if(it == attributes_.end()) return empty_string;
  return it->second;
}

const std::list<std::string> PayloadHTTP::Attributes(const std::string& name) const {
  std::list<std::string> attrs;
  for(std::multimap<std::string,std::string>::const_iterator attr = attributes_.begin();
                          attr != attributes_.end(); ++attr) {
    if(attr->first == name) attrs.push_back(attr->second);
  };
  return attrs;
}

const std::multimap<std::string,std::string>& PayloadHTTP::Attributes(void) const {
  return attributes_;
}

bool PayloadHTTP::AttributeMatch(const std::string& name, const std::string& value) const {
  std::multimap<std::string,std::string>::const_iterator attr = attributes_.begin();
  for(;attr != attributes_.end();++attr) {
    if(attr->first == name) {
      std::string sattr = Arc::lower(Arc::trim(attr->second," \r\n"));
      if(sattr == value) return true;
    };
  };
  return false;
}

PayloadHTTP::PayloadHTTP(void):
    valid_(false),version_major_(1),version_minor_(1),
    code_(0),length_(0),offset_(0),size_(0),end_(0),keep_alive_(true) {
}

PayloadHTTP::PayloadHTTP(const std::string& method,const std::string& url):
    valid_(false),
    uri_(url),version_major_(1),version_minor_(1),method_(method),
    code_(0),length_(0),offset_(0),size_(0),end_(0),keep_alive_(true) {
  // TODO: encode URI properly
}

PayloadHTTP::PayloadHTTP(int code,const std::string& reason):
    valid_(false),
    version_major_(1),version_minor_(1),
    code_(code),reason_(reason),
    length_(0),offset_(0),size_(0),end_(0),keep_alive_(true) {
  if(reason_.empty()) reason_="OK";
}

PayloadHTTP::~PayloadHTTP(void) {
}

// ------------------- PayloadHTTPIn ----------------------------

bool PayloadHTTPIn::readtbuf(void) {
  int l = (sizeof(tbuf_)-1) - tbuflen_;
  if(l > 0) {
    if(stream_->Get(tbuf_+tbuflen_,l)) {
      tbuflen_ += l;
      tbuf_[tbuflen_]=0;
    }
  }
  return (tbuflen_>0);
}

bool PayloadHTTPIn::readline(std::string& line) {
  line.resize(0);
  for(;line.length()<4096;) { // keeping line size sane
    char* p = (char*)memchr(tbuf_,'\n',tbuflen_);
    if(p) {
      *p=0;
      line.append(tbuf_,p-tbuf_);
      tbuflen_-=(p-tbuf_)+1;
      memmove(tbuf_,p+1,tbuflen_+1); 
      if((!line.empty()) && (line[line.length()-1] == '\r')) line.resize(line.length()-1);
      return true;
    };
    line.append(tbuf_,tbuflen_);
    tbuflen_=0;
    if(!readtbuf()) break;
  }; 
  tbuf_[tbuflen_]=0;
  return false;
}

bool PayloadHTTPIn::read(char* buf,int64_t& size) {
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

bool PayloadHTTPIn::readline_chunked(std::string& line) {
  if(!chunked_) return readline(line);
  line.resize(0);
  for(;line.length()<4096;) { // keeping line size sane
    if(tbuflen_ <= 0) {
      if(!readtbuf()) break;
    };
    char c;
    int64_t l = 1;
    if(!read_chunked(&c,l)) break;
    if(c == '\n') {
      if((!line.empty()) && (line[line.length()-1] == '\r')) line.resize(line.length()-1);
      return true;
    };
    line.append(&c,1); // suboptimal
  };
  return false;
}

bool PayloadHTTPIn::read_chunked(char* buf,int64_t& size) {
  if(!chunked_) return read(buf,size);
  int64_t bufsize = size;
  size = 0;
  if(chunked_ == CHUNKED_ERROR) return false;
  for(;bufsize>0;) {
    if(chunked_ == CHUNKED_EOF) break;
    if(chunked_ == CHUNKED_START) { // reading chunk size
      std::string line;
      chunked_ = CHUNKED_ERROR;
      if(!readline(line)) break;
      char* e;
      chunk_size_ = strtoll(line.c_str(),&e,16);
      if((*e != ';') && (*e != 0)) break;
      if(e == line.c_str()) break;
      if(chunk_size_ == 0) {
        chunked_ = CHUNKED_EOF;
      } else {
        chunked_ = CHUNKED_CHUNK;
      };
    };
    if(chunked_ == CHUNKED_CHUNK) { // reading chunk data
      int64_t l = bufsize;
      if(chunk_size_ < l) l = chunk_size_;
      chunked_ = CHUNKED_ERROR;
      if(!read(buf,l)) break;
      chunk_size_ -= l;
      size += l;
      bufsize -= l;
      buf += l;
      if(chunk_size_ <= 0) {
        chunked_ = CHUNKED_END;
      } else {
        chunked_ = CHUNKED_CHUNK;
      };
    };
    if(chunked_ == CHUNKED_END) { // reading CRLF at end of chunk
      std::string line;
      chunked_ = CHUNKED_ERROR;
      if(!readline(line)) break;
      if(!line.empty()) break;
      chunked_ = CHUNKED_START;
    };
  };
  return (size>0);
}

bool PayloadHTTPIn::flush_chunked(void) {
  if(!chunked_) return true;
  if(chunked_ == CHUNKED_EOF) return true;
  if(chunked_ == CHUNKED_ERROR) return false;
  const int bufsize = 1024;
  char* buf = new char[bufsize];
  for(;;) {
    if(chunked_ == CHUNKED_EOF) break;
    if(chunked_ == CHUNKED_ERROR) break;
    int64_t l = bufsize;
    if(!read_chunked(buf,l)) break;
  };
  delete[] buf;
  return (chunked_ == CHUNKED_EOF);
}

char* PayloadHTTPIn::find_multipart(char* buf,int64_t size) {
  char* p = buf;
  for(;;++p) {
    p = (char*)memchr(p,'\r',size-(p-buf));
    if(!p) break; // no tag found
    int64_t l = (multipart_tag_.length()+2) - (size-(p-buf));
    if(l > 0) {
      // filling buffer with necessary amount of information
      if(l > multipart_buf_.length()) {
        int64_t ll = multipart_buf_.length();
        multipart_buf_.resize(l);
        l = l-ll;
        if(!read_chunked((char*)(multipart_buf_.c_str()+ll),l)) {
          p = NULL;
          break;
        };
        multipart_buf_.resize(ll+l);
      }
    }
    int64_t pos = p-buf;
    ++pos;
    char c = '\0';
    if(pos < size) {
      c = buf[pos];
    } else if((pos-size) < multipart_buf_.length()) {
      c = multipart_buf_[pos-size];
    };
    if(c != '\n') continue;
    int tpos = 0;
    for(;tpos<multipart_tag_.length();++tpos) {
      ++pos;
      c = '\0';
      if(pos < size) {
        c = buf[pos];
      } else if((pos-size) < multipart_buf_.length()) {
        c = multipart_buf_[pos-size];
      };
      if(c != multipart_tag_[tpos]) break;
    };
    if(tpos >= multipart_tag_.length()) break; // tag found
  }
  return p;
}

bool PayloadHTTPIn::read_multipart(char* buf,int64_t& size) {
  if(!multipart_) return read_chunked(buf,size);
  if(multipart_ == MULTIPART_END) return false;
  if(multipart_ == MULTIPART_EOF) return false;
  int64_t bufsize = size;
  size = 0;
  if(!multipart_buf_.empty()) {
    // pick up previously loaded data
    if(bufsize >= multipart_buf_.length()) {
      memcpy(buf,multipart_buf_.c_str(),multipart_buf_.length());
      size = multipart_buf_.length();
      multipart_buf_.resize(0);
    } else {
      memcpy(buf,multipart_buf_.c_str(),bufsize);
      size = bufsize;
      multipart_buf_.erase(0,bufsize);
    };
  }
  // read more data if needed
  if(size < bufsize) {
    int64_t l = bufsize - size;
    if(!read_chunked(buf+size,l)) return false;
    size += l;
  }
  // looking for tag
  const char* p = find_multipart(buf,size);
  if(p) {
    // tag found
    // hope nothing sends GBs in multipart
    multipart_buf_.insert(0,p,size-(p-buf));
    size = (p-buf);
    // TODO: check if it is last tag
    multipart_ = MULTIPART_END;
  };
  logger.msg(Arc::DEBUG,"<< %s",std::string(buf,size));
  return true;
}

bool PayloadHTTPIn::flush_multipart(void) {
  // TODO: protect against insame length of body
  if(!multipart_) return true;
  if(multipart_ == MULTIPART_ERROR) return false;
  std::string::size_type pos = 0;
  for(;multipart_ != MULTIPART_EOF;) {
    pos = multipart_buf_.find('\r',pos);
    if(pos == std::string::npos) {
      pos = 0;
      // read just enough
      int64_t l = multipart_tag_.length()+4;
      multipart_buf_.resize(l);
      if(!read_chunked((char*)(multipart_buf_.c_str()),l)) return false;
      multipart_buf_.resize(l);
      continue;
    }
    multipart_buf_.erase(0,pos); // suboptimal
    pos = 0;
    int64_t l = multipart_tag_.length()+4;
    if(l > multipart_buf_.length()) {
      int64_t ll = multipart_buf_.length();
      multipart_buf_.resize(l);
      l = l - ll;
      if(!read_chunked((char*)(multipart_buf_.c_str()+ll),l)) return false;
      ll += l;
      if(ll < multipart_buf_.length()) return false; // can't read enough data
    };
    ++pos;
    if(multipart_buf_[pos] != '\n') continue;
    ++pos;
    if(strncmp(multipart_buf_.c_str()+pos,multipart_tag_.c_str(),multipart_tag_.length())
                 != 0) continue;
    pos+=multipart_tag_.length();
    if(multipart_buf_[pos] != '-') continue;
    ++pos;
    if(multipart_buf_[pos] != '-') continue;
    // end tag found
    multipart_ = MULTIPART_EOF;
  };
  return true;
}

bool PayloadHTTPIn::read_header(void) {
  std::string line;
  for(;readline_chunked(line) && (!line.empty());) {
    logger.msg(Arc::DEBUG,"< %s",line);
    std::string::size_type pos = line.find(':');
    if(pos == std::string::npos) continue;
    std::string name = line.substr(0,pos);
    for(++pos;pos<line.length();++pos) if(!isspace(line[pos])) break;
    if(pos<line.length()) {
      attributes_.insert(std::pair<std::string,std::string>(lower(name),line.substr(pos)));
    } else {
      attributes_.insert(std::pair<std::string,std::string>(lower(name),""));
    };
  };
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
            end_=range_end+1;
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
    chunked_= CHUNKED_START;
  };
  it=attributes_.find("connection");
  if(it != attributes_.end()) {
    if(strcasecmp(it->second.c_str(),"keep-alive") == 0) {
      keep_alive_=true;
    } else {
      keep_alive_=false;
    };
  };
  it=attributes_.find("content-type");
  if(it != attributes_.end()) {
    if(strncasecmp(it->second.c_str(),"multipart/",10) == 0) {
      // TODO: more bulletproof approach is needed
      std::string lline = lower(it->second);
      const char* boundary = strstr(lline.c_str()+10,"boundary=");
      if(!boundary) return false;
      boundary = it->second.c_str()+(boundary-lline.c_str());
      //const char* boundary = strcasestr(it->second.c_str()+10,"boundary=");
      const char* tag_start = strchr(boundary,'"');
      const char* tag_end = NULL;
      if(!tag_start) {
        tag_start = boundary + 9;
        tag_end = strchr(tag_start,' ');
        if(!tag_end) tag_end = tag_start + strlen(tag_start);
      } else {
        ++tag_start;
        tag_end = strchr(tag_start,'"');
      }
      if(!tag_end) return false;
      multipart_tag_ = std::string(tag_start,tag_end-tag_start);
      if(multipart_tag_.empty()) return false;
      multipart_ = MULTIPART_START;
      multipart_tag_.insert(0,"--");
      multipart_buf_.resize(0);
    }
  }
  return true;
}

bool PayloadHTTPIn::parse_header(void) {
  method_.resize(0);
  code_=0;
  keep_alive_=false;
  multipart_ = MULTIPART_NONE;
  multipart_tag_ = "";
  chunked_=CHUNKED_NONE;
  // Skip empty lines
  std::string line;
  for(;line.empty();) if(!readline(line)) {
    method_="END";  // Special name to represent closed connection
    length_=0;
    return true;
  };
  logger.msg(Arc::DEBUG,"< %s",line);
  // Parse request/response line
  std::string::size_type pos2 = line.find(' ');
  if(pos2 == std::string::npos)  return false;
  std::string word1 = line.substr(0,pos2);
  // Identify request/response
  if(ParseHTTPVersion(line.substr(0,pos2),version_major_,version_minor_)) {
    // Response
    std::string::size_type pos3 = line.find(' ',pos2+1);
    if(pos3 == std::string::npos) return false;
    code_=strtol(line.c_str()+pos2+1,NULL,10);
    reason_=line.substr(pos3+1);
    if(code_ == 100) {
      // TODO: skip 100 response
    }
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
  length_=-1; chunked_=CHUNKED_NONE;
  if(!read_header()) return false;
  if(multipart_ == MULTIPART_START) {
    attributes_.erase("content-type");
    // looking for multipart
    std::string line;
    for(;;) {
      if(!readline_chunked(line)) return false;
      if(line.length() == multipart_tag_.length()) {
        if(strncmp(line.c_str(),multipart_tag_.c_str(),multipart_tag_.length()) == 0) {
          multipart_ = MULTIPART_BODY;
          break;
        };
      };
    };
    // reading additional header lines
    chunked_t chunked = chunked_;
    if(!read_header()) return false;
    if(multipart_ !=  MULTIPART_BODY) return false; // nested multipart
    if(chunked_ != chunked) return false; // can't change transfer encoding per part
    // TODO:  check if content-length can be defined here
    // now real body follows
  }
  // In case of keep_alive (HTTP1.1) there must be length specified
  if(keep_alive_ && (!chunked_) && (length_ == -1)) length_=0;
  // If size of object was not reported then try to deduce it.
  if((size_ == 0) && (length_ != -1)) size_=offset_+length_;
  return true;
}

bool PayloadHTTPIn::get_body(void) {
  if(fetched_) return true; // Already fetched body
  fetched_=true; // Even attempt counts
  valid_=false; // But object is invalid till whole body is available
  if(body_) free(body_);
  body_ = NULL; body_size_ = 0;
  if(head_response_ && (code_ == 200)) {
    // Successful response to HEAD contains no body
    valid_=true;
    flush_multipart();
    flush_chunked();
    body_read_=true;
    return true;
  };
  char* result = NULL;
  int64_t result_size = 0;
  if(length_ == 0) {
    valid_=true;
    body_read_=true;
    return true;
  } else if(length_ > 0) {
    // TODO: combination of chunked and defined length is probably impossible
    // TODO: protect against insane length_
    result=(char*)malloc(length_+1);
    if(!read_multipart(result,length_)) { free(result); return false; };
    result_size=length_;
  } else { // length undefined
    // Read till connection closed or some logic reports eof
    for(;;) {
      int64_t chunk_size = 4096;
      char* new_result = (char*)realloc(result,result_size+chunk_size+1);
      if(new_result == NULL) { free(result); return false; };
      result=new_result;
      if(!read_multipart(result+result_size,chunk_size)) break;
      // TODO: logical size is not always same as end of body
      // TODO: protect against insane length of body
      result_size+=chunk_size;
    };
  };
  if (result == NULL) {
    return false;
  }
  result[result_size]=0;
  // Attach result to buffer exposed to user
  body_ = result; body_size_ = result_size;
  // If size of object was not reported then try to deduce it.
  if(size_ == 0) size_=offset_+result_size;
  valid_=true;
  // allign to end of message
  flush_multipart();
  flush_chunked();
  body_read_=true;
  return true;
}

PayloadHTTPIn::PayloadHTTPIn(PayloadStreamInterface& stream,bool own,bool head_response):
    head_response_(head_response),chunked_(CHUNKED_NONE),chunk_size_(0),
    multipart_(MULTIPART_NONE),stream_(&stream),stream_offset_(0),
    stream_own_(own),fetched_(false),header_read_(false),body_read_(false),
    body_(NULL),body_size_(0) {
  tbuf_[0]=0; tbuflen_=0;
  if(!parse_header()) {
    error_ = IString("Failed to parse HTTP header").str();
    return;
  }
  header_read_=true;
  valid_=true;
}

PayloadHTTPIn::~PayloadHTTPIn(void) {
  // allign to end of message (maybe not needed with Sync() exposed)
  flush_multipart();
  flush_chunked();
  if(stream_ && stream_own_) delete stream_;
  if(body_) ::free(body_);
}

char PayloadHTTPIn::operator[](PayloadRawInterface::Size_t pos) const {
  if(!((PayloadHTTPIn*)this)->get_body()) return 0;
  if(!body_) return 0;
  if(pos == -1) pos = offset_;
  if(pos < offset_) return 0;
  pos -= offset_;
  if(pos >= body_size_) return 0;
  return body_[pos];
}

char* PayloadHTTPIn::Content(PayloadRawInterface::Size_t pos) {
  if(!get_body()) return NULL;
  if(!body_) return 0;
  if(pos == -1) pos = offset_;
  if(pos < offset_) return NULL;
  pos -= offset_;
  if(pos >= body_size_) return NULL;
  return body_+pos;
}

PayloadRawInterface::Size_t PayloadHTTPIn::Size(void) const {
  if(!valid_) return 0;
  PayloadRawInterface::Size_t size = 0;
  if(size_ > 0) {
    size = size_;
  } else if(end_ > 0) {
    size = end_;
  } else if(length_ >= 0) {
    size = offset_ + length_;
  } else {
    // Only do it if no other way of determining size worked.
    if(((PayloadHTTPIn*)this)->get_body()) size = body_size_;
  }
  return size;
}

char* PayloadHTTPIn::Insert(PayloadRawInterface::Size_t pos,PayloadRawInterface::Size_t size) {
  return NULL;
}

char* PayloadHTTPIn::Insert(const char* s,PayloadRawInterface::Size_t pos,PayloadRawInterface::Size_t size) {
  return NULL;
}

char* PayloadHTTPIn::Buffer(unsigned int num) {
  if(num != 0) return NULL;
  if(!get_body()) return NULL;
  return body_;
}

PayloadRawInterface::Size_t PayloadHTTPIn::BufferSize(unsigned int num) const {
  if(num != 0) return 0;
  if(!((PayloadHTTPIn*)this)->get_body()) return 0;
  return body_size_;
}

PayloadRawInterface::Size_t PayloadHTTPIn::BufferPos(unsigned int num) const {
  if(num != 0) return 0;
  return offset_;
}

bool PayloadHTTPIn::Truncate(PayloadRawInterface::Size_t size) {
  if(!get_body()) return false;
  if(size <= offset_) {
    if(body_) free(body_);
    body_ = NULL; body_size_ = 0;
  };
  if((size-offset_) <= body_size_) {
    body_size_ = (size-offset_);
    return true;
  };
  return false;
}

bool PayloadHTTPIn::Get(char* buf,int& size) {
  if(!valid_) return false;
  if(fetched_) {
    // Read from buffer
    if(stream_offset_ < body_size_) {
      uint64_t l = body_size_ - stream_offset_;
      if(l>size) l=size;
      ::memcpy(buf,body_+stream_offset_,l);
      size=l; stream_offset_+=l;
      return true;
    };
    return false;
  };
  // Read directly from stream
  // TODO: Check for methods and responses which can't have body
  if(length_ == 0) {
    // No body
    size=0;
    body_read_=true;
    return false;
  };
  if(length_ > 0) {
    // Ordinary stream with known length
    int64_t bs = length_-stream_offset_;
    if(bs == 0) { size=0; return false; }; // End of content
    if(bs > size) bs=size;
    if(!read_multipart(buf,bs)) {
      valid_=false; // This is not expected, hence invalidate object
      size=bs; return false;
    };
    size=bs; stream_offset_+=bs;
    if(stream_offset_ >= length_) body_read_=true;
    return true;
  };
  // Ordinary stream with no length known
  int64_t tsize = size;
  bool r = read_multipart(buf,tsize);
  if(r) stream_offset_+=tsize;
  if(!r) body_read_=true;
  size=tsize;
  // TODO: adjust logical parameters of buffers
  return r;
}

// Stream interface is meant to be used only
// for reading HTTP body.
bool PayloadHTTPIn::Put(const char* /* buf */,PayloadStreamInterface::Size_t /* size */) {
  return false;
}

int PayloadHTTPIn::Timeout(void) const {
  if(!stream_) return 0;
  return stream_->Timeout();
}

void PayloadHTTPIn::Timeout(int to) {
  if(stream_) stream_->Timeout(to);
}

PayloadStreamInterface::Size_t PayloadHTTPIn::Pos(void) const {
  if(!stream_) return 0;
  return offset_+stream_offset_;
}

PayloadStreamInterface::Size_t PayloadHTTPIn::Limit(void) const {
  if(length_ >= 0) return (offset_ + length_);
  return (offset_ + body_size_);
}

bool PayloadHTTPIn::Sync(void) {
  if(!valid_) return false;
  if(!header_read_) return false;
  if(fetched_) return true;
  // For multipart data - read till end tag
  // If data is chunked then it is enough to just read till 
  // chunks are over.
  if(multipart_ || chunked_) {
    bool r = true;
    if(!flush_multipart()) r=false; // not really neaded but keeps variables ok
    if(!flush_chunked()) r=false;
    if(r) body_read_ = true;
    return r;
  };
  // For data without any tags just read till end reached
  for(;!body_read_;) {
    char buf[1024];
    int size = sizeof(buf);
    bool r = Get(buf,size);
    if(!r) break;
  };
  if(body_read_) return true;
  return false;
}

// ------------------- PayloadHTTPOut ---------------------------

void PayloadHTTPOut::Attribute(const std::string& name,const std::string& value) {
  attributes_.insert(std::pair<std::string,std::string>(lower(name),value));
}

PayloadHTTPOut::PayloadHTTPOut(const std::string& method,const std::string& url):
    PayloadHTTP(method,url),
    head_response_(false),rbody_(NULL),sbody_(NULL),sbody_size_(0),
    body_own_(false),to_stream_(false),use_chunked_transfer_(false),
    stream_offset_(0),stream_finished_(false),
    enable_header_out_(true), enable_body_out_(true) {
  valid_ = true;
}

PayloadHTTPOut::PayloadHTTPOut(int code,const std::string& reason,bool head_response):
    PayloadHTTP(code,reason),
    head_response_(head_response),rbody_(NULL),sbody_(NULL),sbody_size_(0),
    body_own_(false),to_stream_(false),use_chunked_transfer_(false),
    stream_offset_(0), stream_finished_(false),
    enable_header_out_(true), enable_body_out_(true) {
  valid_ = true;
}

PayloadHTTPOut::~PayloadHTTPOut(void) {
  if(rbody_ && body_own_) delete rbody_;
  if(sbody_ && body_own_) delete sbody_;
}

PayloadHTTPOutStream::PayloadHTTPOutStream(const std::string& method,const std::string& url):
  PayloadHTTPOut(method,url) /*,chunk_size_offset_(0)*/ {
}

PayloadHTTPOutStream::PayloadHTTPOutStream(int code,const std::string& reason,bool head_response):
  PayloadHTTPOut(code,reason,head_response) /*,chunk_size_offset_(0)*/ {
}

PayloadHTTPOutStream::~PayloadHTTPOutStream(void) {
}

PayloadHTTPOutRaw::PayloadHTTPOutRaw(const std::string& method,const std::string& url):
  PayloadHTTPOut(method,url) {
}

PayloadHTTPOutRaw::PayloadHTTPOutRaw(int code,const std::string& reason,bool head_response):
  PayloadHTTPOut(code,reason,head_response) {
}

PayloadHTTPOutRaw::~PayloadHTTPOutRaw(void) {
}

PayloadRawInterface::Size_t PayloadHTTPOut::body_size(void) const {
  if(rbody_) {
    PayloadRawInterface::Size_t size = 0;
    for(int n = 0;rbody_->Buffer(n);++n) {
      size += rbody_->BufferSize(n);
    };
    return size;
  } else if(sbody_) {
    return sbody_size_;
  };
  return 0;
}

PayloadRawInterface::Size_t PayloadHTTPOut::data_size(void) const {
  if(rbody_) {
    return (rbody_->Size());
  };
  if(sbody_) {
    return (sbody_->Size());
  };
  return 0;
}

bool PayloadHTTPOut::make_header(bool to_stream) {
  header_.resize(0);
  std::string header;
  if(method_.empty() && (code_ == 0)) {
    error_ = IString("Invalid HTTP object can't produce result").str();
    return false;
  };
  // Computing length of Body part
  int64_t length = 0;
  std::string range_header;
  use_chunked_transfer_ = false;
  if((method_ != "GET") && (method_ != "HEAD")) {
    int64_t start = 0;
    if(head_response_ && (code_==HTTP_OK)) {
      length = data_size();
    } else {
      if(sbody_) {
        if(sbody_->Limit() > sbody_->Pos()) {
          length = sbody_->Limit() - sbody_->Pos();
        };
        start = sbody_->Pos();
      } else if(rbody_) {
        for(int n=0;;++n) {
          if(rbody_->Buffer(n) == NULL) break;
          length+=rbody_->BufferSize(n);
        };
        start = rbody_->BufferPos(0);
      } else {
        length = 0;
        start = 0;
      };
    };
    if(length != data_size()) {
      // Add range definition if Body represents part of logical buffer size
      // and adjust HTTP code accordingly
      int64_t end = start+length;
      std::string length_str;
      std::string range_str;
      if(end <= data_size()) {
        length_str=tostring(data_size());
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
    if(length > 0 || !sbody_) {
      range_header+="Content-Length: "+tostring(length)+"\r\n";
    } else {
      // If computed length is 0 for stream source that may also mean it is
      // not known in advance. In this case either connection closing or 
      // chunked encoding may be used. Chunked is better because it 
      // allows to avoid reconnection.
      // But chunked can only be used if writing to stream right now.
      // TODO: extend it to support chunked in Raw buffer mode. It
      // is possible by inserting small buffers with lengths.
      if(to_stream) {
        range_header+="Transfer-Encoding: chunked\r\n";
        use_chunked_transfer_ = true;
      } else {
        // As last resort try to force connection close.
        // Although I have no idea how it shol dwork for PUT
        keep_alive_ = false;
      };
    };
  };
  // Starting header
  if(!method_.empty()) {
    header=method_+" "+uri_+
           " HTTP/"+tostring(version_major_)+"."+tostring(version_minor_)+"\r\n";
  } else if(code_ != 0) {
    header="HTTP/"+tostring(version_major_)+"."+tostring(version_minor_)+" "+
           tostring(code_)+" "+reason_+"\r\n";
  } else {
    return false;
  };
  if((version_major_ == 1) && (version_minor_ == 1) && (!method_.empty())) {
    // Adding Host attribute to request if not present.
    std::map<std::string,std::string>::iterator it = attributes_.find("host");
    if(it == attributes_.end()) {
      std::string host;
      if(!uri_.empty()) {
        std::string::size_type p1 = uri_.find("://");
        if(p1 != std::string::npos) {
          std::string::size_type p2 = uri_.find('/',p1+3);
          if(p2 == std::string::npos) p2 = uri_.length();
          host=uri_.substr(p1+3,p2-p1-3);
          p2 = host.rfind(':');
          if(p2 != std::string::npos) p2 = host.resize(p2);
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
  logger.msg(Arc::DEBUG,"> %s",header);
  header_ = header;
  to_stream_ = to_stream;
  length_ = length;
  return true;
}

bool PayloadHTTPOut::remake_header(bool to_stream) {
  if(header_.empty() || (to_stream_ != to_stream)) return make_header(to_stream);
  return true;
}

bool PayloadHTTPOut::Flush(PayloadStreamInterface& stream) {
  if(enable_header_out_) {
    if(!FlushHeader(stream)) return false;
  }
  if(enable_body_out_) {
    if(!FlushBody(stream)) return false;
  }
  return true;
}

bool PayloadHTTPOut::FlushHeader(PayloadStreamInterface& stream) {
  if(!make_header(true)) return false;
  if(!stream.Put(header_)) {
    error_ = IString("Failed to write header to output stream").str();
    return false;
  };
  return true;
}

bool PayloadHTTPOut::FlushBody(PayloadStreamInterface& stream) {
    // TODO: process 100 request/response
    if((length_ > 0) || (use_chunked_transfer_)) {
      if(sbody_) {
        // stream to stream transfer
        // TODO: choose optimal buffer size
        // TODO: parallel read and write for better performance
        int tbufsize = ((length_ <= 0) || (length_>1024*1024))?(1024*1024):length_;
        char* tbuf = new char[tbufsize];
        if(!tbuf) {
          error_ = IString("Memory allocation error").str();
          return false;
        };
        for(;;) {
          int lbuf = tbufsize;
          if(!sbody_->Get(tbuf,lbuf)) break;
          if(lbuf == 0) continue;
          if(use_chunked_transfer_) {
            if(!stream.Put(inttostr(lbuf,16)+"\r\n")) {
              error_ = IString("Failed to write body to output stream").str();
              delete[] tbuf;
              return false;
            };
          };
          if(!stream.Put(tbuf,lbuf)) {
            error_ = IString("Failed to write body to output stream").str();
            delete[] tbuf;
            return false;
          };
          if(use_chunked_transfer_) {
            if(!stream.Put("\r\n")) {
              error_ = IString("Failed to write body to output stream").str();
              delete[] tbuf;
              return false;
            };
          };
        };
        delete[] tbuf;
        tbuf = NULL;
        if(use_chunked_transfer_) {
          if(!stream.Put("0\r\n\r\n")) {
            error_ = IString("Failed to write body to output stream").str();
            return false;
          };
        };
      } else if(rbody_) {
        for(int n=0;;++n) {
          char* tbuf = rbody_->Buffer(n);
          if(tbuf == NULL) break;
          int64_t lbuf = rbody_->BufferSize(n);
          if(lbuf > 0) {
            if(use_chunked_transfer_) {
              if(!stream.Put(inttostr(lbuf,16)+"\r\n")) {
                error_ = IString("Failed to write body to output stream").str();
                return false;
              };
            };
            if(!stream.Put(tbuf,lbuf)) {
              error_ = IString("Failed to write body to output stream").str();
              return false;
            };
            if(use_chunked_transfer_) {
              if(!stream.Put("\r\n")) {
                error_ = IString("Failed to write body to output stream").str();
                return false;
              };
            };
          };
        };
        if(use_chunked_transfer_) {
          if(!stream.Put("0\r\n\r\n")) {
            error_ = IString("Failed to write body to output stream").str();
            return false;
          };
        };
      } else {
        if(use_chunked_transfer_) {
          if(!stream.Put("0\r\n\r\n")) {
            error_ = IString("Failed to write body to output stream").str();
            return false;
          };
        };
      };
    };
    return true;
}

void PayloadHTTPOutRaw::Body(PayloadRawInterface& body,bool ownership) {
  if(rbody_ && body_own_) delete rbody_;
  if(sbody_ && body_own_) delete sbody_;
  sbody_ = NULL;
  rbody_=&body; body_own_=ownership;
}

void PayloadHTTPOutStream::Body(PayloadStreamInterface& body,bool ownership) {
  if(rbody_ && body_own_) delete rbody_;
  if(sbody_ && body_own_) delete sbody_;
  rbody_ = NULL;
  sbody_=&body; body_own_=ownership; sbody_size_ = 0;
  PayloadStreamInterface::Size_t pos = sbody_->Pos();
  PayloadStreamInterface::Size_t size = sbody_->Size();
  PayloadStreamInterface::Size_t limit = sbody_->Limit();
  if((size == 0) || (size > limit)) size = limit;
  if(pos < size) sbody_size_ = (size-pos);
}

void PayloadHTTPOut::ResetOutput(bool enable_header, bool enable_body) {
  stream_offset_ = 0;
  stream_finished_ = false;
  // Because we can't/do not want to reset state
  // body stream then actual body size need to be
  // recomputed.
  sbody_size_ = 0;
  if(sbody_) {
    PayloadStreamInterface::Size_t pos = sbody_->Pos();
    PayloadStreamInterface::Size_t size = sbody_->Size();
    PayloadStreamInterface::Size_t limit = sbody_->Limit();
    if((size == 0) || (size > limit)) size = limit;
    if(pos < size) sbody_size_ = (size-pos);
  }
  enable_header_out_ = enable_header;
  enable_body_out_ = enable_body;
}


char PayloadHTTPOutRaw::operator[](PayloadRawInterface::Size_t pos) const {
  if(!((PayloadHTTPOutRaw&)(*this)).remake_header(false)) return 0;
  if(pos == -1) pos = 0;
  if(pos < 0) return 0;
  if(pos < header_.length()) {
    return header_[pos];
  };
  pos -= header_.length();
  if(rbody_) {
    return rbody_->operator[](pos);
  };
  if(sbody_) {
    // Not supporting direct read from stream body
  };
  return 0;
}

char* PayloadHTTPOutRaw::Content(PayloadRawInterface::Size_t pos) {
  if(!remake_header(false)) return NULL;
  if(pos == -1) pos = 0;
  if(pos < 0) return NULL;
  if(pos < header_.length()) {
    return (char*)(header_.c_str()+pos);
  };
  pos -= header_.length();
  if(rbody_) {
    return rbody_->Content(pos);
  };
  if(sbody_) {
    // Not supporting content from stream body
  };
  return NULL;
}

PayloadRawInterface::Size_t PayloadHTTPOutRaw::Size(void) const {
  if(!valid_) return 0;
  // Here Size() of Stream and Raw are conflicting. Currently we just
  // hope that nothing will be interested in logical size of stream.
  // TODO: either separate Size()s or implement chunked for Raw. 
  if(!((PayloadHTTPOutRaw&)(*this)).remake_header(false)) return 0;
  return header_.length()+body_size();
}

char* PayloadHTTPOutRaw::Insert(PayloadRawInterface::Size_t pos,PayloadRawInterface::Size_t size) {
  // Not allowing to manipulate body content through this interface
  return NULL;
}

char* PayloadHTTPOutRaw::Insert(const char* s,PayloadRawInterface::Size_t pos,PayloadRawInterface::Size_t size) {
  return NULL;
}

char* PayloadHTTPOutRaw::Buffer(unsigned int num) {
  if(!remake_header(false)) return NULL;
  if(num == 0) {
    return (char*)(header_.c_str());
  };
  --num;
  if(rbody_) {
    return rbody_->Buffer(num);
  };
  if(sbody_) {
    // Not supporting buffer access to stream body
  };
  return NULL;
}

PayloadRawInterface::Size_t PayloadHTTPOutRaw::BufferSize(unsigned int num) const {
  if(!((PayloadHTTPOutRaw&)(*this)).remake_header(false)) return 0;
  if(num == 0) {
    return header_.length();
  };
  --num;
  if(rbody_) {
    return rbody_->BufferSize(num);
  };
  if(sbody_) {
    // Not supporting buffer access to stream body
  };
  return 0;
}

PayloadRawInterface::Size_t PayloadHTTPOutRaw::BufferPos(unsigned int num) const {
  if(num == 0) {
    return 0;
  };
  if(!((PayloadHTTPOutRaw&)(*this)).remake_header(false)) return 0;
  PayloadRawInterface::Size_t pos = header_.length();
  if(rbody_) {
    --num;
    int n = 0;
    for(n=0;n<num;++n) {
      if(!rbody_->Buffer(n)) break;
      pos += rbody_->BufferSize(n);
    };
    return pos;
  };
  if(sbody_) {
    // Not supporting buffer access to stream body
  };
  return pos;
}

bool PayloadHTTPOutRaw::Truncate(PayloadRawInterface::Size_t size) {
  // TODO: Review it later. Truncate may be acting logically on *body_.
  if(!remake_header(false)) return false;
  if(size <= header_.length()) {
    if(rbody_ && body_own_) delete rbody_;
    if(sbody_ && body_own_) delete sbody_;
    rbody_=NULL; sbody_=NULL;
    header_.resize(size);
    return true;
  };
  if(rbody_) {
    return rbody_->Truncate(size-header_.length());
  };
  if(sbody_) {
    // Stream body does not support Truncate yet
  };
  return false;
}

PayloadRawInterface::Size_t PayloadHTTPOutStream::Size(void) const {
  if(!valid_) return 0;
  // Here Size() of Stream and Raw are conflicting. Currently we just
  // hope that nothing will be interested in logical size of stream.
  // TODO: either separate Size()s or implement chunked for Raw. 
  if(!((PayloadHTTPOutStream&)(*this)).remake_header(true)) return 0;
  return header_.length()+body_size();
}

/*
int PayloadHTTPOutStream::chunk_size_get(char* buf,int size,int l,uint64_t chunk_size) {
  if (chunk_size_str_.empty()) {
    // Generating new chunk size
    chunk_size_str_ = inttostr(chunk_size,16)+"\r\n";
    chunk_size_offset_ = 0;
  };
  if(chunk_size_offset_ < chunk_size_str_.length()) {
    // copy chunk size
    std::string::size_type cs = chunk_size_str_.length() - chunk_size_offset_;
    if(cs>(size-l)) cs=(size-l);
    ::memcpy(buf+l,chunk_size_str_.c_str()+chunk_size_offset_,cs);
    l+=cs; chunk_size_offset_+=cs;
  };
  return l;
}
*/

bool PayloadHTTPOutStream::Get(char* buf,int& size) {
  if(!valid_) return false;
  if(!remake_header(true)) return false;
  if(stream_finished_) return false;
  // Read header
  uint64_t bo = 0; // buf offset
  uint64_t bs = enable_header_out_?header_.length():0; // buf size
  int l = 0;
  if(l >= size) { size = l; return true; };
  if((bo+bs) > stream_offset_) {
    const char* p = header_.c_str();
    p+=(stream_offset_-bo);
    bs-=(stream_offset_-bo);
    if(bs>(size-l)) bs=(size-l);
    ::memcpy(buf+l,p,bs);
    l+=bs; stream_offset_+=bs;
    //chunk_size_str_ = ""; chunk_size_offset_ = 0;
  };
  bo+=bs;
  if(l >= size) { size = l; return true; }; // buffer is full
  // Read data
  if(rbody_ && enable_body_out_) {
    /* This code is only needed if stream and raw are mixed.
       Currently it is not possible hence it is not needed.
       But code is kept for future use. Code is not tested.
    for(unsigned int num = 0;;++num) {
      if(l >= size) { size = l; return true; }; // buffer is full
      const char* p = rbody_->Buffer(num);
      if(!p) {
        // No more buffers
        if(use_chunked_transfer_) {
          l = chunk_size_get(buf,size,l,0);
        };
        break;
      };
      bs = rbody_->BufferSize(num);
      if(bs <= 0) continue; // Protection against empty buffers
      if((bo+bs) > stream_offset_) {
        if((use_chunked_transfer_) && (bo == stream_offset_)) {
          l = chunk_size_get(buf,size,l,bs);
          if(l >= size) { size = l; return true; }; // buffer is full
        };
        p+=(stream_offset_-bo);
        bs-=(stream_offset_-bo);
        if(bs>(size-l)) bs=(size-l);
        ::memcpy(buf+l,p,bs);
        l+=bs; stream_offset_+=bs;
        chunk_size_str_ = ""; chunk_size_offset_ = 0;
      };
      bo+=bs;
    };
    size = l;
    if(l > 0) return true;
    return false;
    */
    size = 0;
    return false;
  };
  if(sbody_ && enable_body_out_) {
    if(use_chunked_transfer_) {
      // It is impossible to know size of chunk
      // in advance. So first prelimnary size is
      // is generated and later adjusted.
      // The problem is that if supplied buffer is
      // not enough for size and at least one
      // byte of data that can cause infinite loop.
      // To avoid that false is returned.
      // So in case of very short buffer transmission
      // will fail.
      std::string chunk_size_str = inttostr(size,16)+"\r\n";
      std::string::size_type cs = chunk_size_str.length();
      if((cs+2+1) > (size-l)) {
        // too small buffer
        size = l;
        return (l > 0);
      };
      int s = size-l-cs-2;
      if(sbody_->Get(buf+l+cs,s)) {
        if(s > 0) {
          chunk_size_str = inttostr(s,16)+"\r\n";
          if(chunk_size_str.length() > cs) { // paranoic
            size = 0;
            return false; 
          };
          ::memset(buf+l,'0',cs);
          ::memcpy(buf+l+(cs-chunk_size_str.length()),chunk_size_str.c_str(),chunk_size_str.length()); 
          ::memcpy(buf+l+cs+s,"\r\n",2);
          stream_offset_+=s; l+=(cs+s+2);
        };
        size = l;
        return true;
      };
      // Write 0 chunk size first time. Any later request must just fail.
      if(5 > (size-l)) {
        // too small buffer
        size = l;
        return (l > 0);
      };
      ::memcpy(buf+l,"0\r\n\r\n",5);
      l+=5;
      size = l; stream_finished_ = true;
      return true;
    } else {
      int s = size-l;
      if(sbody_->Get(buf+l,s)) {
        stream_offset_+=s; l+=s;
        size = l;
        return true;
      };
      stream_finished_ = true;
    };
    size = l;
    return false; 
  };
  size = l;
  if(l > 0) return true;
  return false;
}

bool PayloadHTTPOutStream::Get(PayloadStreamInterface& dest,int& size) {
  if((stream_offset_ > 0) || (size >= 0)) {
    // If it is not first call or if size control is requested
    // then convenience method PayloadStreamInterface::Get is
    // used, which finally calls PayloadHTTPOutStream::Get.
    return PayloadStreamInterface::Get(dest,size);
  }
  // But if whole content is requested at once we use faster Flush* methods
  Flush(dest);
  return false; // stream finished
}

// Stream interface is meant to be used only for reading.
bool PayloadHTTPOutStream::Put(const char* /* buf */,PayloadStreamInterface::Size_t /* size */) {
  return false;
}

int PayloadHTTPOutStream::Timeout(void) const {
  if(!sbody_) return 0;
  return sbody_->Timeout();
}

void PayloadHTTPOutStream::Timeout(int to) {
  if(sbody_) sbody_->Timeout(to);
}

PayloadStreamInterface::Size_t PayloadHTTPOutStream::Pos(void) const {
  return stream_offset_;
}

PayloadStreamInterface::Size_t PayloadHTTPOutStream::Limit(void) const {
  if(!((PayloadHTTPOutStream&)(*this)).remake_header(true)) return 0;
  PayloadStreamInterface::Size_t limit = 0;
  if(enable_header_out_) limit += header_.length();
  if(enable_body_out_) limit += body_size();
  return limit;
}

//-------------------------------------------------------------------

} // namespace ArcMCCHTTP
