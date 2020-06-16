#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>
#include <iostream>
#include <string.h>

#include <arc/Run.h>
#include <arc/ArcLocation.h>

#include "hostname_resolver.h"

#include "HostnameResolver.h"

namespace Arc {

  #define READ_TIMEOUT (60000)
  #define WRITE_TIMEOUT (60000)

  static bool sread(Run& r,void* buf,size_t size) {
    while(size) {
      int l = r.ReadStdout(READ_TIMEOUT,(char*)buf,size);
      if(l <= 0) return false;
      size-=l;
      buf = (void*)(((char*)buf)+l);
    };
    return true;
  }

  static bool swrite(Run& r,const void* buf,size_t size) {
    while(size) {
      int l = r.WriteStdin(WRITE_TIMEOUT,(const char*)buf,size);
      if(l <= 0) return false;
      size-=l;
      buf = (void*)(((char*)buf)+l);
    };
    return true;
  }

#define ABORTALL { dispose_executer(hostname_resolver_); hostname_resolver_=NULL; continue; }

#define STARTHEADER(CMD,SIZE) { \
  if(!hostname_resolver_) break; \
  if(!(hostname_resolver_->Running())) break; \
  header_t header; \
  header.cmd = CMD; \
  header.size = SIZE; \
  if(!swrite(*hostname_resolver_,&header,sizeof(header))) ABORTALL; \
}

#define ENDHEADER(CMD,SIZE) { \
  header_t header; \
  if(!sread(*hostname_resolver_,&header,sizeof(header))) ABORTALL; \
  if((header.cmd != CMD) || (header.size != (sizeof(res)+sizeof(errno_)+SIZE))) ABORTALL; \
  if(!sread(*hostname_resolver_,&res,sizeof(res))) ABORTALL; \
  if(!sread(*hostname_resolver_,&errno_,sizeof(errno_))) ABORTALL; \
}

  static void release_executer(Run* hostname_resolver) {
    delete hostname_resolver;
  }

  static void dispose_executer(Run* hostname_resolver) {
    delete hostname_resolver;
  }

  static bool do_tests = false;

  static Run* acquire_executer() {
    std::list<std::string> argv;
    if(!do_tests) {
      argv.push_back(Arc::ArcLocation::Get()+G_DIR_SEPARATOR_S+PKGLIBSUBDIR+G_DIR_SEPARATOR_S+"arc-hostname-resolver");
    } else {
      argv.push_back(std::string("..")+G_DIR_SEPARATOR_S+"arc-hostname-resolver");
    }
    argv.push_back("0");
    argv.push_back("1");
    Run* hostname_resolver_ = new Run(argv);
    hostname_resolver_->KeepStdin(false);
    hostname_resolver_->KeepStdout(false);
    hostname_resolver_->KeepStderr(true);
    if(!(hostname_resolver_->Start())) {
      delete hostname_resolver_;
      hostname_resolver_ = NULL;
      return NULL;
    }
    return hostname_resolver_;
  }

  static bool swrite_string(Run& r,const std::string& str) {
    int l = str.length();
    if(!swrite(r,&l,sizeof(l))) return false;
    if(!swrite(r,str.c_str(),l)) return false;
    return true;
  }

#define RETRYLOOP Glib::Mutex::Lock mlock(lock_); for(int n = 2; n && (hostname_resolver_?hostname_resolver_:(hostname_resolver_=acquire_executer())) ;--n)

