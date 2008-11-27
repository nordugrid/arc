#include <arc/StringConv.h>
#include <arc/globusutils/GlobusErrorUtils.h>

#include "HTTPSClient.h"

namespace Arc {

  Logger HTTPSClient::logger(Logger::getRootLogger(), "HTTPSClient");
  
  HTTPSClient::HTTPSClient(const char* base,bool heavy_encryption,bool gssapi_server, int timeout, bool check_host_cert) try: base_url(base), timeout(timeout*1000){
    c=NULL;
    cred=new GSSCredential();
    valid=false; connected=false;
    /* initialize globus io connection related objects */
    if(base_url.Protocol() == "http") {
      const char * proxy = getenv("ARC_HTTP_PROXY");
      if(!proxy) proxy = getenv("NORDUGRID_HTTP_PROXY");
      if(proxy) {
        proxy_hostname = proxy;
        proxy_port = 8000;
        std::string::size_type n = proxy_hostname.find(':');
        if(n != std::string::npos) {
          proxy_port=atoi(proxy_hostname.c_str()+n+1);
          proxy_hostname.resize(n);
        };
      };
    };
    if(proxy_hostname.empty()) {
      if(!gssapi_server) {
        c=new HTTPSClientConnectorGlobus(base,heavy_encryption);
      } else {
        c=new HTTPSClientConnectorGSSAPI(base,heavy_encryption, timeout*1000, *cred, check_host_cert);
      };
    } else {
      std::string u = "http://"+proxy_hostname+":"+tostring<int>(proxy_port);
      if(!gssapi_server) {
        c=new HTTPSClientConnectorGlobus(u.c_str(),heavy_encryption);
      } else {
        c=new HTTPSClientConnectorGSSAPI(u.c_str(),heavy_encryption, timeout*1000, *cred, check_host_cert);
      };
    };
    valid=true;
  } catch(std::exception e) {
    valid=false; connected=false;
  }
  
  bool HTTPSClient::credentials(const char* filename) {
    if(!filename) return false;
    cred = new GSSCredential(filename, "", "");
    //gss_cred_id_t cred_new = *cred;
    if(*cred == GSS_C_NO_CREDENTIAL) {
      delete cred;
      return false;
    }
    if(!c->credentials(*cred)) {
      delete cred;
      return false;
    };
    return true;
  }
  
  int HTTPSClient::connect(void) {
    if(connected) return 0;
    if(!valid) return -1;
    if(!c->connect()) return -1;
    connected=true;
    return 0;
  }
  
  int HTTPSClient::disconnect(void) {
    if(!connected) return 0;
    c->disconnect();
    connected=false;
    return 0;
  }
  
  HTTPSClient::~HTTPSClient(void) {
    if(!valid) return;
    disconnect();
    if(c) delete c;
    if(*cred != GSS_C_NO_CREDENTIAL) {
      delete cred;
    };
  }
  
  void HTTPSClient::clear_input(void) {
    if(!valid) return;
    char buf[256];
    unsigned int l;
    bool isread,iswritten;
    for(;;) {
      l=sizeof(buf);
      if(!c->read(buf,&l)) return;
      if(!c->transfer(isread,iswritten,0)) { c->read(); return; };
      if(!isread) { c->read(); return; };
      logger.msg(VERBOSE, "clear_input: %s", buf);
    };
  }
  
