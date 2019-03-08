#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ctime>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <netinet/tcp.h>
#include <fcntl.h>

#include <glibmm.h>

#include <arc/StringConv.h>
#include <arc/Utils.h>
#include <arc/HostnameResolver.h>
#include "PayloadTCPSocket.h"

#define USE_REMOTE_HOSTNAME_RESOLVER 1

namespace ArcMCCTCP {

using namespace Arc;

static int spoll(int h, int timeout, unsigned int& events) {
  int r;
  // Second resolution is enough
  time_t c_time = time(NULL);
  time_t f_time = c_time + timeout;
  struct pollfd fd;
  for(;;) {
    fd.fd=h; fd.events=events; fd.revents=0;
    r = ::poll(&fd,1,(f_time-c_time)*1000);
    if(r != -1) break; // success or timeout
    // Checking for operation interrupted by signal
    if(errno != EINTR) break;
    time_t n_time = time(NULL);
    // Protection against time jumping backward
    if(((int)(n_time - c_time)) < 0) f_time -= (c_time - n_time);
    c_time = n_time;
    // If over time, make one more try with 0 timeout
    if(((int)(f_time - c_time)) < 0) c_time = f_time;
  }
  events = fd.revents;
  return r;
}

int PayloadTCPSocket::connect_socket(const char* hostname,int port) 
{
  std::string port_str = tostring(port);
#ifndef USE_REMOTE_HOSTNAME_RESOLVER
  struct addrinfo hint;
  memset(&hint, 0, sizeof(hint));
  hint.ai_family = AF_UNSPEC;
  hint.ai_socktype = SOCK_STREAM;
  hint.ai_protocol = IPPROTO_TCP;
  struct addrinfo *info = NULL;
  int ret = getaddrinfo(hostname, port_str.c_str(), &hint, &info);
  if ((ret != 0) || (!info)) {
    std::string err_str = gai_strerror(ret); 
    error_ = IString("Failed to resolve %s (%s)", hostname, err_str).str();
    logger.msg(VERBOSE, "%s", error_);
    return -1;
  }
  int s = -1;
  for(struct addrinfo *info_ = info;info_;info_=info_->ai_next) {
    int family = info_->ai_family;
    socklen_t slen = info_->ai_addrlen;
    struct sockaddr* saddr = info_->ai_addr;
#else
  HostnameResolver* hr = HostnameResolver::Acquire();
  std::list<HostnameResolver::SockAddr> info;
  int ret = hr->hr_resolve(hostname, port_str, false, info);
  HostnameResolver::Release(hr);
  if ((ret != 0) || (info.empty())) {
    std::string err_str = gai_strerror(ret); 
    error_ = IString("Failed to resolve %s (%s)", hostname, err_str).str();
    logger.msg(VERBOSE, "%s", error_);
    return -1;
  }
  int s = -1;
  for(std::list<HostnameResolver::SockAddr>::iterator info_ = info.begin(); info_ != info.end(); ++info_) {
    int family = info_->Family();
    socklen_t slen = info_->Length();
    struct sockaddr const* saddr = info_->Addr();
#endif
    logger.msg(VERBOSE,"Trying to connect %s(%s):%d",
                     hostname,family==AF_INET6?"IPv6":"IPv4",port);
    s = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
    if(s == -1) {
      error_ = IString("Failed to create socket for connecting to %s(%s):%d - %s",
                        hostname,family==AF_INET6?"IPv6":"IPv4",port,
                        Arc::StrError(errno)).str();
      logger.msg(VERBOSE, "%s", error_);
      continue;
    }
    // In *NIX we can use non-blocking socket because poll() will 
    // be used for waiting.
    int s_flags = ::fcntl(s, F_GETFL, 0);
    if(s_flags != -1) {
      ::fcntl(s, F_SETFL, s_flags | O_NONBLOCK);
    } else {
      logger.msg(VERBOSE, "Failed to get TCP socket options for connection"
                        " to %s(%s):%d - timeout won't work - %s",
                        hostname,family==AF_INET6?"IPv6":"IPv4",port,
                        Arc::StrError(errno));
    }
    if(::connect(s, saddr, slen) == -1) {
      if(errno != EINPROGRESS) {
        error_ = IString("Failed to connect to %s(%s):%i - %s",
                        hostname,family==AF_INET6?"IPv6":"IPv4",port,
                        Arc::StrError(errno)).str();
        logger.msg(VERBOSE, "%s", error_);
        close(s); s = -1;
        continue;
      }
      unsigned int events = POLLOUT | POLLPRI;
      int pres = spoll(s,timeout_,events);
      if(pres == 0) {
        error_ = IString("Timeout connecting to %s(%s):%i - %i s",
                        hostname,family==AF_INET6?"IPv6":"IPv4",port,
                        timeout_).str();
        logger.msg(VERBOSE, "%s", error_);
        close(s); s = -1;
        continue;
      }
      if(pres != 1) {
        error_ = IString("Failed while waiting for connection to %s(%s):%i - %s",
                        hostname,family==AF_INET6?"IPv6":"IPv4",port,
                        Arc::StrError(errno)).str();
        logger.msg(VERBOSE, "%s", error_);
        close(s); s = -1;
        continue;
      }
      // man connect says one has to check SO_ERROR, but poll() returns
      // POLLERR and POLLHUP so we can use them directly. 
      if(events & (POLLERR | POLLHUP)) {
        error_ = IString("Failed to connect to %s(%s):%i",
                        hostname,family==AF_INET6?"IPv6":"IPv4",port).str();
        logger.msg(VERBOSE, "%s", error_);
        close(s); s = -1;
        continue;
      }
    }
    break;
  };
#ifndef USE_REMOTE_HOSTNAME_RESOLVER
  freeaddrinfo(info);
#endif
  if(s != -1) error_ = "";
  return s;
}

PayloadTCPSocket::PayloadTCPSocket(const char* hostname,
                                   int port,
                                   int timeout,
                                   Logger& logger) :
  logger(logger)
{
  timeout_=timeout;
  handle_=connect_socket(hostname,port);
  acquired_=true;
}

PayloadTCPSocket::PayloadTCPSocket(const std::string& endpoint, int timeout,
				                   Logger& logger) :
  logger(logger)
{
  handle_ = -1;
  acquired_=false;
  std::string hostname = endpoint;
  std::string::size_type p = hostname.find(':');
  if(p == std::string::npos) return;
  int port = atoi(hostname.c_str()+p+1);
  hostname.resize(p);
  timeout_=timeout;
  handle_=connect_socket(hostname.c_str(),port);
  acquired_=true;
}

PayloadTCPSocket::~PayloadTCPSocket(void) {
  if(acquired_ && (handle_ != -1)) { shutdown(handle_,2); close(handle_); };
}

bool PayloadTCPSocket::Get(char* buf,int& size) {
  ssize_t l = size;
  size=0;
  if(handle_ == -1) return false;
  int flags = 0;
  unsigned int events = POLLIN | POLLPRI | POLLERR;
  if(spoll(handle_,timeout_,events) != 1) return false;
  if(!(events & (POLLIN | POLLPRI))) return false; // Probably never happens
  if((events & POLLPRI) && !(events & POLLIN)) {
    logger.msg(ERROR, "Received message out-of-band (not critical, ERROR level is just for debugging purposes)");
    flags = MSG_OOB;
  }
  l=::recv(handle_,buf,l,flags);
  if(flags & MSG_OOB) { size = 0; return true; }
  if(l == -1) return false;
  size=l;
  if(l == 0) return false;
  return true;
}

bool PayloadTCPSocket::Get(std::string& buf) {
  char tbuf[1024];
  int l = sizeof(tbuf);
  bool result = Get(tbuf,l);
  buf.assign(tbuf,l);
  return result;
}

bool PayloadTCPSocket::Put(const char* buf,Size_t size) {
  ssize_t l;
  if(handle_ == -1) return false;
  time_t start = time(NULL);
  for(;size;) {
    unsigned int events = POLLOUT | POLLERR;
    int to = timeout_-(unsigned int)(time(NULL)-start);
    if(to < 0) to = 0;
    if(spoll(handle_,to,events) != 1) return false;
    if(!(events & POLLOUT)) return false;
    l=::send(handle_, buf, size, 0);
    if(l == -1) return false;
    buf+=l; size-=l;
  };  
  return true;
}

void PayloadTCPSocket::NoDelay(bool val) {
  if(handle_ == -1) return;
  int flag = val?1:0;

 ::setsockopt(handle_,IPPROTO_TCP,TCP_NODELAY,&flag,sizeof(flag));

}

} // namespace Arc
