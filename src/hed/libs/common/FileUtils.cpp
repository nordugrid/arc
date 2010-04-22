#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef WIN32
// These utilities are POSIX specific.
// They may work in MinGW but that needs testing.
// So currently they are disabled in windows environment.

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <glibmm.h>

#include "FileUtils.h"
#include "User.h"

namespace Arc {

Glib::Mutex suid_lock;

int FileOpen(const char* path,int flags,mode_t mode) {
  return FileOpen(path,flags,0,0,mode);
}

int FileOpen(const char* path,int flags,uid_t uid,gid_t gid,mode_t mode) {
  int h = -1;
  {
    UserSwitch usw(uid,gid);
    if(!usw) return -1;
    h = open(path,flags | O_NONBLOCK,mode);
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
  return h;
}

Glib::Dir* DirOpen(const char* path) {
  return DirOpen(path,0,0);
}

// TODO: find non-blocking way to open directory
Glib::Dir* DirOpen(const char* path,uid_t uid,gid_t gid) {
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

bool FileStat(const char* path,struct stat *st,bool follow_symlinks) {
  return FileStat(path,st,0,0,follow_symlinks);
}

// TODO: maybe by using open + fstat it would be possible to 
// make this functin less blocking
bool FileStat(const char* path,struct stat *st,uid_t uid,gid_t gid,bool follow_symlinks) {
  int r = -1;
  {
    UserSwitch usw(uid,gid);
    if(!usw) return false;
    if(follow_symlinks) {
      r = stat(path,st);
    } else {
      r = lstat(path,st);
    };
  };
  return (r == 0);
}

bool DirCreate(const char* path,mode_t mode) {
  return DirCreate(path,0,0,mode);
}

// TODO: find non-blocking way to create directory
bool DirCreate(const char* path,uid_t uid,gid_t gid,mode_t mode) {
  int r = -1;
  {
    UserSwitch usw(uid,gid);
    if(!usw) return false;
    r = mkdir(path,mode);
  };
  return (r == 0);
}


bool DirDelete(const char* path,uid_t uid,gid_t gid) {

  bool r = false;
  {
    UserSwitch usw(uid, gid);
    if (!usw) return false;
    r = DirDelete(path);
  }
  return r;
}  

bool DirDelete(const char* path) {

  struct stat st;
  if (stat(path, &st) != 0 || ! S_ISDIR(st.st_mode))
    return false;
  try {
    Glib::Dir dir(path);
    std::string file_name;
    while ((file_name = dir.read_name()) != "") {
      std::string fullpath(path);
      fullpath += '/' + file_name;
      if (stat(fullpath.c_str(), &st) != 0) return false;
      if (S_ISDIR(st.st_mode)) {
        if (!DirDelete(fullpath.c_str())) {
          return false;
        }
      } else {
        if (remove(fullpath.c_str()) != 0) {
          return false;
        }
      }
    } 
  }
  catch (Glib::FileError& e) {
    return false;
  }
  if (rmdir(path) != 0) return false;
      
  return true;
}

} // namespace Arc

#endif

