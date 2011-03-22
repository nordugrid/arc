#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>
#include <iostream>

#include <arc/Run.h>

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

  static ssize_t swrite(Run& r,const void* buf,size_t size) {
    while(size) {
std::cerr<<"-- swrite: size="<<size<<std::endl;
      int l = r.WriteStdin(-1,(const char*)buf,size);
std::cerr<<"-- swrite: l="<<l<<std::endl;
      if(l < 0) return false;
      size-=l;
      buf = (void*)(((char*)buf)+l);
    };
    return true;
  }

  static ssize_t swrite_string(Run& r,const std::string& str) {
    int l = str.length();
    if(!swrite(r,&l,sizeof(l))) return false;
    if(!swrite(r,str.c_str(),l)) return false;
    return true;
  }

#define ABORTALL { delete file_access_; file_access_=NULL; return false; }

#define STARTHEADER(CMD,SIZE) { \
  if(!file_access_) return false; \
  if(!(file_access_->Running())) return false; \
  header_t header; \
  header.cmd = CMD; \
  header.size = SIZE; \
  if(!swrite(*file_access_,&header,sizeof(header))) ABORTALL; \
}

#define ENDHEADER(CMD,SIZE) { \
  header_t header; \
  if(!sread(*file_access_,&header,sizeof(header))) ABORTALL; \
  if((header.cmd != CMD) || (header.size != (sizeof(res)+sizeof(errno_)+SIZE))) ABORTALL; \
  if(!sread(*file_access_,&res,sizeof(res))) ABORTALL; \
  if(!sread(*file_access_,&errno_,sizeof(errno_))) ABORTALL; \
}


  FileAccess::FileAccess(void):file_access_(NULL),errno_(0) {
    std::list<std::string> argv;
    argv.push_back("./arc-file-access");
    argv.push_back("0");
    argv.push_back("1");
    file_access_ = new Run(argv);
    file_access_->KeepStdin(false);
    file_access_->KeepStdout(false);
    file_access_->KeepStderr(true);
    if(!(file_access_->Start())) {
      delete file_access_;
      file_access_ = NULL;
    };
  }

  FileAccess::~FileAccess(void) {
    if(file_access_) {
      delete file_access_;
      file_access_ = NULL;
    };
  }

  bool FileAccess::ping(void) {
    STARTHEADER(CMD_PING,0);
    header_t header;
    if(!sread(*file_access_,&header,sizeof(header))) ABORTALL;
    if((header.cmd != CMD_PING) || (header.size != 0)) ABORTALL;
    return true;
  }

  bool FileAccess::setuid(int uid,int gid) {
    STARTHEADER(CMD_SETUID,sizeof(uid)+sizeof(gid));
    if(!swrite(*file_access_,&uid,sizeof(uid))) ABORTALL;
    if(!swrite(*file_access_,&gid,sizeof(gid))) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_SETUID,0);
    return (res == 0);
  }

  bool FileAccess::mkdir(const std::string& path, mode_t mode) {
    STARTHEADER(CMD_MKDIR,sizeof(mode)+sizeof(int)+path.length());
    if(!swrite(*file_access_,&mode,sizeof(mode))) ABORTALL;
    if(!swrite_string(*file_access_,path)) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_MKDIR,0);
    return (res == 0);
  }

  bool FileAccess::link(const std::string& oldpath, const std::string& newpath) {
    STARTHEADER(CMD_HARDLINK,sizeof(int)+oldpath.length()+sizeof(int)+newpath.length());
    if(!swrite_string(*file_access_,oldpath)) ABORTALL;
    if(!swrite_string(*file_access_,newpath)) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_HARDLINK,0);
    return (res == 0);
  }

  bool FileAccess::softlink(const std::string& oldpath, const std::string& newpath) {
    STARTHEADER(CMD_SOFTLINK,sizeof(int)+oldpath.length()+sizeof(int)+newpath.length());
    if(!swrite_string(*file_access_,oldpath)) ABORTALL;
    if(!swrite_string(*file_access_,newpath)) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_SOFTLINK,0);
    return (res == 0);
  }

  bool FileAccess::stat(const std::string& path, struct stat& st) {
    STARTHEADER(CMD_STAT,sizeof(int)+path.length());
    if(!swrite_string(*file_access_,path)) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_STAT,sizeof(st));
    if(!sread(*file_access_,&st,sizeof(st))) ABORTALL;
    return (res == 0);
  }

  bool FileAccess::lstat(const std::string& path, struct stat& st) {
    STARTHEADER(CMD_LSTAT,sizeof(int)+path.length());
    if(!swrite_string(*file_access_,path)) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_LSTAT,sizeof(st));
    if(!sread(*file_access_,&st,sizeof(st))) ABORTALL;
    return (res == 0);
  }

  bool FileAccess::remove(const std::string& path) {
    STARTHEADER(CMD_REMOVE,sizeof(int)+path.length());
    if(!swrite_string(*file_access_,path)) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_REMOVE,0);
    return (res == 0);
  }

  bool FileAccess::unlink(const std::string& path) {
    STARTHEADER(CMD_UNLINK,sizeof(int)+path.length());
    if(!swrite_string(*file_access_,path)) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_UNLINK,0);
    return (res == 0);
  }

  bool FileAccess::rmdir(const std::string& path) {
    STARTHEADER(CMD_RMDIR,sizeof(int)+path.length());
    if(!swrite_string(*file_access_,path)) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_RMDIR,0);
    return (res == 0);
  }

  bool FileAccess::opendir(const std::string& path) {
    STARTHEADER(CMD_OPENDIR,sizeof(int)+path.length());
    if(!swrite_string(*file_access_,path)) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_OPENDIR,0);
    return (res == 0);
  }

  bool FileAccess::closedir(void) {
    STARTHEADER(CMD_CLOSEDIR,0);
    int res = 0;
    ENDHEADER(CMD_CLOSEDIR,0);
    return (res == 0);
  }

  bool FileAccess::readdir(std::string& name) {
    return false;
  }

  bool FileAccess::open(const std::string& path, int flags, mode_t mode) {
    STARTHEADER(CMD_OPENFILE,sizeof(int)+sizeof(flags)+sizeof(mode)+path.length());
    if(!swrite(*file_access_,&flags,sizeof(flags))) ABORTALL;
    if(!swrite(*file_access_,&mode,sizeof(mode))) ABORTALL;
    if(!swrite_string(*file_access_,path)) ABORTALL;
    int res = 0;
    ENDHEADER(CMD_OPENFILE,0);
    return (res == 0);
  }

  bool FileAccess::close(void) {
    STARTHEADER(CMD_CLOSEFILE,0);
    int res = 0;
    ENDHEADER(CMD_CLOSEFILE,0);
    return (res == 0);
  }

  off_t FileAccess::lseek(off_t offset, int whence) {
    return -1;
  }

  ssize_t FileAccess::read(void* buf,size_t size) {
    return -1;
  }

  ssize_t FileAccess::write(const void* buf,size_t size) {
    return -1;
  }

}

