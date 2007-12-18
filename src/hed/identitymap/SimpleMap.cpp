#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <fstream>
#include <list>
#include <utime.h>
#include <sys/file.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#define odlog(LEVEL) std::cerr

#include "SimpleMap.h"

namespace ArcSec {

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
  operator bool(void) { return (h_ != -1); };
  bool operator!(void) { return (h_ == -1); };
};

SimpleMap::SimpleMap(const std::string& dir):dir_(dir) {
  if((dir_.length() == 0) || (dir_[dir_.length()-1] != '/')) dir_+="/";
  if(dir_[0] != '/') {
    char buf[PATH_MAX];
    if(getcwd(buf,sizeof(buf))) dir_=std::string(buf)+"/"+dir_;
  };
  pool_handle_=open((dir_+"pool").c_str(),O_RDWR);
}

SimpleMap::~SimpleMap(void) {
  if(pool_handle_ != -1) close(pool_handle_);
  pool_handle_=-1;
}

#define failure(S) { \
  odlog(ERROR)<<"SimpleMap: "<<(S)<<std::endl; \
  return ""; \
}

#define info(S) { \
  odlog(INFO)<<"SimpleMap: "<<(S)<<std::endl; \
}

std::string SimpleMap::map(const std::string& subject) {
  if(pool_handle_ == -1) failure("not initialized");
  if(subject.empty()) failure("missing subject");
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
    std::string name;
    getline(f,name);
    utime(filename.c_str(),NULL);
    return name;
  };
  // Look for unused names
  // Get full list first.
  std::list<std::string> names;
  {
    std::ifstream f((dir_+"pool").c_str());
    if(!f.is_open()) failure("can't open pool file")
    std::string name;
    while(!f.eof()) {
      std::getline(f,name);
      if(!f.fail()) break;
      if(name.empty()) continue;
      names.push_back(name);
    };
  };
  if(!names.size()) failure("pool is empty");
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
      if(!strcmp(file->d_name,".")) continue;
      if(!strcmp(file->d_name,"..")) continue;
      if(!strcmp(file->d_name,"pool")) continue;
      std::string filename = dir_+file->d_name;
      struct stat st;
      if(stat(filename.c_str(),&st) != 0) continue;
      if(!S_ISREG(st.st_mode)) continue;
      std::ifstream f(filename.c_str());
      if(!f.is_open()) { // trash in directory
        closedir(dir); failure("can't open one of mapping files"); 
      };
      std::string name;
      std::getline(f,name);
      // find this name in list
      std::list<std::string>::iterator i = names.begin();
      for(;i!=names.end();++i) if(*i == name) break;
      if(i == names.end()) {
        // Always try to destroy old mappings without corresponding 
        // entry in the pool file
        if(((unsigned int)(time(NULL) - st.st_mtime)) >= SELFUNMAP_TIME) {
          unlink(filename.c_str());
        };
      } else {
        names.erase(i);
        if( (oldmap_name.length() == 0) ||
            (((int)(oldmap_time - st.st_mtime)) > 0) ) {
          oldmap_name=name;
          oldmap_subject=file->d_name;
          oldmap_time=st.st_mtime;
        };
      };
    };
    closedir(dir);
  };
  if(names.size()) {
    // Claim one of unused names
    std::ofstream f(filename.c_str());
    if(!f.is_open()) failure("can't create mapping file");
    f<<*(names.begin())<<std::endl;
    info(std::string("Mapped ")+subject+" to "+(*(names.begin())));
    return *(names.begin());
  };
  // Try to release one of old names
  if(oldmap_name.length() == 0) failure("no old mappings found");
  if(((unsigned int)(time(NULL) - oldmap_time)) < SELFUNMAP_TIME) failure("no old enough mappings found");
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

bool SimpleMap::unmap(const std::string& subject) {
  if(pool_handle_ == -1) return false;
  FileLock lock(pool_handle_);
  if(!lock) return false;
  if(unlink((dir_+subject).c_str()) == 0) return true;
  if(errno == ENOENT) return true;
  return false;
}

} 
