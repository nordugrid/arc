#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>
#include <iostream>
#include <string.h>

#include <arc/Run.h>
#include <arc/ArcLocation.h>

#include "file_access.h"

#include "FileAccess.h"

namespace Arc {

  static bool sread(Run& r,void* buf,size_t size) {
    while(size) {
      int l = r.ReadStdout(-1,(char*)buf,size);
      if(l < 0) return false;
      size-=l;
      buf = (void*)(((char*)buf)+l);
    };
    return true;
  }

  static bool swrite(Run& r,const void* buf,size_t size) {
    while(size) {
      int l = r.WriteStdin(-1,(const char*)buf,size);
      if(l < 0) return false;
      size-=l;
      buf = (void*)(((char*)buf)+l);
    };
    return true;
  }

#define ABORTALL { dispose_executer(file_access_); file_access_=NULL; continue; }

#define STARTHEADER(CMD,SIZE) { \
  if(!file_access_) break; \
  if(!(file_access_->Running())) break; \
  FileAccess::header_t header; \
  header.cmd = CMD; \
  header.size = SIZE; \
  if(!swrite(*file_access_,&header,sizeof(header))) ABORTALL; \
}

#define ENDHEADER(CMD,SIZE) { \
  FileAccess::header_t header; \
  if(!sread(*file_access_,&header,sizeof(header))) ABORTALL; \
  if((header.cmd != CMD) || (header.size != (sizeof(res)+sizeof(errno_)+SIZE))) ABORTALL; \
  if(!sread(*file_access_,&res,sizeof(res))) ABORTALL; \
  if(!sread(*file_access_,&errno_,sizeof(errno_))) ABORTALL; \
}

  static void release_executer(Run* file_access) {
    delete file_access;
  }

  static void dispose_executer(Run* file_access) {
    delete file_access;
  }

  static bool do_tests = false;

  static Run* acquire_executer(uid_t uid,gid_t gid) {
    // TODO: pool
    std::list<std::string> argv;
    if(!do_tests) {
      argv.push_back(Arc::ArcLocation::Get()+G_DIR_SEPARATOR_S+PKGLIBEXECSUBDIR+G_DIR_SEPARATOR_S+"arc-file-access");
    } else {
      argv.push_back(std::string("..")+G_DIR_SEPARATOR_S+"arc-file-access");
    }
    argv.push_back("0");
    argv.push_back("1");
    Run* file_access_ = new Run(argv);
    file_access_->KeepStdin(false);
    file_access_->KeepStdout(false);
    file_access_->KeepStderr(true);
    if(!(file_access_->Start())) {
      delete file_access_;
      file_access_ = NULL;
      return NULL;
    }
    if(uid || gid) {
      for(int n=0;n<1;++n) {
        STARTHEADER(CMD_SETUID,sizeof(uid)+sizeof(gid));
        if(!swrite(*file_access_,&uid,sizeof(uid))) ABORTALL;
        if(!swrite(*file_access_,&gid,sizeof(gid))) ABORTALL;
        int res = 0;
        int errno_ = 0;
        ENDHEADER(CMD_SETUID,0);
        if(res != 0) ABORTALL;
      };
    };
    return file_access_;
  }

  static bool sread_buf(Run& r,void* buf,unsigned int& bufsize,unsigned int& maxsize) {
    char dummy[1024];
    unsigned int size;
    if(sizeof(size) > maxsize) return false;
    if(!sread(r,&size,sizeof(size))) return false;
    maxsize -= sizeof(size);
    if(size > maxsize) return false;
    if(size <= bufsize) {
      if(!sread(r,buf,size)) return false;
      bufsize = size;
      maxsize -= size;
    } else {
      if(!sread(r,buf,bufsize)) return false;
      maxsize -= bufsize;
      // skip rest
      size -= bufsize;
      while(size > sizeof(dummy)) {
        if(!sread(r,dummy,sizeof(dummy))) return false;
        size -= sizeof(dummy);
        maxsize -= sizeof(dummy);
      };
      if(!sread(r,dummy,size)) return false;
      maxsize -= size;
    };
    return true;
  }

  static bool swrite_string(Run& r,const std::string& str) {
    int l = str.length();
    if(!swrite(r,&l,sizeof(l))) return false;
    if(!swrite(r,str.c_str(),l)) return false;
    return true;
  }

#define RETRYLOOP Glib::Mutex::Lock mlock(lock_); for(int n = 2; n && (file_access_?file_access_:(file_access_=acquire_executer(uid_,gid_))) ;--n)

