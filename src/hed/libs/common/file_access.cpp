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

static bool swrite_result(int s,int cmd,int res,int err,const void* add1,int addsize1,const void* add2,int addsize2) {
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

static bool swrite_result(int s,int cmd,int res,int err,const std::string& str) {
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

static bool cleandir(const std::string& path,int& err) {
  errno = 0;
  DIR* dir = opendir(path.c_str());
  if(!dir) { err = errno; return false; };
  for(;;) {
    struct dirent* d = ::readdir(dir);
    if(!d) break;
    if(strcmp(d->d_name,".") == 0) continue;
    if(strcmp(d->d_name,"..") == 0) continue;
    std::string npath = path + "/" + d->d_name;
    errno = 0;
    if(::remove(npath.c_str()) == 0) continue;
    if(errno != ENOTEMPTY) { err = errno; closedir(dir); return false; };
    if(!cleandir(npath,err)) { closedir(dir); return false; };
    errno = 0;
    if(::remove(npath.c_str()) != 0) { err = errno; closedir(dir); return false; };
  };
  closedir(dir);
  err = 0;
  return true;
}

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
    header_t header;
    sread_start = true;
    if(!sread(sin,&header,sizeof(header))) break;
    switch(header.cmd) {
      case CMD_PING: {
        if(header.size != 0) return -1;
        if(!swrite(sout,&header,sizeof(header))) return -1;
      }; break;

      case CMD_SETUID: {
        int uid = 0;
        int gid = 0;
        int res = -1;
        if(header.size != (sizeof(uid)+sizeof(gid))) return -1;
        if(!sread(sin,&uid,sizeof(uid))) return -1;
        if(!sread(sin,&gid,sizeof(gid))) return -1;
        errno = 0;
        res = 0;
        seteuid(initial_uid);
        setegid(initial_gid);
        if((gid != 0) && (gid != initial_gid)) {
          errno = 0;
          res = setegid(gid);
        };
        if((res == 0) && (uid != 0) && (uid != initial_uid)) {
          errno = 0;
          res = seteuid(uid);
        };
        if(!swrite_result(sout,header.cmd,res,errno)) return -1;
      }; break;
 
      case CMD_MKDIR:
      case CMD_MKDIRP: {
        mode_t mode;
        std::string dirname;
        if(!sread(sin,&mode,sizeof(mode))) return -1;
        header.size -= sizeof(mode);
        if(!sread_string(sin,dirname,header.size)) return -1;
        if(header.size) return -1;
        errno = 0;
        int res = ::mkdir(dirname.c_str(),mode);
        if((res != 0) && (header.cmd == CMD_MKDIRP)) {
          // resursively up
          std::string::size_type p = dirname.length();
          if(p > 0) {
            while(errno == ENOENT) {
              p = dirname.rfind('/',p-1); 
              if(p == std::string::npos) break;
              if(p == 0) break;
              errno = 0;
              res = ::mkdir(dirname.substr(0,p).c_str(),mode);
              if(res == 0) break;
            };
            if((res == 0) || (errno == EEXIST)) {
              // resursively down
              while(p < dirname.length()) {
                p = dirname.find('/',p+1);
                if(p == std::string::npos) p = dirname.length();
                errno = 0;
                res = ::mkdir(dirname.substr(0,p).c_str(),mode);
                if(res != 0) break;
              };
            };
          };
        };
        if(!swrite_result(sout,header.cmd,res,errno)) return -1;
      }; break;

      case CMD_HARDLINK:
      case CMD_SOFTLINK: {
        std::string oldpath;
        std::string newpath;
        if(!sread_string(sin,oldpath,header.size)) return -1;
        if(!sread_string(sin,newpath,header.size)) return -1;
        if(header.size) return -1;
        int res = -1;
        errno = 0;
        if(header.cmd == CMD_HARDLINK) {
          res = ::link(oldpath.c_str(),newpath.c_str());
        } else {
          res = ::symlink(oldpath.c_str(),newpath.c_str());
        };
        if(!swrite_result(sout,header.cmd,res,errno)) return -1;
      }; break;

      case CMD_COPY: {
        mode_t mode;
        std::string oldpath;
        std::string newpath;
        if(!sread(sin,&mode,sizeof(mode))) return -1;
        header.size -= sizeof(mode);
        if(!sread_string(sin,oldpath,header.size)) return -1;
        if(!sread_string(sin,newpath,header.size)) return -1;
        if(header.size) return -1;
        int res = 0;
        int err = 0;
        errno = 0;
        int h_src = ::open(oldpath.c_str(),O_RDONLY,0);
        if(h_src != -1) {
          int h_dst = ::open(newpath.c_str(),O_WRONLY|O_CREAT|O_TRUNC,mode);
          if(h_dst != -1) {
            for(;;) {
              ssize_t l = read(h_src,filebuf,sizeof(filebuf));
              if(l <= 0) { err = errno; res = l; break; };
              for(size_t p = 0;p<l;) {
                ssize_t ll = write(h_dst,filebuf+p,l-p);
                if(ll < 0) { err = errno; res = ll; break; };
                p+=ll;
              };
            };
            ::close(h_dst);
          } else  { err = errno; res = -1; };
          ::close(h_src);
        } else { err = errno; res = -1; };
        if(!swrite_result(sout,header.cmd,res,err)) return -1;
      }; break;

      case CMD_STAT:
      case CMD_LSTAT: {
        std::string path;
        if(!sread_string(sin,path,header.size)) return -1;
        if(header.size) return -1;
        int res = 0;
        struct stat st;
        errno = 0;
        if(header.cmd == CMD_STAT) {
          res = ::stat(path.c_str(),&st);
        } else {
          res = ::lstat(path.c_str(),&st);
        };
        if(!swrite_result(sout,header.cmd,res,errno,&st,sizeof(st))) return -1;
      }; break;

      case CMD_CHMOD: {
        mode_t mode;
        std::string path;
        if(!sread(sin,&mode,sizeof(mode))) return -1;
        header.size -= sizeof(mode);
        if(!sread_string(sin,path,header.size)) return -1;
        if(header.size) return -1;
        errno = 0;
        int res = ::chmod(path.c_str(),mode);
        if(!swrite_result(sout,header.cmd,res,errno)) return -1;
      }; break;

      case CMD_REMOVE:
      case CMD_UNLINK:
      case CMD_RMDIR:
      case CMD_RMDIRR: {
        std::string path;
        if(!sread_string(sin,path,header.size)) return -1;
        if(header.size) return -1;
        int res = 0;
        int err = 0;
        errno = 0;
        if(header.cmd == CMD_REMOVE) {
          res = ::remove(path.c_str());
          err = errno;
        } else if(header.cmd == CMD_UNLINK) {
          res = ::unlink(path.c_str());
          err = errno;
        } else if(header.cmd == CMD_RMDIR) {
          res = ::rmdir(path.c_str());
          err = errno;
        } else {
          res = ::rmdir(path.c_str());
          err = errno;
          if(err == ENOTEMPTY) {
            if(cleandir(path,err)) {
              errno = 0;
              res = ::rmdir(path.c_str());
              err = errno;
            };
          };
        };
        if(!swrite_result(sout,header.cmd,res,err)) return -1;
      }; break;

      case CMD_OPENDIR: {
        std::string path;
        if(!sread_string(sin,path,header.size)) return -1;
        if(header.size) return -1;
        int res = 0;
        if(curdir) ::closedir(curdir);
        errno = 0;
        curdir = ::opendir(path.c_str());
        if(!curdir) res = -1;
        if(!swrite_result(sout,header.cmd,res,errno)) return -1;
      }; break;

      case CMD_CLOSEDIR: {
        if(header.size) return -1;
        int res = 0;
        errno = 0;
        if(curdir) res = ::closedir(curdir);
        curdir = NULL;
        if(!swrite_result(sout,header.cmd,res,errno)) return -1;
      }; break;

      case CMD_READDIR: {
        if(header.size) return -1;
        int res = 0;
        if(curdir) {
          std::string name;
          errno = 0;
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
        errno = 0;
        int res = (curfile = ::open(path.c_str(),flags,mode));
        if(!swrite_result(sout,header.cmd,res,errno)) return -1;
      }; break;

      case CMD_TEMPFILE: {
        mode_t mode;
        std::string path;
        if(!sread(sin,&mode,sizeof(mode))) return -1;
        header.size -= sizeof(mode);
        if(!sread_string(sin,path,header.size)) return -1;
        if(header.size) return -1;
        if(curfile != -1) ::close(curfile);
        errno = 0;
        int res = (curfile = mkstemp((char*)(path.c_str())));
        if(res != -1) ::chmod(path.c_str(),mode);
        int l = path.length();
        if(!swrite_result(sout,header.cmd,res,errno,&l,sizeof(l),path.c_str(),l)) return -1;
      }; break;

      case CMD_CLOSEFILE: {
        if(header.size) return -1;
        int res = 0;
        errno = 0;
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
        errno = 0;
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
        errno = 0;
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
        errno = 0;
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
        ssize_t l = -1;
        errno = 0;
        l = ::pread(curfile,filebuf,size,offset);
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
        ssize_t l = -1;
        errno = 0;
        l = ::pwrite(curfile,filebuf,size,offset);
        int res = l;
        if(!swrite_result(sout,header.cmd,res,errno)) return -1;
      }; break;

      case CMD_FSTAT: {
        if(header.size) return -1;
        int res = 0;
        struct stat st;
        errno = 0;
        res = ::fstat(curfile,&st);
        if(!swrite_result(sout,header.cmd,res,errno,&st,sizeof(st))) return -1;
      }; break;

      case CMD_READLINK: {
        std::string path;
        if(!sread_string(sin,path,header.size)) return -1;
        if(header.size) return -1;
        int res = 0;
        errno = 0;
        int l = readlink(path.c_str(), filebuf, sizeof(filebuf));
        res = l;
        if(l < 0) l = 0;
        if(!swrite_result(sout,header.cmd,res,errno,&l,sizeof(l),filebuf,l)) return -1;
      }; break;

      case CMD_FTRUNCATE: {
        off_t length;
        if(!sread(sin,&length,sizeof(length))) return -1;
        header.size -= sizeof(length);
        if(header.size) return -1;
        int res = 0;
        errno = 0;
        res = ::ftruncate(curfile,length);
        if(!swrite_result(sout,header.cmd,res,errno)) return -1;
      }; break;

      case CMD_FALLOCATE: {
        off_t length;
        if(!sread(sin,&length,sizeof(length))) return -1;
        header.size -= sizeof(length);
        if(header.size) return -1;
        int res = 0;
        int err = 0;
        errno = 0;
#ifdef HAVE_POSIX_FALLOCATE
        res = posix_fallocate(curfile,0,length);
        err = res;
        length = lseek(curfile,0,SEEK_END);
#else
        off_t olength = lseek(curfile, 0, SEEK_END);
        if(olength >= 0) {
          if(olength < length) {
            memset(filebuf, 0xFF, sizeof(filebuf));
            while(olength < length) {
              size_t l = sizeof(filebuf);
              if (l > (length - olength)) l = length - olength;
              if (write(curfile, filebuf, l) == -1) {
                break;
              };
              olength = lseek(curfile, 0, SEEK_END);
            };
          };
        };
        err = errno; res = err;
        length = olength;
#endif
        if(!swrite_result(sout,header.cmd,res,err,&length,sizeof(length))) return -1;
      }; break;

      case CMD_RENAME: {
        std::string oldpath;
        std::string newpath;
        if(!sread_string(sin,oldpath,header.size)) return -1;
        if(!sread_string(sin,newpath,header.size)) return -1;
        if(header.size) return -1;
        errno = 0;
        int res = rename(oldpath.c_str(), newpath.c_str());
        if(!swrite_result(sout,header.cmd,res,errno)) return -1;
      }; break;

      default: return -1;
    };
  };
  return 0;
}