  void HTTPSClient::analyze_response_line(char* line) {
    for(;*line;line++) if(!isspace(*line)) break;
    int len = strlen(line);
    if(len<2) return; // too short
    if(answer_count==0) {  // first line
      bool answer_keep_alive = true;
      answer_code=0;
      char* p = line;
      char* http_version = p;
      for(;*p;p++) if(isspace(*p)) break; (*p)=0; p++;
      for(;*p;p++) if(!isspace(*p)) break;
      char* code = p;
      for(;*p;p++) if(isspace(*p)) break; (*p)=0; p++;
      for(;*p;p++) if(!isspace(*p)) break;
      char* reason = p;
      char *e;
      answer_code=strtoul(code,&e,10);
      if((*e) != 0) return;
      answer_reason=reason;
      answer_count++;
      if(strcmp(http_version,"HTTP/1.1") == 0) { answer_keep_alive=true; }
      else { answer_keep_alive=false; };
      fields.reset(answer_keep_alive);
    } else  {
      char* token = line; for(;*token;token++) if(isspace(*token)) break;
      if(*token) {
        (*token)=0; token++;
        for(;*token;token++) if(!isspace(*token)) break;
      };
      fields.set(line,token);
    };
  }
  
  // read server response
  int HTTPSClient::read_response_header(void) {
    bool isread,iswritten;
    answer_count=0; // line counter
    // let external procedures manage timeouts
    if(!c->transfer(isread,iswritten,-1)) {
      disconnect();
      return -1;
    };
    if(answer_size != 0) isread=true;
    if((!isread) && (!iswritten)) {
      disconnect();
      return -1;
    };
    char line_buf[256];
    int line_p = 0;
    for(;;) {
      answer_buf[answer_size]=0;
      char* p = strchr(answer_buf,'\n');
      unsigned int l = answer_size;
      if(p) l=(p-answer_buf)+1; // available data
      unsigned int ll = (sizeof(line_buf)-1-line_p);
      if(ll>l) ll=l; // available space
      memcpy(line_buf+line_p,answer_buf,ll); line_p+=ll; line_buf[line_p]=0;
      if(l<answer_size) { memmove(answer_buf,answer_buf+l,answer_size-l); };
      answer_size-=l;
      if(p) {  // eol 
        for(;line_p;) {
          if((line_buf[line_p-1] != '\r') && (line_buf[line_p-1] != '\n')) break;
          line_p--;
        };
        line_buf[line_p]=0;
        if(line_p == 0) break; // end of header
        logger.msg(VERBOSE, "read_response_header: line: %s", line_buf);
        analyze_response_line(line_buf);
        line_p=0;
      };
      if(answer_size>0) continue;
      // request new portion
      answer_size=sizeof(answer_buf)-1; 
      if(isread && (!c->read(answer_buf,&answer_size))) {
        disconnect();
        return -1;
      };
      if(!c->transfer(isread,iswritten,timeout)) {
        logger.msg(ERROR, "Timeout while reading response header");
        disconnect();
        return -1;
      };
      if(!isread) {
        logger.msg(ERROR, "Error while reading response header");
        disconnect();
        return -1;
      };
    };
    logger.msg(VERBOSE, "read_response_header: header finished");
    return 0;
  }
  
  int HTTPSClient::skip_response_entity(void) {
    logger.msg(VERBOSE, "skip_response_entity");
    if(fields.haveContentLength() || fields.haveContentRange()) {
      unsigned long long int size = fields.ContentLength();
      logger.msg(VERBOSE, "skip_response_entity: size: %llu", size);
      if(size<=answer_size) {
        memmove(answer_buf,answer_buf+size,answer_size-size);
        answer_size-=size;
        logger.msg(VERBOSE, "skip_response_entity: already have all");
        return 0;
      };
      size-=answer_size;
      logger.msg(VERBOSE, "skip_response_entity: size left: %llu", size);
      // read size bytes
      char buf[1024];
      for(;size;) {
        logger.msg(VERBOSE, "skip_response_entity:  to read: %llu", size);
        answer_size=sizeof(buf);
        if(!c->read(buf,&answer_size)) {
          disconnect(); return -1;
        };
        bool isread,iswritten;
        if(!c->transfer(isread,iswritten,timeout)) {
          logger.msg(VERBOSE, "skip_response_entity: timeout %llu", size);
          disconnect(); return -1; // timeout
        };
        if(!isread) { disconnect(); return -1; }; // failure
        size-=answer_size;
        logger.msg(VERBOSE, "skip_response_entity: read: %u (%llu)", answer_size, size);
      };
      logger.msg(VERBOSE, "skip_response_entity: read all");
      return 0;
    };
    if(fields.KeepAlive()) {
      logger.msg(VERBOSE, "skip_response_entity: no entity");
      // no entity passed at all
      return 0;
    };
    logger.msg(VERBOSE, "skip_response_entity: unknown size");
    // can't handle unknown size - let connection be closed
    return 0;
  }
  
