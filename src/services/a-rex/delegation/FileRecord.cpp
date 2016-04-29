#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm.h>

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

} // namespace ARex

