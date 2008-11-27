#include "HTTPSClient.h"

namespace Arc {
  
  int HTTPSClientSOAP::local_fsend(struct soap *sp, const char* buf, size_t l) {
    if(sp->socket != 0) return SOAP_SSL_ERROR;
    HTTPSClientSOAP *it = (HTTPSClientSOAP*)(sp->user);
  
    // do not register read - so cond will be set by writer only
    if(!it->c->write(buf,l)) {
      return SOAP_SSL_ERROR;
    };
    bool isread,iswritten;
    if(!it->c->transfer(isread,iswritten,it->timeout)) {
      logger.msg(ERROR, "Timeout while sending SOAP request");
      return SOAP_SSL_ERROR;
    };
    if(!iswritten) {
      logger.msg(ERROR, "Error sending data to server");
      return SOAP_SSL_ERROR;
    };
    return SOAP_OK;
  }
  
  size_t HTTPSClientSOAP::local_frecv(struct soap* sp, char* buf, size_t l) {
    if(sp->socket != 0) return 0;
    HTTPSClientSOAP *it = (HTTPSClientSOAP*)(sp->user);
    it->answer_size = l;
    if(!it->c->read(buf,&(it->answer_size))) {
      return 0;
    };
    bool isread,iswritten;
    if(!it->c->transfer(isread,iswritten,it->timeout)) {
      return 0;
    };
    if(!isread) return 0;
    return it->answer_size;
  }
  
  int HTTPSClientSOAP::local_fopen(struct soap* sp, const char* endpoint, const char* host, int port) {
    if(sp->socket == 0) return 0;
    HTTPSClientSOAP *it = (HTTPSClientSOAP*)(sp->user);
    if(it->connect() != 0) return -1;
    sp->socket=0;
    return 0;
  };
  
  int HTTPSClientSOAP::local_fclose(struct soap* sp) {
    if(sp->socket == -1) return 0;
    HTTPSClientSOAP *it = (HTTPSClientSOAP*)(sp->user);
    if(it->disconnect() != 0) return -1;
    sp->socket=-1;
    return 0;
  };
  
  
  HTTPSClientSOAP::HTTPSClientSOAP(const char* base,struct soap *sp,bool gssapi_server, int soap_timeout, bool check_host_cert):HTTPSClient(base,true,gssapi_server,soap_timeout, check_host_cert),soap(sp) {
    /* intialize soap */
    namespaces=NULL;
    soap_init(soap);
    soap->fsend=&local_fsend;
    soap->frecv=&local_frecv;
    soap->fopen=&local_fopen;
    soap->fclose=&local_fclose;
    soap->http_version="1.1";
    soap->socket=-1;
    soap->keep_alive=1;
    soap_set_imode(soap,SOAP_IO_KEEPALIVE);
    soap_set_omode(soap,SOAP_IO_KEEPALIVE);
    soap->user=this;
    soap_url=base;
    std::string::size_type n = soap_url.find(':');
    if(n != std::string::npos) soap_url.replace(0,n,"http"); // fake url for gSOAP
  }  
  
  HTTPSClientSOAP::~HTTPSClientSOAP(void) {
    soap_destroy(soap);
    soap_end(soap);
    soap_done(soap);
  }
  
  void HTTPSClientSOAP::reset(void) {
    soap_destroy(soap);
    soap_end(soap);
  }
  
  void HTTPSClientSOAP::AddSOAPNamespaces(struct Namespace* namespaces_) {
    if(!namespaces_) return;
    int n = 0;
    int n_ = 0;
    struct Namespace *ns;
    if(namespaces) for(ns=namespaces;ns->id;ns++) { n++; };
    for(ns=namespaces_;ns->id;ns++) { n_++; };
    ns=(struct Namespace*)realloc(namespaces,sizeof(struct Namespace)*(n+n_+1));
    if(ns == NULL) return;
    memcpy(ns+n,namespaces_,sizeof(struct Namespace)*(n_+1));
    namespaces=ns;
    if(soap) soap->namespaces=namespaces;
  }
  
  std::string HTTPSClientSOAP::SOAP_URL(const char* path) {
    std::string s = soap_url;
    if(s.length() <= 0) return s;
    if(s[s.length()-1] != '/') s+="/";
    if(path == NULL) return s;
    if(path[0] == '/') path++;
    s+=path;
    return s;
  }
  
  const struct Namespace* HTTPSClientSOAP::Namespaces(void) {
    if(namespaces) return namespaces;
    if(soap) return soap->namespaces;
    return NULL;
  }
}  // namespace Arc
