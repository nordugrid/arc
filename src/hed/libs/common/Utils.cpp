// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_GLIBMM_GETENV
#include <glibmm/miscutils.h>
#else
#include <stdlib.h>
#endif

#ifdef HAVE_GLIBMM_SETENV
#include <glibmm/miscutils.h>
#else
#include <stdlib.h>
#endif

#ifdef HAVE_GLIBMM_UNSETENV
#include <glibmm/miscutils.h>
#else
#include <stdlib.h>
#endif

#include <glibmm/module.h>
#include <glibmm/fileutils.h>

#include <string.h>

#include <arc/ArcLocation.h>
#include <arc/StringConv.h>
#include <arc/Thread.h>

#ifndef BUFLEN
#define BUFLEN 1024
#endif

namespace Arc {

  std::string GetEnv(const std::string& var) {
#ifdef HAVE_GLIBMM_GETENV
    return Glib::getenv(var);
#else
    char val* = getenv(var.c_str());
    return val ? val : "";
#endif
  }

  std::string GetEnv(const std::string& var, bool &found) {
#ifdef HAVE_GLIBMM_GETENV
    return Glib::getenv(var, found);
#else
    char val* = getenv(var.c_str());
    found = (val != NULL)
    return val ? val : "";
#endif
  }

  bool SetEnv(const std::string& var, const std::string& value) {
#ifdef HAVE_GLIBMM_SETENV
    return Glib::setenv(var, value);
#else
#ifdef HAVE_SETENV
    return (setenv(var.c_str(), value.c_str(), 1) == 0);
#else
    return (putenv(strdup((var + "=" + value).c_str())) == 0);
#endif
#endif
  }

  void UnsetEnv(const std::string& var) {
#ifdef HAVE_GLIBMM_UNSETENV
    Glib::unsetenv(var);
#else
#ifdef HAVE_UNSETENV
    unsetenv(var.c_str());
#else
    putenv(strdup(var.c_str()));
#endif
#endif
  }

  std::string StrError(int errnum) {
#ifdef HAVE_STRERROR_R
    char errbuf[BUFLEN];
#ifdef STRERROR_R_CHAR_P
    return strerror_r(errnum, errbuf, sizeof(errbuf));
#else
    if (strerror_r(errnum, errbuf, sizeof(errbuf)) == 0)
      return errbuf;
    else
      return "Unknown error " + tostring(errnum);
#endif
#else
    return strerror(errnum);
#endif
  }
  
  static Glib::Mutex persistent_libraries_lock;
  static std::list<std::string> persistent_libraries_list;

  bool PersistentLibraryInit(const std::string& name) {
    std::string arc_lib_path = ArcLocation::Get();
    if(!arc_lib_path.empty()) {
#ifndef WIN32
      arc_lib_path = arc_lib_path + G_DIR_SEPARATOR_S + LIBSUBDIR;
#else
      arc_lib_path = arc_lib_path + G_DIR_SEPARATOR_S + "bin";
#endif
    }
#ifndef WIN32
    std::string libpath = Glib::build_filename(arc_lib_path,"lib"+name+"."+G_MODULE_SUFFIX);
#else
    std::string libpath = Glib::build_filename(arc_lib_path,"lib"+name+"-0."+G_MODULE_SUFFIX);
#endif

    // MacOSX suffix is "dylib" and not "so" (G_MODULE_SUFFIX macro content "so" under MacOSX, maybe this is a glib bug)

    if (!Glib::file_test(libpath, Glib::FILE_TEST_EXISTS)) {
       // this should happen only if G_MODULE_SUFFIX is not "dll" or not "so" 
       libpath = Glib::build_filename(arc_lib_path,"lib"+name+".dylib");
    }   

    // std::cout << "PersistentLibraryInit libpath: " << libpath << std::endl;

    persistent_libraries_lock.lock();
    for(std::list<std::string>::iterator l = persistent_libraries_list.begin();
            l != persistent_libraries_list.end();++l) {
      if(*l == libpath) {
        persistent_libraries_lock.unlock();
        return true;
      };
    };
    persistent_libraries_lock.unlock();
    Glib::Module *module = new Glib::Module(libpath);
    if(module && (*module)) {
      persistent_libraries_lock.lock();
      persistent_libraries_list.push_back(libpath);
      persistent_libraries_lock.unlock();
      return true;
    };
    if(module) delete module;
    return false;
  }

} // namespace Arc
