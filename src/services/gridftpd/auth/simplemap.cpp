#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <fstream>
#include <list>
#include <utime.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <glibmm/miscutils.h>

#include <arc/StringConv.h>
#include <arc/Logger.h>

#include "simplemap.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(),"SimpleMap");

class FileLock {
 private:
  int h_;
  struct flock l_;
 public:
  FileLock(int h):h_(h) {
    if(h_ == -1) return;
    l_.l_type=F_WRLCK;
    l_.l_whence=SEEK_SET;
    l_.l_start=0;
    l_.l_len=0;
    for(;;) {
      if(fcntl(h_,F_SETLKW,&l_) == 0) break;
      if(errno != EINTR) { h_=-1; return; };
    };
  };
  ~FileLock(void) {
    if(h_ == -1) return;
    l_.l_type=F_UNLCK;
    fcntl(h_,F_SETLKW,&l_);
  };
  operator bool(void) const { return (h_ != -1); };
  bool operator!(void) const { return (h_ == -1); };
};

SimpleMap::SimpleMap(const char* dir):dir_(dir) {
  if((dir_.length() == 0) || (dir_[dir_.length()-1] != '/')) dir_+="/";
  if(dir_[0] != '/') dir_=Glib::get_current_dir()+"/"+dir_;
  pool_handle_=open((dir_+"pool").c_str(),O_RDWR);
  selfunmap_time_ = SELFUNMAP_TIME;
  std::ifstream config(dir_+"config");
  while(config.good()) {
    std::string str;
    getline(config, str);
    std::string::size_type sep = str.find('=');
    if(sep != std::string::npos) {
      // So far only one command is supported
      if(str.substr(0,sep) == "timeout") {
        unsigned int n;
        if(Arc::stringto(str.substr(sep+1), n)) {
          selfunmap_time_ = n*24*60*60;
          logger.msg(Arc::VERBOSE, "SimpleMap: acquired new unmap time of %u seconds", selfunmap_time_);
        } else {
          logger.msg(Arc::ERROR, "SimpleMap: wrong number in unmaptime command", str.substr(sep+1));
        }
      }
    }
  }
}

SimpleMap::~SimpleMap(void) {
  if(pool_handle_ != -1) close(pool_handle_);
  pool_handle_=-1;
}

#define failure(S) { \
  logger.msg(Arc::ERROR, "SimpleMap: %s", (S)); \
  return ""; \
}

#define info(S) { \
  logger.msg(Arc::INFO, "SimpleMap: %s", (S)); \
}

std::string SimpleMap::map(const char* subject) {
  if(pool_handle_ == -1) failure("not initialized");
  if(!subject) failure("missing subject");
  std::string filename(subject);
  for(std::string::size_type i = filename.find('/');i!=std::string::npos;
      i=filename.find('/',i+1)) filename[i]='_';
  filename=dir_+filename;
  FileLock lock(pool_handle_);
  if(!lock) failure("failed to lock pool file");
  // Check for existing mapping
  struct stat st;
  if(stat(filename.c_str(),&st) == 0) {
    if(!S_ISREG(st.st_mode)) failure("mapping is not a regular file");
    std::ifstream f(filename.c_str());
    if(!f.is_open()) failure("can't open mapping file");
    std::string buf;
    getline(f,buf);
    utime(filename.c_str(),NULL);
    return buf;
  };
  // Look for unused names
  // Get full list first.
  std::list<std::string> names;
  {
    std::ifstream f((dir_+"pool").c_str());
    if(!f.is_open()) failure("can't open pool file")
    std::string buf;
    while(getline(f,buf)) {
      if(buf.empty()) continue;
      names.push_back(buf);
    };
  };
  if(names.empty()) failure("pool is empty");
  // Remove all used names from list. Also find oldest maping.
  time_t oldmap_time = 0;
  std::string oldmap_name;
  std::string oldmap_subject;
  {
    struct dirent file_;
    struct dirent *file;
    DIR *dir=opendir(dir_.c_str());
    if(dir == NULL) failure("can't list pool directory");
    for(;;) {
      readdir_r(dir,&file_,&file);
      if(file == NULL) break;
      if(std::string(file->d_name) == ".") continue;
      if(std::string(file->d_name) == "..") continue;
      if(std::string(file->d_name) == "pool") continue;
      std::string filename = dir_+file->d_name;
      struct stat st;
      if(stat(filename.c_str(),&st) != 0) continue;
      if(!S_ISREG(st.st_mode)) continue;
      std::ifstream f(filename.c_str());
      if(!f.is_open()) { // trash in directory
        closedir(dir); failure("can't open one of mapping files"); 
      };
      std::string buf;
      getline(f,buf);
      // find this name in list
      std::list<std::string>::iterator i = names.begin();
      for(;i!=names.end();++i) if(*i == buf) break;
      if(i == names.end()) {
        // Always try to destroy old mappings without corresponding 
        // entry in the pool file
        if((selfunmap_time_ > 0) && (((unsigned int)(time(NULL) - st.st_mtime)) >= selfunmap_time_)) {
          unlink(filename.c_str());
        };
      } else {
        names.erase(i);
        if( (oldmap_name.length() == 0) ||
            (((int)(oldmap_time - st.st_mtime)) > 0) ) {
          oldmap_name=buf;
          oldmap_subject=file->d_name;
          oldmap_time=st.st_mtime;
        };
      };
    };
    closedir(dir);
  };
  if(!names.empty()) {
    // Claim one of unused names
    std::ofstream f(filename.c_str());
    if(!f.is_open()) failure("can't create mapping file");
    f<<*(names.begin())<<std::endl;
    info(std::string("Mapped ")+subject+" to "+(*(names.begin())));
    return *(names.begin());
  };
  // Try to release one of old names
  if(selfunmap_time_ == 0) failure("old mappings are not allowed to expire");
  if(oldmap_name.length() == 0) failure("no old mappings found");
  if(((unsigned int)(time(NULL) - oldmap_time)) < selfunmap_time_) failure("no old enough mappings found");
  // releasing the old entry
  info(std::string("Releasing expired mapping of ")+oldmap_subject+
       " to "+oldmap_name+" back to pool");
  if(unlink((dir_+oldmap_subject).c_str()) != 0) failure("failed to remove mapping file");
  // writing the new mapping to the file
  std::ofstream f(filename.c_str());
  if(!f.is_open()) failure("can't create mapping file");
  f<<oldmap_name<<std::endl;
  info(std::string("Mapped ")+subject+" to "+oldmap_name);
  return oldmap_name;
}

bool SimpleMap::unmap(const char* subject) {
  if(pool_handle_ == -1) return false;
  FileLock lock(pool_handle_);
  if(!lock) return false;
  if(unlink((dir_+subject).c_str()) == 0) return true;
  if(errno == ENOENT) return true;
  return false;
}

