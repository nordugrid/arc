#include "HTTPSClient.h"

namespace Arc {
  
  HTTPResponseHeader::HTTPResponseHeader(bool alive):keep_alive(alive),content_length_passed(false),content_range_passed(false) { };
  
  void HTTPResponseHeader::reset(bool alive) {
    keep_alive=alive;
    content_length_passed=false;
    content_range_passed=false;
    expires=0; last_modified=0;
  }
  
  bool HTTPResponseHeader::set(const char* name,const char* value) {
    if(strcasecmp("Connection:",name) == 0) {
      if(strcasecmp("close",value) == 0) { keep_alive=0; }
      else if(strcasecmp("keep-alive",value) == 0) { keep_alive=1; }
      else { return false; };
      return true;
    } else if(strcasecmp("Content-Length:",name) == 0) {
      content_length_passed=false;
      char *e;
      content_length=strtoull(value,&e,10);
      if((*e) != 0) return false;
      content_length_passed=true;
      return true;
    } else if(strcasecmp("Content-Range:",name) == 0) {
      content_range_passed=false;
      content_size=0;
      const char* p = value; for(;*p;p++) if(isspace(*p)) break;
      if(strncasecmp("bytes",value,p-value) != 0) return false;
      for(;*p;p++) if(!isspace(*p)) break;
      char *e;
      content_start=strtoull(p,&e,10);
      if((*e) != '-') return false; 
      p=e+1; content_end=strtoull(p,&e,10); p=e;
      if(((*e) != '/') && ((*e) != 0)) return false;
      if(content_start > content_end) return false;
      if((*p) == '/') {
        p++; content_size=strtoull(p,&e,10);
        if((*e) != 0) { return false; }
      };
      content_range_passed=true;
      return true;
    } else if(strcasecmp("Expires:",name) == 0) {
      expires=Time(value);
    } else if(strcasecmp("Last-Modified:",name) == 0) {
      last_modified=Time(value);
    };
    return true;
  }
  
} // namespace Arc
