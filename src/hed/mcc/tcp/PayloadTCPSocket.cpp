#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef WIN32
#include <arc/win32.h>
#else // UNIX
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/poll.h>
#endif

#include <arc/StringConv.h>
#include "PayloadTCPSocket.h"

namespace Arc {

int PayloadTCPSocket::connect_socket(const char* hostname,int port) 
{
  struct addrinfo hint;
  memset(&hint, 0, sizeof(hint));
  hint.ai_family = AF_UNSPEC;
  hint.ai_socktype = SOCK_STREAM;
  hint.ai_protocol = IPPROTO_TCP;
  std::string port_str = Arc::tostring(port);
  struct addrinfo *info = NULL;
  int ret = getaddrinfo(hostname, port_str.c_str(), &hint, &info);
  if (ret != 0) {
    std::string err_str = gai_strerror(ret); 
    logger.msg(WARNING, "Failed to resolve %s (%s)", hostname, err_str);
	return -1;
  }
  int s = -1;
  for(struct addrinfo *info_ = info;info_;info_=info_->ai_next) {
    logger.msg(DEBUG,"Trying to connect %s(%s):%d",
                     hostname,info_->ai_family==AF_INET6?"IPv6":"IPv4",port);
    s = ::socket(info_->ai_family, info_->ai_socktype, info_->ai_protocol);
    if(s == -1) {
      // TODO: print error description
      logger.msg(DEBUG, "Failed to create socket to %s(%s):%d",
                        hostname,info_->ai_family==AF_INET6?"IPv6":"IPv4",port);
      continue;
    }
    if(::connect(s, info_->ai_addr, info_->ai_addrlen) == -1) {
      logger.msg(DEBUG, "Failed to connect to %s(%s):%i",
                        hostname,info_->ai_family==AF_INET6?"IPv6":"IPv4",port);
      close(s); s = -1;
      continue;
    };
    break;
  };
  if(s == -1) {
    logger.msg(DEBUG, "Failed to establish connection to %s:%i", hostname, port);
  };
  freeaddrinfo(info);
  return s;
}

PayloadTCPSocket::PayloadTCPSocket(const char* hostname,
				   int port,
				   Logger& logger) :
  logger(logger)
{
  handle_=connect_socket(hostname,port);
  acquired_=true;
}

PayloadTCPSocket::PayloadTCPSocket(const std::string endpoint,
				   Logger& logger) :
  logger(logger)
{
  std::string hostname = endpoint;
  std::string::size_type p = hostname.find(':');
  if(p == std::string::npos) return;
  int port = atoi(hostname.c_str()+p+1);
  hostname.resize(p);
  handle_=connect_socket(hostname.c_str(),port);
  acquired_=true;
}

PayloadTCPSocket::~PayloadTCPSocket(void) {
  if(acquired_) { shutdown(handle_,2); close(handle_); };
}

bool PayloadTCPSocket::Get(char* buf,int& size) {
  if(handle_ == -1) return false;
  ssize_t l = size;
  size=0;
  if(seekable_) { // check for EOF
    struct stat st;
    if(fstat(handle_,&st) != 0) return false;
    off_t o = lseek(handle_,0,SEEK_CUR);
    if(o == (off_t)(-1)) return false;
    o++;
    if(o >= st.st_size) return false;
  };
#ifndef WIN32
  struct pollfd fd;
  fd.fd=handle_; fd.events=POLLIN | POLLPRI | POLLERR; fd.revents=0;
  if(poll(&fd,1,timeout_*1000) != 1) return false;
  if(!(fd.revents & (POLLIN | POLLPRI))) return false;
#endif
  l=::recv(handle_,buf,l,0);
  if(l == -1) return false;
  size=l;
#ifndef WIN32
  if((l == 0) && (fd.revents && POLLERR)) return false;
#else
  if(l == 0) return false;
#endif
  return true;
}

bool PayloadTCPSocket::Put(const char* buf,int size) {
  ssize_t l;
  if(handle_ == -1) return false;
  time_t start = time(NULL);
  for(;size;) {
#ifndef WIN32
    struct pollfd fd;
    fd.fd=handle_; fd.events=POLLOUT | POLLERR; fd.revents=0;
    int to = timeout_-(unsigned int)(time(NULL)-start);
    if(to < 0) to=0;
    if(poll(&fd,1,to*1000) != 1) return false;
    if(!(fd.revents & POLLOUT)) return false;
#endif
    l=::send(handle_, buf, size, 0);
    if(l == -1) return false;
    buf+=l; size-=l;
#ifdef WIN32
    int to = timeout_-(unsigned int)(time(NULL)-start);
    if(to < 0) return false;
#endif
  };  
  return true;
}

} // namespace Arc