#define NORETRYLOOP Glib::Mutex::Lock mlock(lock_); for(int n = 1; n && (file_access_?file_access_:(file_access_=acquire_executer(uid_,gid_))) ;--n)

  FileAccess::FileAccess(void):file_access_(NULL),errno_(0),uid_(0),gid_(0) {
    file_access_ = acquire_executer(uid_,gid_);
  }

  FileAccess::~FileAccess(void) {
    release_executer(file_access_);
    file_access_ = NULL;
  }

  bool FileAccess::ping(void) {
    RETRYLOOP {
      STARTHEADER(CMD_PING,0);
      header_t header;
      if(!sread(*file_access_,&header,sizeof(header))) ABORTALL;
      if((header.cmd != CMD_PING) || (header.size != 0)) ABORTALL;
      return true;
    }
    return false;
  }

  bool FileAccess::setuid(int uid,int gid) {
    RETRYLOOP {
    STARTHEADER(CMD_SETUID,sizeof(uid)+sizeof(gid));
    if(!swrite(*file_access_,&uid,sizeof(uid))) ABORTALL;
    if(!swrite(*file_access_,&gid,sizeof(gid))) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_SETUID,0);
    if(res == 0) { uid_ = uid; gid_ = gid; };
    return (res == 0);
    }
    errno_ = -1;
    return false;
  }

  bool FileAccess::mkdir(const std::string& path, mode_t mode) {
    RETRYLOOP {
    STARTHEADER(CMD_MKDIR,sizeof(mode)+sizeof(int)+path.length());
    if(!swrite(*file_access_,&mode,sizeof(mode))) ABORTALL;
    if(!swrite_string(*file_access_,path)) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_MKDIR,0);
    return (res == 0);
    }
    errno_ = -1;
    return false;
  }

  bool FileAccess::mkdirp(const std::string& path, mode_t mode) {
    RETRYLOOP {
    STARTHEADER(CMD_MKDIRP,sizeof(mode)+sizeof(int)+path.length());
    if(!swrite(*file_access_,&mode,sizeof(mode))) ABORTALL;
    if(!swrite_string(*file_access_,path)) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_MKDIRP,0);
    return (res == 0);
    }
    errno_ = -1;
    return false;
  }

  bool FileAccess::link(const std::string& oldpath, const std::string& newpath) {
    RETRYLOOP {
    STARTHEADER(CMD_HARDLINK,sizeof(int)+oldpath.length()+sizeof(int)+newpath.length());
    if(!swrite_string(*file_access_,oldpath)) ABORTALL;
    if(!swrite_string(*file_access_,newpath)) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_HARDLINK,0);
    return (res == 0);
    }
    errno_ = -1;
    return false;
  }

  bool FileAccess::softlink(const std::string& oldpath, const std::string& newpath) {
    RETRYLOOP {
    STARTHEADER(CMD_SOFTLINK,sizeof(int)+oldpath.length()+sizeof(int)+newpath.length());
    if(!swrite_string(*file_access_,oldpath)) ABORTALL;
    if(!swrite_string(*file_access_,newpath)) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_SOFTLINK,0);
    return (res == 0);
    }
    errno_ = -1;
    return false;
  }

  bool FileAccess::copy(const std::string& oldpath, const std::string& newpath, mode_t mode) {
    RETRYLOOP {
    STARTHEADER(CMD_COPY,sizeof(mode)+sizeof(int)+oldpath.length()+sizeof(int)+newpath.length());
    if(!swrite(*file_access_,&mode,sizeof(mode))) ABORTALL;
    if(!swrite_string(*file_access_,oldpath)) ABORTALL;
    if(!swrite_string(*file_access_,newpath)) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_COPY,0);
    return (res == 0);
    }
    errno_ = -1;
    return false;
  }

  bool FileAccess::stat(const std::string& path, struct stat& st) {
    RETRYLOOP {
    STARTHEADER(CMD_STAT,sizeof(int)+path.length());
    if(!swrite_string(*file_access_,path)) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_STAT,sizeof(st));
    if(!sread(*file_access_,&st,sizeof(st))) ABORTALL;
    return (res == 0);
    }
    errno_ = -1;
    return false;
  }

  bool FileAccess::lstat(const std::string& path, struct stat& st) {
    RETRYLOOP {
    STARTHEADER(CMD_LSTAT,sizeof(int)+path.length());
    if(!swrite_string(*file_access_,path)) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_LSTAT,sizeof(st));
    if(!sread(*file_access_,&st,sizeof(st))) ABORTALL;
    return (res == 0);
    }
    errno_ = -1;
    return false;
  }

  bool FileAccess::fstat(struct stat& st) {
    RETRYLOOP {
    STARTHEADER(CMD_FSTAT,0);
    int res = 0;
    ENDHEADER(CMD_FSTAT,sizeof(st));
    if(!sread(*file_access_,&st,sizeof(st))) ABORTALL;
    return (res == 0);
    }
    errno_ = -1;
    return false;
  }

  bool FileAccess::ftruncate(off_t length) {
    RETRYLOOP {
    STARTHEADER(CMD_FTRUNCATE,sizeof(length));
    if(!swrite(*file_access_,&length,sizeof(length))) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_FTRUNCATE,0);
    return (res == 0);
    }
    errno_ = -1;
    return false;
  }

  off_t FileAccess::fallocate(off_t length) {
    RETRYLOOP {
    STARTHEADER(CMD_FALLOCATE,sizeof(length));
    if(!swrite(*file_access_,&length,sizeof(length))) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_FALLOCATE,sizeof(length));
    if(!sread(*file_access_,&length,sizeof(length))) ABORTALL;
    return length;
    }
    errno_ = -1;
    return -1;
  }

  bool FileAccess::readlink(const std::string& path, std::string& linkpath) {
    RETRYLOOP {
    STARTHEADER(CMD_READLINK,sizeof(int)+path.length());
    if(!swrite_string(*file_access_,path)) ABORTALL;
    int res = 0;
    int l = 0;
    header_t header;
    if(!sread(*file_access_,&header,sizeof(header))) ABORTALL;
    if((header.cmd != CMD_READLINK) || (header.size < (sizeof(res)+sizeof(errno_)+sizeof(int)))) ABORTALL;
    if(!sread(*file_access_,&res,sizeof(res))) ABORTALL;
    if(!sread(*file_access_,&errno_,sizeof(errno_))) ABORTALL;
    if(!sread(*file_access_,&l,sizeof(l))) ABORTALL;
    if((sizeof(res)+sizeof(errno_)+sizeof(l)+l) != header.size) ABORTALL;
    linkpath.assign(l,' ');
    if(!sread(*file_access_,(void*)linkpath.c_str(),l)) ABORTALL;
    return (res >= 0);
    }
    errno_ = -1;
    return false;
  }

  bool FileAccess::remove(const std::string& path) {
    RETRYLOOP {
    STARTHEADER(CMD_REMOVE,sizeof(int)+path.length());
    if(!swrite_string(*file_access_,path)) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_REMOVE,0);
    return (res == 0);
    }
    errno_ = -1;
    return false;
  }

  bool FileAccess::unlink(const std::string& path) {
    RETRYLOOP {
    STARTHEADER(CMD_UNLINK,sizeof(int)+path.length());
    if(!swrite_string(*file_access_,path)) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_UNLINK,0);
    return (res == 0);
    }
    errno_ = -1;
    return false;
  }

  bool FileAccess::rmdir(const std::string& path) {
    RETRYLOOP {
    STARTHEADER(CMD_RMDIR,sizeof(int)+path.length());
    if(!swrite_string(*file_access_,path)) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_RMDIR,0);
    return (res == 0);
    }
    errno_ = -1;
    return false;
  }

  bool FileAccess::rmdirr(const std::string& path) {
    RETRYLOOP {
    STARTHEADER(CMD_RMDIRR,sizeof(int)+path.length());
    if(!swrite_string(*file_access_,path)) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_RMDIRR,0);
    return (res == 0);
    }
    errno_ = -1;
    return false;
  }

  bool FileAccess::opendir(const std::string& path) {
    RETRYLOOP {
    STARTHEADER(CMD_OPENDIR,sizeof(int)+path.length());
    if(!swrite_string(*file_access_,path)) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_OPENDIR,0);
    return (res == 0);
    }
    errno_ = -1;
    return false;
  }

  bool FileAccess::closedir(void) {
    NORETRYLOOP {
    STARTHEADER(CMD_CLOSEDIR,0);
    int res = 0;
    ENDHEADER(CMD_CLOSEDIR,0);
    return (res == 0);
    }
    errno_ = -1;
    return false;
  }

  bool FileAccess::readdir(std::string& name) {
    NORETRYLOOP {
    STARTHEADER(CMD_READDIR,0);
    int res = 0;
    int l = 0;
    header_t header;
    if(!sread(*file_access_,&header,sizeof(header))) ABORTALL;
    if((header.cmd != CMD_READDIR) || (header.size < (sizeof(res)+sizeof(errno_)+sizeof(l)))) ABORTALL;
    if(!sread(*file_access_,&res,sizeof(res))) ABORTALL;
    if(!sread(*file_access_,&errno_,sizeof(errno_))) ABORTALL;
    if(!sread(*file_access_,&l,sizeof(l))) ABORTALL;
    if((sizeof(res)+sizeof(errno_)+sizeof(l)+l) != header.size) ABORTALL;
    name.assign(l,' ');
    if(!sread(*file_access_,(void*)name.c_str(),l)) ABORTALL;
    return (res == 0);
    }
    errno_ = -1;
    return false;
  }

  bool FileAccess::open(const std::string& path, int flags, mode_t mode) {
    RETRYLOOP {
    STARTHEADER(CMD_OPENFILE,sizeof(flags)+sizeof(mode)+sizeof(int)+path.length());
    if(!swrite(*file_access_,&flags,sizeof(flags))) ABORTALL;
    if(!swrite(*file_access_,&mode,sizeof(mode))) ABORTALL;
    if(!swrite_string(*file_access_,path)) ABORTALL;
    int res = -1;
    ENDHEADER(CMD_OPENFILE,0);
    return (res != -1);
    }
    errno_ = -1;
    return false;
  }

  bool FileAccess::close(void) {
    NORETRYLOOP {
    STARTHEADER(CMD_CLOSEFILE,0);
    int res = 0;
    ENDHEADER(CMD_CLOSEFILE,0);
    return (res == 0);
    }
    errno_ = -1;
    return false;
  }

  off_t FileAccess::lseek(off_t offset, int whence) {
    NORETRYLOOP {
    STARTHEADER(CMD_SEEKFILE,sizeof(offset)+sizeof(whence));
    if(!swrite(*file_access_,&offset,sizeof(offset))) ABORTALL;
    if(!swrite(*file_access_,&whence,sizeof(whence))) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_SEEKFILE,sizeof(offset));
    if(!sread(*file_access_,&offset,sizeof(offset))) ABORTALL;
    return offset;
    }
    errno_ = -1;
    return (off_t)(-1);
  }

  ssize_t FileAccess::read(void* buf,size_t size) {
    NORETRYLOOP {
    STARTHEADER(CMD_READFILE,sizeof(size));
    if(!swrite(*file_access_,&size,sizeof(size))) ABORTALL;
    int res = 0;
    header_t header;
    if(!sread(*file_access_,&header,sizeof(header))) ABORTALL;
    if((header.cmd != CMD_READFILE) || (header.size < (sizeof(res)+sizeof(errno_)))) ABORTALL;
    if(!sread(*file_access_,&res,sizeof(res))) ABORTALL;
    if(!sread(*file_access_,&errno_,sizeof(errno_))) ABORTALL;
    header.size -= sizeof(res)+sizeof(errno_);
    unsigned int l = size;
    if(!sread_buf(*file_access_,buf,l,header.size)) ABORTALL;
    return (res < 0)?res:l;
    }
    errno_ = -1;
    return -1;
  }

  ssize_t FileAccess::pread(void* buf,size_t size,off_t offset) {
    NORETRYLOOP {
    STARTHEADER(CMD_READFILEAT,sizeof(size)+sizeof(offset));
    if(!swrite(*file_access_,&size,sizeof(size))) ABORTALL;
    if(!swrite(*file_access_,&offset,sizeof(offset))) ABORTALL;
    int res = 0;
    header_t header;
    if(!sread(*file_access_,&header,sizeof(header))) ABORTALL;
    if((header.cmd != CMD_READFILEAT) || (header.size < (sizeof(res)+sizeof(errno_)))) ABORTALL;
    if(!sread(*file_access_,&res,sizeof(res))) ABORTALL;
    if(!sread(*file_access_,&errno_,sizeof(errno_))) ABORTALL;
    header.size -= sizeof(res)+sizeof(errno_);
    unsigned int l = size;
    if(!sread_buf(*file_access_,buf,l,header.size)) ABORTALL;
    return (res < 0)?res:l;
    }
    errno_ = -1;
    return -1;
  }

  ssize_t FileAccess::write(const void* buf,size_t size) {
    NORETRYLOOP {
    unsigned int l = size;
    STARTHEADER(CMD_WRITEFILE,sizeof(l)+l);
    if(!swrite(*file_access_,&l,sizeof(l))) ABORTALL;
    if(!swrite(*file_access_,buf,l)) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_WRITEFILE,0);
    return res;
    }
    errno_ = -1;
    return -1;
  }

  ssize_t FileAccess::pwrite(const void* buf,size_t size,off_t offset) {
    NORETRYLOOP {
    unsigned int l = size;
    STARTHEADER(CMD_WRITEFILEAT,sizeof(offset)+sizeof(l)+l);
    if(!swrite(*file_access_,&offset,sizeof(offset))) ABORTALL;
    if(!swrite(*file_access_,&l,sizeof(l))) ABORTALL;
    if(!swrite(*file_access_,buf,l)) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_WRITEFILEAT,0);
    return res;
    }
    errno_ = -1;
    return -1;
  }

  void FileAccess::testtune(void) {
    do_tests = true;
  }

}

