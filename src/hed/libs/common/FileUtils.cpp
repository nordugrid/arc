#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// These utilities are implemented using POSIX.
// But most used features are availble in MinGW
// and hence code should compile in windows environment.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <glibmm.h>
#ifndef WIN32
#include <poll.h>
#include <sys/mman.h>
#else
#include <arc/win32.h>
#endif

#include <arc/StringConv.h>
#include <arc/DateTime.h>
#include <arc/User.h>
#include <arc/GUID.h>

#include "FileUtils.h"

namespace Arc {

Glib::Mutex suid_lock;

static bool write_all(int h,const void* buf,size_t l) {
  for(;l>0;) {
    ssize_t ll = write(h,buf,l);
    if(ll == -1) return false;
    buf = (const void*)(((const char*)buf)+ll);
    l-=ll;
  }
  return true;
}

int FileOpen(const std::string& path,int flags,mode_t mode) {
  return FileOpen(path,flags,0,0,mode);
}

int FileOpen(const std::string& path,int flags,uid_t uid,gid_t gid,mode_t mode) {
  int h = -1;
#ifndef WIN32
  {
    UserSwitch usw(uid,gid);
    if(!usw) return -1;
    h = open(path.c_str(),flags | O_NONBLOCK,mode);
  };
  if(h == -1) return -1;
  if(flags & O_NONBLOCK) return h;
  while(1) {
    pollfd ph;
    ph.fd=h; ph.events=POLLOUT; ph.revents=0;
    if(poll(&ph,1,-1) <= 0) {
      if(errno != EINTR) {
        close(h);
        return -1;
      };
    };
    if(ph.revents & POLLOUT) break;
  };
  int fl = fcntl(h,F_GETFL);
  if(fl == -1) {
    close(h);
    return -1;
  };
  fl &= ~O_NONBLOCK;
  if(fcntl(h,F_SETFL,fl) == -1) {
    close(h);
    return -1;
  };
#else
  {
    UserSwitch usw(uid,gid);
    if(!usw) return -1;
    h = open(path.c_str(),flags,mode);
  };
#endif
  return h;
}

bool FileCopy(const std::string& source_path,const std::string& destination_path) {
  struct stat st;
  int source_handle = FileOpen(source_path,O_RDONLY,0,0,0);
  if(source_handle == -1) return false;
  if(!FileStat(source_path,&st,true)) return false;
  int destination_handle = FileOpen(destination_path,O_WRONLY | O_CREAT | O_TRUNC,0,0,st.st_mode);
  if(destination_handle == -1) {
    ::close(source_handle);
    return false;
  }
  bool r = FileCopy(source_handle,destination_handle);
  ::close(source_handle);
  ::close(destination_handle);
  return r;
}

bool FileCopy(const std::string& source_path,int destination_handle) {
  int source_handle = FileOpen(source_path,O_RDONLY,0,0,0);
  if(source_handle == -1) return false;
  if(::ftruncate(destination_handle,0) != 0) {
    ::close(source_handle);
    return false;
  }
  bool r = FileCopy(source_handle,destination_handle);
  ::close(source_handle);
  return r;
}

bool FileCopy(int source_handle,const std::string& destination_path) {
  int destination_handle = FileOpen(destination_path,O_WRONLY | O_CREAT | O_TRUNC,0,0,0600);
  if(destination_handle == -1) return false;
  bool r = FileCopy(source_handle,destination_handle);
  ::close(destination_handle);
  return r;
}

#define FileCopyBigThreshold (50*1024*1024)
#define FileCopyBufSize (4*1024)

bool FileCopy(int source_handle,int destination_handle) {
  off_t source_size = lseek(source_handle,0,SEEK_END);
  if(source_size == (off_t)(-1)) return false;
  if(source_size == 0) return true;
#ifndef WIN32
  if(source_size <= FileCopyBigThreshold) {
    void* source_addr = mmap(NULL,source_size,PROT_READ,MAP_SHARED,source_handle,0);
    if(source_addr != (void *)(-1)) {
      bool r = write_all(destination_handle,source_addr,source_size);
      munmap(source_addr,source_size);
      return r;
    }
  }
#endif
  if(lseek(source_handle,0,SEEK_SET) != 0) return false;
  char* buf = new char[FileCopyBufSize];
  if(!buf) return false;
  bool r = true;
  for(;;) {
    ssize_t l = FileCopyBufSize;
    l=::read(source_handle,buf,l);
    if(l == 0) break; // less than expected
    if(l == -1) {
      // EWOULDBLOCK
      r = false;
      break;
    }
    if(!write_all(destination_handle,buf,l)) {
      r = false;
      break;
    }
  }
  return r;
}

bool FileRead(const std::string& filename, std::list<std::string>& data, uid_t uid, gid_t gid) {
  data.clear();
  UserSwitch usw(uid, gid);
  std::ifstream is(filename.c_str());
  if (!is.good()) {
    is.close();
    return false;
  }
  std::string line;
  while (std::getline(is, line)) {
    data.push_back(line);
  }
  is.close();
  return true;
}

bool FileCreate(const std::string& filename, const std::string& data, uid_t uid, gid_t gid) {
  UserSwitch usw(uid, gid);
  if (remove(filename.c_str()) != 0 && errno != ENOENT)
    return false;
  std::ofstream os(filename.c_str());
  if (!os.good()) {
    os.close();
    return false;
  }
  os << data;
  os.close();
  return true;
}


Glib::Dir* DirOpen(const std::string& path) {
  return DirOpen(path,0,0);
}

// TODO: find non-blocking way to open directory
Glib::Dir* DirOpen(const std::string& path,uid_t uid,gid_t gid) {
  Glib::Dir* dir = NULL;
  {
    UserSwitch usw(uid,gid);
    if(!usw) return NULL;
    try {
      dir = new Glib::Dir(path);
    } catch(Glib::FileError& e) {
      // err=e.what();
    };
  };
  return dir;
}

bool FileStat(const std::string& path,struct stat *st,bool follow_symlinks) {
  return FileStat(path,st,0,0,follow_symlinks);
}

// TODO: maybe by using open + fstat it would be possible to 
// make this functin less blocking
bool FileStat(const std::string& path,struct stat *st,uid_t uid,gid_t gid,bool follow_symlinks) {
  int r = -1;
  {
    UserSwitch usw(uid,gid);
    if(!usw) return false;
    if(follow_symlinks) {
      r = ::stat(path.c_str(),st);
    } else {
      r = ::lstat(path.c_str(),st);
    };
  };
  return (r == 0);
}

bool FileLink(const std::string& oldpath,const std::string& newpath,bool symbolic) {
  return FileLink(oldpath,newpath,0,0,symbolic);
}

bool FileLink(const std::string& oldpath,const std::string& newpath,uid_t uid,gid_t gid,bool symbolic) {
  UserSwitch usw(uid,gid);
  if(!usw) return false;
  if(symbolic) {
    return (symlink(oldpath.c_str(),newpath.c_str()) == 0);
  } else {
    return (link(oldpath.c_str(),newpath.c_str()) == 0);
  }
}

bool DirCreate(const std::string& path,mode_t mode,bool with_parents) {
  return DirCreate(path,0,0,mode,with_parents);
}

std::string FileReadLink(const std::string& path) {
  return FileReadLink(path,0,0);
}

std::string FileReadLink(const std::string& path,uid_t uid,gid_t gid) {
  class charbuf {
   private:
    char* v;
   public:
    charbuf(int size) {
      v = new char[size];
    };
    ~charbuf(void) {
      delete[] v;
    };
    char* str(void) {
      return v;
    };
    char& operator[](int n) {
      return v[n];
    };
  };
  const int bufsize = 1024;
  UserSwitch usw(uid,gid);
  charbuf buf(bufsize);
  ssize_t l = readlink(path.c_str(),buf.str(),bufsize);
  if(l<0) {
    l = 0;
  } else if(l>bufsize) {
    l = bufsize; 
  }
  return std::string(buf.str(),l);
}

bool FileDelete(const std::string& path) {
  return FileDelete(path,0,0);
}

bool FileDelete(const std::string& path,uid_t uid,gid_t gid) {
  UserSwitch usw(uid,gid);
  return (unlink(path.c_str()) == 0);
}

// TODO: find non-blocking way to create directory
bool DirCreate(const std::string& path,uid_t uid,gid_t gid,mode_t mode,bool with_parents) {
  {
    UserSwitch usw(uid,gid);
    if(!usw) return false;
    if(::mkdir(path.c_str(),mode) == 0) return true;
  }
  if(errno == EEXIST) {
    /*
    Should it be just dumb mkdir or something clever?
    struct stat st; 
    int r = ::stat(path.c_str(),&st);
    if((r == 0) && (S_ISDIR(st.st_mode))) {
      if((uid == 0) || (st.st_uid == uid)) {
        if((gid == 0) || (st.st_gid == gid) ||
           (::chown(path.c_str(),(uid_t)(-1),gid) == 0)) {
          // mode ?
          return true;
        }
      }
    }
    */
    return true;
  } else if(errno == ENOENT) {
    if(with_parents) {
      std::string ppath(path);
      if(!Glib::path_is_absolute(ppath)) {
        ppath=Glib::get_current_dir()+G_DIR_SEPARATOR_S+ppath;
      }
      std::string::size_type pos = ppath.rfind(G_DIR_SEPARATOR_S);
      if((pos != 0) && (pos != std::string::npos)) {
        ppath.resize(pos);
        if(!DirCreate(ppath,uid,gid,mode,true)) return false;
        UserSwitch usw(uid,gid);
        if(!usw) return false;
        if(::mkdir(path.c_str(),mode) == 0) return true;
      }
    }
  }
  return false;
}


bool DirDelete(const std::string& path,uid_t uid,gid_t gid) {

  bool r = false;
  {
    UserSwitch usw(uid, gid);
    if (!usw) return false;
    r = DirDelete(path);
  }
  return r;
}  

bool DirDelete(const std::string& path) {

  struct stat st;
  if (::stat(path.c_str(), &st) != 0 || ! S_ISDIR(st.st_mode))
    return false;
  try {
    Glib::Dir dir(path);
    std::string file_name;
    while ((file_name = dir.read_name()) != "") {
      std::string fullpath(path);
      fullpath += '/' + file_name;
      if (::lstat(fullpath.c_str(), &st) != 0) return false;
      if (S_ISDIR(st.st_mode)) {
        if (!DirDelete(fullpath.c_str())) {
          return false;
        }
      } else {
        if (::remove(fullpath.c_str()) != 0) {
          return false;
        }
      }
    } 
  }
  catch (Glib::FileError& e) {
    return false;
  }
  if (rmdir(path.c_str()) != 0) return false;
      
  return true;
}

bool TmpDirCreate(std::string& path) {
  std::string tmpdir(Glib::get_tmp_dir());
  bool result = false;
#ifdef HAVE_MKDTEMP
  char tmpdirtemplate[] = "ARC-XXXXXX";
  tmpdir = Glib::build_filename(tmpdir, tmpdirtemplate);
  char* tmpdirchar = mkdtemp(const_cast<char*>(tmpdir.c_str()));
  if (tmpdirchar) {
    path = tmpdirchar;
    result = true;
  }
#else
  // NOTE: not safe!
  std::string tmp("ARC-" + UUID());
  path = Glib::build_filename(tmpdir, tmp);
  result = DirCreate(path, 0700, true);
#endif
  return result;
}

} // namespace Arc


