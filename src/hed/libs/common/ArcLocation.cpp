// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arc/Logger.h>
#include <arc/Utils.h>
#include <arc/StringConv.h>

#ifdef ENABLE_NLS
#include <libintl.h>
#endif

#include <glibmm.h>

#include "ArcLocation.h"

namespace Arc {

  std::string& ArcLocation::location(void) {
    static std::string* location_ = new std::string;
    return *location_;
  }

  static bool canonic_path(std::string& path) {
    bool absolute = ((path.length() > 0) && (path[0] == G_DIR_SEPARATOR));
    std::list<std::string> items;
    tokenize(path, items, G_DIR_SEPARATOR_S);
    for(std::list<std::string>::iterator pos = items.begin(); pos != items.end(); ) {
      if(pos->empty() || (*pos == ".")) {
        // useless part
        pos = items.erase(pos);
      } else if(*pos == "..") {
        if(pos  == items.begin()) return false;
        --pos;
        pos = items.erase(pos);
        pos = items.erase(pos);
      } else {
        ++pos;
      }
    }
    path.resize(0);
    for(std::list<std::string>::iterator pos = items.begin(); pos != items.end(); ) {
      if((pos != items.begin()) || absolute) path += G_DIR_SEPARATOR_S;
      path += *pos;
      ++pos;
    }
    return true;
  }

  static void array_deleter(char* array) {
    delete[] array;
  }

  static bool resolve_link(std::string& path) {
    struct stat linkst;
    ssize_t r;
    if(lstat(path.c_str(), &linkst) == -1) return false;
    if(!S_ISLNK(linkst.st_mode)) return false;
    AutoPointer<char> linkname(new char[linkst.st_size], &array_deleter);
    if(!linkname) return false;
    ssize_t linksize = readlink(path.c_str(), linkname.Ptr(), linkst.st_size);
    if(linksize > linkst.st_size) return false;
    if(linksize < 0) return false;
    if((linksize > 0) && (linkname.Ptr()[0] == G_DIR_SEPARATOR)) {
      // absolute link
      path.assign(linkname.Ptr(),linksize);
    } else {
      std::string::size_type sep = path.rfind(G_DIR_SEPARATOR);
      if(sep == std::string::npos)
        path.assign(linkname.Ptr(),linksize);
      else
        path.replace(sep+1,std::string::npos,linkname.Ptr(),linksize);
    }
    canonic_path(path);
    return true;
  }

  static std::string resolve_links(std::string const& path) {
    std::string full_path(path);
    canonic_path(full_path);
    std::string::size_type pos = 1;
    while(pos < full_path.length()) {
      std::string::size_type sep = full_path.find(G_DIR_SEPARATOR, pos);
      if(sep == std::string::npos) sep = full_path.length();
      std::string subpath = full_path.substr(0, sep);
      if(resolve_link(subpath)) {
        full_path.replace(0, sep, subpath);
        pos = 1;
      } else {
        pos = sep + 1;
      }
    }
    return full_path; 
  }

  void ArcLocation::Init(std::string path) {
    location().clear();
    location() = GetEnv("ARC_LOCATION");
    if (location().empty() && !path.empty()) {
      // Find full path if only name is given
      if (path.rfind(G_DIR_SEPARATOR_S) == std::string::npos)
        path = Glib::find_program_in_path(path);
      // Assign current dir if path is relative
      if (path.substr(0, 2) == std::string(".") + G_DIR_SEPARATOR_S) {
        std::string cwd = Glib::get_current_dir();
        path.replace(0, 1, cwd);
      }
      // Find out real path through all links
      path = resolve_links(path);
      // Strip off bin/tool part
      std::string::size_type pos = path.rfind(G_DIR_SEPARATOR_S);
      if (pos != std::string::npos && pos > 0) {
        pos = path.rfind(G_DIR_SEPARATOR_S, pos - 1);
        if (pos != std::string::npos) {
          if(pos == 0)
            location() = "/";  // Installed in root ?
          else
            location() = path.substr(0, pos);
        }
      }
    }
    if (location().empty()) {
      Logger::getRootLogger().msg(WARNING,
                                  "Can not determine the install location. "
                                  "Using %s. Please set ARC_LOCATION "
                                  "if this is not correct.", INSTPREFIX);
      location() = INSTPREFIX;
    }
#ifdef ENABLE_NLS
    bindtextdomain(PACKAGE, (location() + G_DIR_SEPARATOR_S "share" G_DIR_SEPARATOR_S "locale").c_str());
#endif
  }

  const std::string& ArcLocation::Get() {
    if (location().empty())
      Init("");
    return location();
  }


  std::list<std::string> ArcLocation::GetPlugins() {
    std::list<std::string> plist;
    std::string arcpluginpath = GetEnv("ARC_PLUGIN_PATH");
    if (!arcpluginpath.empty()) {
      std::string::size_type pos = 0;
      while (pos != std::string::npos) {
        std::string::size_type pos2 = arcpluginpath.find(G_SEARCHPATH_SEPARATOR, pos);
        plist.push_back(pos2 == std::string::npos ?
                        arcpluginpath.substr(pos) :
                        arcpluginpath.substr(pos, pos2 - pos));
        pos = pos2;
        if (pos != std::string::npos)
          pos++;
      }
    }
    else
      plist.push_back(Get() + G_DIR_SEPARATOR_S + PKGLIBSUBDIR);
    return plist;
  }

  std::string ArcLocation::GetDataDir() {
    if (location().empty()) Init("");
    return location() + G_DIR_SEPARATOR_S + PKGDATASUBDIR;
  }

  std::string ArcLocation::GetLibDir() {
    if (location().empty()) Init("");
    return location() + G_DIR_SEPARATOR_S + PKGLIBSUBDIR;
  }

  std::string ArcLocation::GetToolsDir() {
    if (location().empty()) Init("");
    return location() + G_DIR_SEPARATOR_S + PKGLIBEXECSUBDIR;
  }

} // namespace Arc