  int HTTPSClient::make_header(const char* path,
        unsigned long long int offset,unsigned long long int size,
        unsigned long long int fd_size,std::string& header) {
    // create header
    if(!valid) return -1;
    if(path[0] == '/') path++;
    header = "PUT ";
    std::string url_path;
    if(proxy_hostname.length() == 0) {
      url_path=base_url.Path();
    } else {
      url_path=base_url.Protocol()+"://"+base_url.Host()+":"+tostring(base_url.Port())+base_url.Path();
    };
    if(path[0]) {
      if(url_path[url_path.length()-1] != '/') url_path+="/";
      url_path+=path;
    };
    if(!base_url.HTTPOptions().empty()) {
      url_path+='?'+URL::OptionString(base_url.HTTPOptions(), '&');
    };
    std::string url_host = base_url.Host()+":"+tostring(base_url.Port());
    header+=url_path; header+=" HTTP/1.1\r\n";
    header+="Host: "+url_host+"\r\n";
    header+="Connection: keep-alive\r\n";
    header+="Content-Length: "+tostring(size)+"\r\n";
    header+="Content-Range: bytes "+tostring(offset)+"-"+tostring(offset+size-1);
    if(fd_size>=size) {
      header+="/"+tostring(fd_size);
    };
    header+="\r\n";
    header+="\r\n";
    return 0;
  }
  
  /*
    HTTP PUT method
    Content of 'buf' with size 'size' is sent to 'path'. 
    Treated as part size 'fd_size' starting from 'offset'.
  */
  int HTTPSClient::PUT(const char* path,
        unsigned long long int offset,unsigned long long int size,
        const unsigned char* buf,unsigned long long int fd_size,bool wait) {
    if(!connected) {
      logger.msg(ERROR, "Not connected");
      return -1;
    };
    // send header
    // create header
    std::string header;
    make_header(path,offset,size,fd_size,header);
    c->clear();
    answer_size=sizeof(answer_buf)-1;
    if(!c->read(answer_buf,&answer_size)) {
      disconnect(); return -1;
    };
    // send header
    if(!c->write(header.c_str(),header.length())) {
      disconnect(); return -1;
    };
    bool isread,iswritten;
    if(!c->transfer(isread,iswritten,timeout)) {
      logger.msg(ERROR, "Timeout sending header");
      disconnect(); return -1;
    };
    if(!iswritten) { // Server responded too early
      logger.msg(ERROR, "Early response from server");
      disconnect(); return -1;
    };
    // send body
    if(!c->write((const char*)buf,size)) {
      disconnect(); return -1;
    };
    // read server response
    if(read_response_header() != 0) {
      logger.msg(ERROR, "No response from server received");
      disconnect(); return -1;
    };
    if(!c->eofwrite()) {
      logger.msg(ERROR, "Failed to send body");
      disconnect(); return -1;
    };
    if(fields.KeepAlive()) {  // skip entity only if trying to keep connection
      if(skip_response_entity() != 0) {
        logger.msg(ERROR, "Failure while receiving entity");
        disconnect();
        return -1;
      };
      c->read(); // just in case
    } else {
      disconnect();
    };
    if(answer_code != HTTP_OK) return -1;
    return 0;
  }
  
