#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glibmm.h>
#include <arc/FileUtils.h>

#include "FileRecord.h"

namespace ARex {

  std::string FileRecord::uid_to_path(const std::string& uid) {
    std::string path = basepath_;
    std::string::size_type p = 0;
    for(;uid.length() > (p+4);) {
      path = path + G_DIR_SEPARATOR_S + uid.substr(p,3);
      p += 3;
    };
    return path + G_DIR_SEPARATOR_S + uid.substr(p);
  }

  bool FileRecord::make_file(const std::string& uid) {
    std::string path = uid_to_path(uid);
    std::string::size_type p = path.rfind(G_DIR_SEPARATOR_S);
    if((p != std::string::npos) && (p != 0)) {
      (void)Arc::DirCreate(path.substr(0,p),0,0,S_IXUSR|S_IRUSR|S_IWUSR,true);
    }
    return Arc::FileCreate(uid_to_path(uid),"",0,0,S_IRUSR|S_IWUSR);
  }

  bool FileRecord::remove_file(const std::string& uid) {
    std::string path = uid_to_path(uid);
    if(Arc::FileDelete(path)) {
      while(true) {
        std::string::size_type p = path.rfind(G_DIR_SEPARATOR_S);
        if((p == std::string::npos) || (p == 0)) break;
        if(p <= basepath_.length()) break;
        path.resize(p);
        if(!Arc::DirDelete(path,false)) break;
      };
      return true;
    };
    return false;
  }

} // namespace ARex

