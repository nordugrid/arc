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
#include <unistd.h>
#include <poll.h>
#include <dirent.h>
#include <fcntl.h>

#include "file_access.h"

typedef struct {
  unsigned int size;
  unsigned int cmd;
} header_t;

static bool sread(int s,void* buf,size_t size) {
  while(size) {
    //std::cerr<<"sread: size="<<size<<std::endl;
    ssize_t l = ::read(s,buf,size);
    //std::cerr<<"sread: l="<<l<<std::endl;
    if(l < 0) {
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

static ssize_t swrite(int s,const void* buf,size_t size) {
  while(size) {
    //std::cerr<<"swrite: size="<<size<<std::endl;
    ssize_t l = ::write(s,buf,size);
    //std::cerr<<"swrite: l="<<l<<std::endl;
    if(l < 0) {
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

static bool sread_string(int s,std::string& str,unsigned int& maxsize) {
  unsigned int ssize;
  //std::cerr<<"sread_string: maxsize="<<maxsize<<std::endl;
  if(sizeof(ssize) > maxsize) return false;
  if(!sread(s,&ssize,sizeof(ssize))) return false;
  //std::cerr<<"sread_string: ssize="<<ssize<<std::endl;
  maxsize -= sizeof(ssize);
  if(ssize > maxsize) return false;
  str.assign(ssize,' ');
  // Not nice but saves memory copying
  if(!sread(s,(void*)(str.c_str()),ssize)) return false;
  //std::cerr<<"sread_string: str="<<str<<std::endl;
  maxsize -= ssize;
  return true;
}

static bool sread_buf(int s,void* buf,unsigned int& bufsize,unsigned int& maxsize) {
  char dummy[1024];
  unsigned int size;
  if(sizeof(size) > maxsize) return false;
  if(!sread(s,&size,sizeof(size))) return false;
  maxsize -= sizeof(size);
  if(size > maxsize) return false;
  if(size <= bufsize) {
    if(!sread(s,buf,size)) return false;
    bufsize = size;
    maxsize -= size;
  } else {
    if(!sread(s,buf,bufsize)) return false;
    maxsize -= bufsize;
    // skip rest
    size -= bufsize;
    while(size > sizeof(dummy)) {
      if(!sread(s,dummy,sizeof(dummy))) return false;
      size -= sizeof(dummy);
      maxsize -= sizeof(dummy);
    };
    if(!sread(s,dummy,size)) return false;
    maxsize -= size;
  };
  return true;
}

static bool swrite_result(int s,int cmd,int res,int err) {
  header_t header;
  header.cmd = cmd;
  header.size = sizeof(res) + sizeof(err);
  if(!swrite(s,&header,sizeof(header))) return -1;
  if(!swrite(s,&res,sizeof(res))) return -1;
  if(!swrite(s,&err,sizeof(err))) return -1;
  return true;
}

static bool swrite_result(int s,int cmd,int res,int err,void* add,int addsize) {
  header_t header;
  header.cmd = cmd;
  header.size = sizeof(res) + sizeof(err) + addsize;
  if(!swrite(s,&header,sizeof(header))) return -1;
  if(!swrite(s,&res,sizeof(res))) return -1;
  if(!swrite(s,&err,sizeof(err))) return -1;
  if(!swrite(s,add,addsize)) return -1;
  return true;
}

static bool swrite_result(int s,int cmd,int res,int err,void* add1,int addsize1,void* add2,int addsize2) {
  header_t header;
  header.cmd = cmd;
  header.size = sizeof(res) + sizeof(err) + addsize1 + addsize2;
  if(!swrite(s,&header,sizeof(header))) return -1;
  if(!swrite(s,&res,sizeof(res))) return -1;
  if(!swrite(s,&err,sizeof(err))) return -1;
  if(!swrite(s,add1,addsize1)) return -1;
  if(!swrite(s,add2,addsize2)) return -1;
  return true;
}

static bool swrite_result(int s,int cmd,int res,int err,const std::string str) {
  unsigned int l = str.length();
  header_t header;
  header.cmd = cmd;
  header.size = sizeof(res) + sizeof(err) + sizeof(l) + str.length();
  if(!swrite(s,&header,sizeof(header))) return -1;
  if(!swrite(s,&res,sizeof(res))) return -1;
  if(!swrite(s,&err,sizeof(err))) return -1;
  if(!swrite(s,&l,sizeof(l))) return -1;
  if(!swrite(s,str.c_str(),l)) return -1;
  return true;
}

static char filebuf[1024*1024*10];

int main(int argc,char* argv[]) {
  uid_t initial_uid = getuid();
  gid_t initial_gid = getgid();
  DIR* curdir = NULL;
  int curfile = -1;

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
      case CMD_PING: {
        if(header.size != 0) return -1;
        if(!swrite(sout,&header,sizeof(header))) return -1;
      }; break;

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
        if(!swrite_result(sout,header.cmd,res,errno)) return -1;
      }; break;
 
      case CMD_MKDIR: {
        mode_t mode;
        std::string dirname;
        if(!sread(sin,&mode,sizeof(mode))) return -1;
        header.size -= sizeof(mode);
        if(!sread_string(sin,dirname,header.size)) return -1;
        if(header.size) return -1;
        int res = ::mkdir(dirname.c_str(),mode);
        if(!swrite_result(sout,header.cmd,res,errno)) return -1;
      }; break;

      case CMD_HARDLINK:
      case CMD_SOFTLINK: {
        std::string oldpath;
        std::string newpath;
        if(!sread_string(sin,oldpath,header.size)) return -1;
        if(!sread_string(sin,newpath,header.size)) return -1;
        if(header.size) return -1;
        int res = 0;
        if(header.cmd == CMD_HARDLINK) {
          res = ::link(oldpath.c_str(),newpath.c_str());
        } else {
          res = ::symlink(oldpath.c_str(),newpath.c_str());
        };
        if(!swrite_result(sout,header.cmd,res,errno)) return -1;
      }; break;

      case CMD_STAT:
      case CMD_LSTAT: {
        std::string path;
        if(!sread_string(sin,path,header.size)) return -1;
        if(header.size) return -1;
        int res = 0;
        struct stat st;
        if(header.cmd == CMD_STAT) {
          res = ::stat(path.c_str(),&st);
        } else {
          res = ::lstat(path.c_str(),&st);
        };
        if(!swrite_result(sout,header.cmd,res,errno,&st,sizeof(st))) return -1;
      }; break;

      case CMD_REMOVE:
      case CMD_UNLINK:
      case CMD_RMDIR: {
        std::string path;
        if(!sread_string(sin,path,header.size)) return -1;
        if(header.size) return -1;
        int res = 0;
        if(header.cmd == CMD_REMOVE) {
          res = ::remove(path.c_str());
        } else if(header.cmd == CMD_UNLINK) {
          res = ::unlink(path.c_str());
        } else {
          res = ::rmdir(path.c_str());
        };
        if(!swrite_result(sout,header.cmd,res,errno)) return -1;
      }; break;

      case CMD_OPENDIR: {
        std::string path;
        if(!sread_string(sin,path,header.size)) return -1;
        if(header.size) return -1;
        int res = 0;
        if(curdir) ::closedir(curdir);
        curdir = ::opendir(path.c_str());
        if(!curdir) res = -1;
        if(!swrite_result(sout,header.cmd,res,errno)) return -1;
      }; break;

      case CMD_CLOSEDIR: {
        if(header.size) return -1;
        int res = 0;
        if(curdir) res = ::closedir(curdir);
        curdir = NULL;
        if(!swrite_result(sout,header.cmd,res,errno)) return -1;
      }; break;

      case CMD_READDIR: {
        if(header.size) return -1;
        int res = 0;
        if(curdir) {
          std::string name;
          struct dirent* d = ::readdir(curdir);
          if(d) {
            name = d->d_name;
          } else {
            res = -1;
          };
          if(!swrite_result(sout,header.cmd,res,errno,name)) return -1;
        } else {
          if(!swrite_result(sout,header.cmd,res,EBADF)) return -1;
        };
      }; break;

      case CMD_OPENFILE: {
        int flags;
        mode_t mode;
        std::string path;
        if(!sread(sin,&flags,sizeof(flags))) return -1;
        header.size -= sizeof(flags);
        if(!sread(sin,&mode,sizeof(mode))) return -1;
        header.size -= sizeof(mode);
        if(!sread_string(sin,path,header.size)) return -1;
        if(header.size) return -1;
        if(curfile != -1) ::close(curfile);
        int res = (curfile = ::open(path.c_str(),flags,mode));
        if(!swrite_result(sout,header.cmd,res,errno)) return -1;
      }; break;

      case CMD_CLOSEFILE: {
        if(header.size) return -1;
        int res = 0;
        res = ::close(curfile);
        curfile = -1;
        if(!swrite_result(sout,header.cmd,res,errno)) return -1;
      }; break;

      case CMD_SEEKFILE: {
        off_t offset;
        int whence;
        if(!sread(sin,&offset,sizeof(offset))) return -1;
        header.size -= sizeof(offset);
        if(!sread(sin,&whence,sizeof(whence))) return -1;
        header.size -= sizeof(whence);
        if(header.size) return -1;
        int res = (offset = ::lseek(curfile,offset,whence));
        if(!swrite_result(sout,header.cmd,res,errno,&offset,sizeof(offset))) return -1;
      }; break;

      case CMD_READFILE: {
        // TODO: maybe use shared memory
        size_t size;
        if(!sread(sin,&size,sizeof(size))) return -1;
        header.size -= sizeof(size);
        if(header.size) return -1;
        if(size > sizeof(filebuf)) size = sizeof(filebuf);
        ssize_t l = ::read(curfile,filebuf,size);
        int res = l;
        if(l < 0) l = 0;
        int n = l;
        if(!swrite_result(sout,header.cmd,res,errno,&n,sizeof(n),filebuf,l)) return -1;
      }; break;

      case CMD_WRITEFILE: {
        unsigned int size = sizeof(filebuf);
        if(!sread_buf(sin,filebuf,size,header.size)) return false;
        if(header.size) return -1;
        ssize_t l = ::write(curfile,filebuf,size);
        int res = l;
        if(!swrite_result(sout,header.cmd,res,errno)) return -1;
      }; break;

      case CMD_READFILEAT: {
        off_t offset;
        size_t size;
        if(!sread(sin,&size,sizeof(size))) return -1;
        header.size -= sizeof(size);
        if(!sread(sin,&offset,sizeof(offset))) return -1;
        header.size -= sizeof(offset);
        if(header.size) return -1;
        if(size > sizeof(filebuf)) size = sizeof(filebuf);
        ssize_t l = ::pread(curfile,filebuf,size,offset);
        int res = l;
        if(l < 0) l = 0;
        int n = l;
        if(!swrite_result(sout,header.cmd,res,errno,&n,sizeof(n),filebuf,l)) return -1;
      }; break;

      case CMD_WRITEFILEAT: {
        off_t offset;
        if(!sread(sin,&offset,sizeof(offset))) return -1;
        header.size -= sizeof(offset);
        unsigned int size = sizeof(filebuf);
        if(!sread_buf(sin,filebuf,size,header.size)) return false;
        if(header.size) return -1;
        ssize_t l = ::pwrite(curfile,filebuf,size,offset);
        int res = l;
        if(!swrite_result(sout,header.cmd,res,errno)) return -1;
      }; break;

      default: return -1;
    };
  };
  return 0;
}