  /*
    HTTP GET method
  */
  int HTTPSClient::GET_header(const char* path,
        unsigned long long int offset,unsigned long long int size) {
    // create header
    if(!valid) return -1;
    // *** Generate header
    if(path[0] == '/') path++;
    std::string header = "GET ";
    std::string url_path;
    if(proxy_hostname.length() == 0) {
      url_path=base_url.Path();
    } else {
      url_path=base_url.Protocol()+"://"+base_url.Host()+":"+tostring(base_url.Port())+base_url.Path();
    };
    if(path[0]) {
      if(url_path[url_path.length()-1] != '/') url_path+="/"; 
      url_path+=path;
    };
    if(!base_url.HTTPOptions().empty()) {
      url_path+='?'+URL::OptionString(base_url.HTTPOptions(), '&');
    };
    std::string url_host = base_url.Host()+":"+tostring(base_url.Port());
    header+=url_path; header+=" HTTP/1.1\r\n";
    header+="Host: "+url_host+"\r\n";
    header+="Connection: keep-alive\r\n";
    header+="Range: bytes="+tostring(offset)+"-"+tostring(offset+size-1)+"\r\n";
    header+="\r\n";
    logger.msg(VERBOSE, "header: %s", header);
    // *** Send header 
    c->clear();
    // get ready for any answer
    answer_size=sizeof(answer_buf);
    if(!c->read(answer_buf,&answer_size)) {
      disconnect();
      return -1;
    };
    // send
    if(!c->write(header.c_str(),header.length())) {
      disconnect();
      return -1;
    };
    bool isread, iswritten;
    for(;;) {
      if(!c->transfer(isread,iswritten,timeout)) {
        logger.msg(ERROR, "Timeout while sending header");
        disconnect();
        return -1;
      };
      if(iswritten) break;
      if(isread) continue;
      logger.msg(ERROR, "Failed to send header");
      disconnect();
      return -1;
    };
    return 0;
  }
  