#define NORETRYLOOP Glib::Mutex::Lock mlock(lock_); for(int n = 1; n && (hostname_resolver_?hostname_resolver_:(hostname_resolver_=acquire_executer())) ;--n)

  HostnameResolver::SockAddr::SockAddr():family(0),length(0),addr(NULL) {
  }
 
  HostnameResolver::SockAddr::SockAddr(SockAddr const& other):family(0),length(0),addr(NULL) {
    operator=(other);
  }

  HostnameResolver::SockAddr& HostnameResolver::SockAddr::operator=(SockAddr const& other) {
    if(this == &other) return *this;
    family = other.family;
    length = other.length;
    ::free(addr);
    addr = (sockaddr*)::malloc(length);
    memcpy(addr,other.addr,length);
    return *this;
  }

  HostnameResolver::SockAddr::~SockAddr() {
    ::free(addr);
  }

  HostnameResolver::HostnameResolver(void):hostname_resolver_(NULL),errno_(0) {
    hostname_resolver_ = acquire_executer();
  }

  HostnameResolver::~HostnameResolver(void) {
    release_executer(hostname_resolver_);
    hostname_resolver_ = NULL;
  }

  bool HostnameResolver::ping(void) {
    RETRYLOOP {
      STARTHEADER(CMD_PING,0);
      header_t header;
      if(!sread(*hostname_resolver_,&header,sizeof(header))) ABORTALL;
      if((header.cmd != CMD_PING) || (header.size != 0)) ABORTALL;
      return true;
    }
    return false;
  }

  int HostnameResolver::hr_resolve(std::string const& node, std::string const& service, bool local, std::list<SockAddr>& addrs) {
    NORETRYLOOP {
    int command = local?CMD_RESOLVE_TCP_LOCAL:CMD_RESOLVE_TCP_REMOTE;
    STARTHEADER(command,sizeof(int)+node.length()+sizeof(int)+service.length());
    if(!swrite_string(*hostname_resolver_,node)) ABORTALL;
    if(!swrite_string(*hostname_resolver_,service)) ABORTALL;
    int res = 0;
    header_t header;
    if(!sread(*hostname_resolver_,&header,sizeof(header))) ABORTALL;
    if((header.cmd != command) || (header.size < (sizeof(res)+sizeof(errno_)))) ABORTALL; \
    if(!sread(*hostname_resolver_,&res,sizeof(res))) ABORTALL;
    if(!sread(*hostname_resolver_,&errno_,sizeof(errno_))) ABORTALL;
    header.size -= sizeof(res)+sizeof(errno_);
    while(hostname_resolver_ && (header.size > 0)) {
      SockAddr addr;
      if(header.size < sizeof(addr.family)) ABORTALL;
      if(!sread(*hostname_resolver_,&addr.family,sizeof(addr.family))) ABORTALL;
      header.size-=sizeof(addr.family);
      if(header.size < sizeof(addr.length)) ABORTALL;
      if(!sread(*hostname_resolver_,&addr.length,sizeof(addr.length))) ABORTALL;
      header.size-=sizeof(addr.length);
      if(header.size < addr.length) ABORTALL;
      if((addr.addr = (sockaddr*)::malloc(addr.length)) == NULL) ABORTALL;
      if(!sread(*hostname_resolver_,addr.addr,addr.length)) ABORTALL;
      header.size-=addr.length;
      addrs.push_back(addr);
    };
    if(!hostname_resolver_) continue;
    return res;
    }
    errno_ = -1;
    return -1;
  }

  void HostnameResolver::testtune(void) {
    do_tests = true;
  }

  static HostnameResolverContainer hrs_(0,100);

  HostnameResolver* HostnameResolver::Acquire(void) {
    return hrs_.Acquire();
  }

  void HostnameResolver::Release(HostnameResolver* hr) {
    hrs_.Release(hr);
  }

  HostnameResolverContainer::HostnameResolverContainer(unsigned int minval,unsigned int maxval):min_(minval),max_(maxval) {
    KeepRange();
  }

  HostnameResolverContainer::HostnameResolverContainer(void):min_(1),max_(10) {
    KeepRange();
  }

  HostnameResolverContainer::~HostnameResolverContainer(void) {
    Glib::Mutex::Lock lock(lock_);
    for(std::list<HostnameResolver*>::iterator hr = hrs_.begin();hr != hrs_.end();++hr) {
      delete *hr;
    }
  }

  HostnameResolver* HostnameResolverContainer::Acquire(void) {
    Glib::Mutex::Lock lock(lock_);
    HostnameResolver* r = NULL;
    for(std::list<HostnameResolver*>::iterator hr = hrs_.begin();hr != hrs_.end();) {
      r = *hr; hr = hrs_.erase(hr);
      // Test if it still works
      if(r->ping()) break;
      // Broken proxy
      delete r; r = NULL;
    }
    // If no proxies - make new
    if(!r) r = new HostnameResolver;
    KeepRange();
    return r;
  }

  void HostnameResolverContainer::Release(HostnameResolver* hr) {
    Glib::Mutex::Lock lock(lock_);
    if(!hr) return;
    hrs_.push_back(hr);
    KeepRange();
    return;
  }

  void HostnameResolverContainer::SetMin(unsigned int val) {
    Glib::Mutex::Lock lock(lock_);
    min_ = val;
    KeepRange();
  }

  void HostnameResolverContainer::SetMax(unsigned int val) {
    Glib::Mutex::Lock lock(lock_);
    min_ = val;
    KeepRange();
  }

  void HostnameResolverContainer::KeepRange(void) {
    while(hrs_.size() > ((max_>=min_)?max_:min_)) {
      HostnameResolver* fa = hrs_.front();
      hrs_.pop_front();
      delete fa;
    }
    while(hrs_.size() < ((min_<=max_)?min_:max_)) {
      hrs_.push_back(new HostnameResolver);
    }
  }

}

