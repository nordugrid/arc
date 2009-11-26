// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef WIN32
#include <signal.h>
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

#ifndef WIN32
  class SIGPIPEIngore {
  private:
    sighandler_t sighandler_;
  public:
    SIGPIPEIngore(void);
    ~SIGPIPEIngore(void);
  };

  SIGPIPEIngore::SIGPIPEIngore(void) {
    sighandler_ = signal(SIGPIPE,SIG_IGN);
  }

  SIGPIPEIngore::~SIGPIPEIngore(void) {
    signal(SIGPIPE,sighandler_);
  }

  static SIGPIPEIngore sigpipe_ignore;
#endif

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
#ifndef WIN32
    if(!arc_lib_path.empty()) 
      arc_lib_path = arc_lib_path + G_DIR_SEPARATOR_S + LIBSUBDIR;
#ifdef __APPLE__
    // According to "Mac OS X Reference Library" proper macro to detect 
    // compilation on Mac OS X is either __MACH__ or __APPLE__.
    // According to "Mac OS X for Unix geeks" Mac OS X suffix for 
    // _shared libraries_ is "dylib". While "so" and "bundle" are used for
    // "loadable modules". Hence G_MODULE_SUFFIX macro is not suitable
    // because it reads "so" under Mac OS X and that is proper suffix 
    // for _loadable modules_ but not for _shared libraries_. 
    std::string libpath = Glib::build_filename(arc_lib_path,"lib"+name+".dylib");
#else
    std::string libpath = Glib::build_filename(arc_lib_path,"lib"+name+"."+G_MODULE_SUFFIX);
#endif
#else
    if(!arc_lib_path.empty()) 
      arc_lib_path = arc_lib_path + G_DIR_SEPARATOR_S + "bin";
    std::string libpath = Glib::build_filename(arc_lib_path,"lib"+name+"-0."+G_MODULE_SUFFIX);
#endif

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