  int HTTPSClient::GET(const char* path,
        unsigned long long int offset,unsigned long long int size,
        get_callback_t callback,void* arg,
        unsigned char* buf,unsigned long long int bufsize) {
    if(!connected) {
      logger.msg(ERROR, "Not connected");
      return -1;
    };
    // send header
    if(GET_header(path,offset,size)) {
      // There are servers which close connection even if they shouldn't - retry here
      if(connect()) return -1;
      if(GET_header(path,offset,size)) return -1;
    };
    // read server response
    if(read_response_header() != 0) {
      logger.msg(ERROR, "No response from server received");
      disconnect();
      return -1;
    };
    if(answer_code == 416) { // out of range
      if(skip_response_entity() != 0) {
        disconnect(); return -1;
      };
      if(!fields.KeepAlive()) {
        logger.msg(DEBUG, "GET: connection to be closed");
        disconnect();
      };
      return 0;
    };
    if((answer_code != 200) && (answer_code != 206)) {
      if(skip_response_entity() != 0) {
        disconnect(); return -1;
      };
      if(!fields.KeepAlive()) {
      logger.msg(DEBUG, "GET: connection to be closed");
        disconnect();
      };
      return -1;
    };
    logger.msg(DEBUG, "GET: header is read - rest: %u", answer_size);
    unsigned long long c_offset = 0;
    if(fields.haveContentRange()) c_offset=fields.ContentStart();
    bool have_length = fields.haveContentLength() || fields.haveContentRange();
    unsigned long long length = fields.ContentLength();
    // take rest of already read data
    if(answer_size) {
      if(have_length) if(answer_size > length) answer_size=length;
      logger.msg(VERBOSE, "GET: calling callback(rest): content: %s", answer_buf);
      logger.msg(VERBOSE, "GET: calling callback(rest): size: %u", answer_size);
      logger.msg(VERBOSE, "GET: calling callback(rest): offset: %llu", c_offset);
      unsigned char* in_buf = (unsigned char*)answer_buf;
      unsigned long  in_size = answer_size;
      for(;;) {
        if(in_size == 0) break;
        if(buf) {
          unsigned long l = in_size;
          if(l>bufsize) l=bufsize;
          memcpy(buf,in_buf,l);
          if(callback(c_offset,l,&buf,&bufsize,arg) != 0) {
            logger.msg(ERROR, "GET callback returned error");
            disconnect(); // ?????????????????
            return -1;
          };
          in_buf+=l; c_offset+=l; in_size-=l;
        } else {
          unsigned char* in_buf_ = in_buf;
          if(callback(c_offset,in_size,&in_buf_,&bufsize,arg) != 0) {
            logger.msg(ERROR, "GET callback returned error");
            disconnect(); // ?????????????????
            return -1;
          };
          if(in_buf_ != in_buf) buf=in_buf_;
          in_buf+=in_size; c_offset+=in_size; in_size=0;
        };
      };
      if(have_length) length-=answer_size;
    };
    unsigned char* in_buf = NULL;
    for(;;) {
      if(have_length) if(length == 0) break; 
      if(buf == NULL) {
        if(in_buf == NULL) in_buf=(unsigned char*)malloc(65536);
        if(in_buf == NULL) {
          logger.msg(ERROR, "Failed to allocate memory");
          disconnect();
          return -1;
        };
        buf=in_buf; bufsize=65536;
      };
      /// cond_read.reset();
      answer_size=bufsize;
      if(!c->read((char*)buf,&answer_size)) {
        logger.msg(ERROR, "Failed while reading response content");
        disconnect();
        if(in_buf) free(in_buf);
        return -1;
      };
      bool isread,iswritten;
      if(!c->transfer(isread,iswritten,timeout)) {
        logger.msg(ERROR, "Timeout while reading response content: ");
        disconnect();
        if(in_buf) free(in_buf);
        return -1; // timeout
      };
      if(!isread) { // failure
        if(c->eofread()) { // eof - ok if length not specified
          if(!have_length) { disconnect(); break; };
        };
        logger.msg(ERROR, "Error while reading response content: ");
        disconnect();
        if(in_buf) free(in_buf);
        return -1;
      };
      logger.msg(VERBOSE, "GET: calling callback: content: %s", buf);
      logger.msg(VERBOSE, "GET: calling callback: size: %u", answer_size);
      logger.msg(VERBOSE, "GET: calling callback: offset: %llu", c_offset);
      if(callback(c_offset,answer_size,&buf,&bufsize,arg) != 0) {
        logger.msg(ERROR, "GET callback returned error");
        disconnect();
        if(in_buf) free(in_buf);
        return -1;
      };
      c_offset+=answer_size;
      if(have_length) length-=answer_size;
    };
    // cancel? - just in case
    if(in_buf) free(in_buf);
    if(!fields.KeepAlive()) {
      logger.msg(DEBUG, "GET: connection to be closed");
      disconnect();
    };
    return 0;
  }
  
  // -------------------------------------------------
  
  bool HTTPSClientConnector::connect(void) { return false; }
  
  bool HTTPSClientConnector::disconnect(void)  { return false; }
  
  bool HTTPSClientConnector::clear(void) { return false; }
  
  bool HTTPSClientConnector::read(char* buf,unsigned int* size) { return false; }
  
  bool HTTPSClientConnector::write(const char* buf,unsigned int size) { return false; }
  
  bool HTTPSClientConnector::transfer(bool& read,bool& write,int timeout) { return false; }
  
  bool HTTPSClientConnector::eofread(void) { return false; }
  
  bool HTTPSClientConnector::eofwrite(void) { return false; }
  
  HTTPSClientConnector::HTTPSClientConnector(void) { }
  
  HTTPSClientConnector::~HTTPSClientConnector(void) { }
  
  bool HTTPSClientConnector::credentials(gss_cred_id_t cred) { return false; }
  
} // namespace Arc
