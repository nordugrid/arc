#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <string>

#include <cstdlib>
#include <cstring>
// NOTE: On Solaris errno is not working properly if cerrno is included first
#include <cerrno>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <poll.h>

typedef struct {
  unsigned int size;
  unsigned int cmd;
} header_t;

#define CMD_SETUID (1)
// int uid
// int gid

#define CMD_MKDIR  (2)
// mode_t mode
// string dirname

bool sread(int s,void* buf,size_t size) {
  while(size) {
    ssize_t l = ::read(s,buf,size);
    if(l == -1) {
      if((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
        struct pollfd p[1];
        p[0].fd = s;
        p[0].events = POLLIN;
        p[0].revents = 0;
        if(poll(p,1,-1) >= 0) continue;
      };
      return false;
    };
    if(l == 0) return false;
    size-=l;
    buf = (void*)(((char*)buf)+l);
  };
  return true;
}

ssize_t swrite(int s,const void* buf,size_t size) {
  while(size) {
    ssize_t l = ::write(s,buf,size);
    if(l == -1) {
      if((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
        struct pollfd p[1];
        p[0].fd = s;
        p[0].events = POLLOUT;
        p[0].revents = 0;
        if(poll(p,1,-1) >= 0) continue;
      };
      return false;
    };
    size-=l;
    buf = (void*)(((char*)buf)+l);
  };
  return true;
}

bool sread_string(int s,std::string& str,int maxsize) {
  int ssize;
  if(!sread(s,&ssize,sizeof(ssize))) return false;
  maxsize -= sizeof(ssize);
  if(ssize > maxsize) return false;
  str.assign(ssize,' ');
  // Not nice but saves memory copying
  if(!sread(s,(void*)(str.c_str()),ssize)) return false;
  return true;
}

int main(int argc,char* argv[]) {
  uid_t initial_uid = getuid();
  gid_t initial_gid = getgid();
  if(argc != 3) return -1;
  char* e;
  e = argv[1];
  int sin = strtoul(argv[1],&e,10);
  if((e == argv[1]) || (*e != 0)) return -1;
  e = argv[2];
  int sout = strtoul(argv[2],&e,10);
  if((e == argv[2]) || (*e != 0)) return -1;
  while(true) {
    ssize_t l;
    header_t header;
    if(!sread(sin,&header,sizeof(header))) break;
    switch(header.cmd) {
      case CMD_SETUID: {
        int uid = 0;
        int gid = 0;
        int res = 0;
        if(header.size != (sizeof(uid)+sizeof(gid))) return -1;
        if(!sread(sin,&uid,sizeof(uid))) return -1;
        if(!sread(sin,&gid,sizeof(gid))) return -1;
        seteuid(initial_uid);
        setegid(initial_gid);
        if((gid != 0) && (gid != initial_gid)) {
          res = setegid(gid);
        };
        if((res == 0) && (uid != 0) && (uid != initial_uid)) {
          res = seteuid(uid);
        };
        int err = errno;
        header.size = sizeof(res) + sizeof(err);
        header.cmd = CMD_SETUID;
        if(!swrite(sout,&header,sizeof(header))) return -1;
        if(!swrite(sout,&res,sizeof(res))) return -1;
        if(!swrite(sout,&err,sizeof(err))) return -1;
      }; break;
 
      case CMD_MKDIR: {
        mode_t mode;
        std::string dirname;
        if(!sread(sin,&mode,sizeof(mode))) return -1;
        header.size -= sizeof(mode);
        if(!sread_string(sin,dirname,header.size)) return -1;
        int res = ::mkdir(dirname.c_str(),mode);
        int err = errno;
        header.size = sizeof(res) + sizeof(err);
        header.cmd = CMD_MKDIR;
        if(!swrite(sout,&header,sizeof(header))) return -1;
        if(!swrite(sout,&res,sizeof(res))) return -1;
        if(!swrite(sout,&err,sizeof(err))) return -1;
      }; break;
    };
  };
  return 0;
}

