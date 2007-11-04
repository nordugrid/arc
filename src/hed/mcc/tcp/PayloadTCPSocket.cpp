#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef WIN32
#define NOGDI
#include <winsock2.h>
typedef int socklen_t;
#define ErrNo WSAGetLastError()
#else
#include <sys/socket.h>
#include <netdb.h>
#endif
#include <unistd.h>

#include "PayloadTCPSocket.h"

namespace Arc {

int PayloadTCPSocket::connect_socket(const char* hostname,int port) {
  struct hostent* host = NULL;
  struct hostent  hostbuf;
  int    errcode = 0;
#ifndef HAVE_GETHOSTBYNAME_R
  /* According to the developer manual the Darwin's version of gethostbyname
     is thread-safe */
  host = gethostbyname(hostname);
  if (host == NULL) {
#else
  #if defined(_AIX)
  struct hostent_data buf[BUFSIZ];
  if((errcode=gethostbyname_r(hostname,
                                  (host=&hostbuf),buf)) != 0) {
  #else
  char   buf[BUFSIZ];
  if((gethostbyname_r(hostname,&hostbuf,buf,sizeof(buf),
                                        &host,&errcode) != 0) ||
     (host == NULL)) {
  #endif
#endif
    logger.msg(WARNING, "Failed to resolve %s", hostname);
    return -1;
  };
  if( (host->h_length < (int)sizeof(struct in_addr)) ||
      (host->h_addr_list[0] == NULL) ) {
    logger.msg(WARNING, "Failed to resolve %s", hostname);
    return -1;
  };
  struct sockaddr_in addr;
  memset(&addr,0,sizeof(addr));
  addr.sin_family=AF_INET;
  addr.sin_port=htons(port);
  memcpy(&addr.sin_addr,host->h_addr_list[0],sizeof(struct in_addr));
  int s = ::socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
  if(s==-1) return -1;
  if(::connect(s,(struct sockaddr *)&addr,sizeof(addr))==-1) {
    logger.msg(WARNING, "Failed to connect to %s:%i", hostname, port);
    close(s); return -1;
  };
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

} // namespace Arc
