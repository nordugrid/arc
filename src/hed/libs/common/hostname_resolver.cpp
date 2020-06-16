#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <string>

#include <cstdlib>
#include <cstdio>
#include <cstring>
// NOTE: On Solaris errno is not working properly if cerrno is included first
#include <cerrno>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <dirent.h>
#include <fcntl.h>
#include <netdb.h>

#include "hostname_resolver.h"

typedef struct {
  unsigned int size;
  unsigned int cmd;
} header_t;

// How long it is allowed for controlling side to react
#define COMMUNICATION_TIMEOUT (10)

static bool sread_start = true;

static bool sread(int s,void* buf,size_t size) {
  while(size) {
    struct pollfd p[1];
    p[0].fd = s;
    p[0].events = POLLIN;
    p[0].revents = 0;
    int err = poll(p,1,sread_start?-1:(COMMUNICATION_TIMEOUT*1000));
    if(err == 0) return false;
    if((err == -1) && (errno != EINTR)) return false;
    if(err == 1) {
      ssize_t l = ::read(s,buf,size);
      if(l <= 0) return false;
      size-=l;
      buf = (void*)(((char*)buf)+l);
      sread_start =  false;
    };
  };
  return true;
}

static bool swrite(int s,const void* buf,size_t size) {
  while(size) {
    struct pollfd p[1];
    p[0].fd = s;
    p[0].events = POLLOUT;
    p[0].revents = 0;
    int err = poll(p,1,COMMUNICATION_TIMEOUT*1000);
    if(err == 0) return false;
    if((err == -1) && (errno != EINTR)) return false;
    if(err == 1) {
      ssize_t l = ::write(s,buf,size);
      if(l < 0) return false;
      size-=l;
      buf = (void*)(((char*)buf)+l);
    };
  };
  return true;
}

static bool sread_string(int s,std::string& str,unsigned int& maxsize) {
  unsigned int ssize;
  if(sizeof(ssize) > maxsize) return false;
  if(!sread(s,&ssize,sizeof(ssize))) return false;
  maxsize -= sizeof(ssize);
  if(ssize > maxsize) return false;
  str.assign(ssize,' ');
  // Not nice but saves memory copying
  if(!sread(s,(void*)(str.c_str()),ssize)) return false;
  maxsize -= ssize;
  return true;
}

static bool swrite_result(int s,int cmd,int res,int err,const void* add,int addsize) {
  header_t header;
  header.cmd = cmd;
  header.size = sizeof(res) + sizeof(err) + addsize;
  if(!swrite(s,&header,sizeof(header))) return -1;
  if(!swrite(s,&res,sizeof(res))) return -1;
  if(!swrite(s,&err,sizeof(err))) return -1;
  if(!swrite(s,add,addsize)) return -1;
  return true;
}

int main(int argc,char* argv[]) {
  if(argc != 3) return -1;
  char* e;
  e = argv[1];
  int sin = strtoul(argv[1],&e,10);
  if((e == argv[1]) || (*e != 0)) return -1;
  e = argv[2];
  int sout = strtoul(argv[2],&e,10);
  if((e == argv[2]) || (*e != 0)) return -1;
  while(true) {
    header_t header;
    sread_start = true;
    if(!sread(sin,&header,sizeof(header))) break;
    switch(header.cmd) {
      case CMD_PING: {
        if(header.size != 0) return -1;
        if(!swrite(sout,&header,sizeof(header))) return -1;
      }; break;

      case CMD_RESOLVE_TCP_LOCAL:
      case CMD_RESOLVE_TCP_REMOTE: {
        std::string node;
        std::string service;
        if(!sread_string(sin,node,header.size)) return -1;
        if(!sread_string(sin,service,header.size)) return -1;
        if(header.size) return -1;
        errno = 0;
        struct addrinfo hint;
        ::memset(&hint,0,sizeof(hint));
        hint.ai_socktype = SOCK_STREAM;
        hint.ai_protocol = IPPROTO_TCP;
        hint.ai_family = AF_UNSPEC;
        if (header.cmd == CMD_RESOLVE_TCP_LOCAL) hint.ai_flags = AI_PASSIVE;
        struct addrinfo* addrs = NULL;
        std::string data;
        int res = getaddrinfo(node.c_str(),service.c_str(),&hint,&addrs);
        if(res == 0) {
          for (struct addrinfo* addr = addrs; addr != NULL; addr = addr->ai_next) {
            int family = addr->ai_family;
            socklen_t length = addr->ai_addrlen;
            struct sockaddr* saddr = addr->ai_addr;
            data.append((char const*)&family,sizeof(family));
            data.append((char const*)&length,sizeof(length));
            data.append((char const*)saddr,length);
          };
          freeaddrinfo(addrs);
          errno = 0;
        };
        if(!swrite_result(sout,header.cmd,res,errno,data.c_str(),data.length())) return -1;
      }; break;
 
      default: return -1;
    };
  };
  return 0;
}

