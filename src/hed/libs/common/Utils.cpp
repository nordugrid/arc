// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <signal.h>

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

#include "Utils.h"

#ifndef BUFLEN
#define BUFLEN 1024
#endif

// If defined this turn on additional layer in handling
// setenv() which tries to avoid memory leaks.
// Windows uses different way to store environment. TODO: investigate
#define TRICKED_ENVIRONMENT
extern char** environ;

namespace Arc {

  class SIGPIPEIngore {
  private:
    void (*sighandler_)(int);
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

#ifdef TRICKED_ENVIRONMENT

  // This class is not protected by mutexes because its methods
  // are always called with SetEnv mutex locked.
  class TrickEnvRecord {
  private:
    static const unsigned int alloc_size = 1024;
    static std::list<TrickEnvRecord> records_;
    std::string name_;
    //std::string value_;
    char* ptr_;
    unsigned int size_; // max value to fit into allocated space
    bool unset_;
    TrickEnvRecord(void);
    TrickEnvRecord(const std::string& name);
  public:
    bool Set(const std::string& value);
    bool Unset(void);
    static bool Set(const std::string& name, const std::string& value, bool overwrite);
    static bool Unset(const std::string& name);
  };

  std::list<TrickEnvRecord> TrickEnvRecord::records_;

  TrickEnvRecord::TrickEnvRecord(const std::string& name):
                         name_(name),ptr_(NULL),size_(0),unset_(false) {
  }

  bool TrickEnvRecord::Set(const std::string& value) {
    char* curval = getenv(name_.c_str());
    if(ptr_ && curval && (curval == ptr_)) {
      // Still same memory is active. Can modify content.
      if(value.length() <= size_) {
        // Enough space to store new value
        memcpy(ptr_,value.c_str(),value.length());
        memset(ptr_+value.length(),0,size_-value.length()+1);
        return true;
      };
    };
    unsigned int newsize = 0;
    char* newrec = NULL;
    bool allocated = false;
    if(unset_ && ptr_ && (value.length() <= size_)) {
      // Unset buffer can be reused
      newsize = size_; size_ = 0;
      newrec = ptr_-(name_.length()+1); ptr_ = NULL;
    } else {
      // Allocate new memory
      newsize = (value.length()/alloc_size+1)*alloc_size;
      newrec = (char*) ::malloc(name_.length()+1+newsize+1);
      allocated = true;
    };
    if(newrec) {
      memcpy(newrec,name_.c_str(),name_.length());
      newrec[name_.length()] = '=';
      memcpy(newrec+name_.length()+1,value.c_str(),value.length());
      memset(newrec+name_.length()+1+value.length(),0,newsize-value.length()+1);
      if(::putenv(newrec) == 0) {
        // New record stored
        ptr_ = newrec+name_.length()+1;
        size_ = newsize;
        //value_ = value;
        unset_ = false;
        return true;
      } else {
        if(allocated) {
          free(newrec); newrec = NULL;
        };
      };
    };
    // Failure
    return false;
  }

  bool TrickEnvRecord::Set(const std::string& name, const std::string& value, bool overwrite) {
    if(!overwrite) {
      if(getenv(name.c_str())) return false;
    };
    for(std::list<TrickEnvRecord>::iterator r = records_.begin();
                                                r != records_.end(); ++r) {
      if(r->name_ == name) { // TODO: more optimal search
        return r->Set(value);
      };
    };
    // No such record - making new
    TrickEnvRecord rec(name);
    if(!rec.Set(value)) return false;
    records_.push_back(rec);
    return true;
  }

  bool TrickEnvRecord::Unset(void) {
    unset_ = true;
#ifdef HAVE_UNSETENV
    unsetenv(name_.c_str());
    if(ptr_) *(ptr_-1) = 0; // Removing '='
#else
    // Reusing buffer
    if(ptr_) {
      *(ptr_-1) = 0;
      putenv(ptr_-(name_.length()+1));
    } else {
      return false; // Never happens
    };
#endif
    return true;
  }

  bool TrickEnvRecord::Unset(const std::string& name) {
    for(std::list<TrickEnvRecord>::iterator r = records_.begin();
                                                r != records_.end(); ++r) {
      if(r->name_ == name) { // TODO: more optimal search
        return r->Unset();
      };
    };
#ifdef HAVE_UNSETENV
    unsetenv(name.c_str());
#else
    // Better solution is needed
    putenv(strdup(name.c_str()));
#endif
    return true;
  }
#endif // TRICKED_ENVIRONMENT


  // Below is a set of mutexes for protecting environment
  // variables. Current implementation is very simplistic.
  // There are 2 mutexes. 'env_read_lock' protects any 
  // access to the pool of environment variables. It is
  // exposed to outside through EnvLockWrap() and EnvLockUnwrap()
  // functions. Any third party code doing setenv()/getenv()
  // must be wrapped using those functions. GetEnv() and SetEnv()
  // functions of this API are using that lock internally.
  // Second mutex 'env_write_lock' is meant for protecting
  // set of environment variables from modification. It
  // is only supposed to be used outside this library
  // for protecting access to external libraries using
  // environment variables as input parameters. That mutex
  // is exposed through EnvLockAcquire() and EnvLockRelease().
  // Those functions do not acquire 'env_lock_read' mutex.
  // Hence proper usage of this infrastructure requires
  // one to acquire both mutexes. See example below:
  //   EnvLockAcquire();
  //   SetEnv("ARG","VAL"); // Setting variable needed for ext. library
  //   EnvLockWrap();
  //   func(); // Calling ext function which uses getenv()
  //   EnvLockUnwrap();
  //   EnvLockRelease();
  // Current implementation locks too many resources and has
  // negative performance impact. In a future (unless there
  // will be no need for all that at all) EnvLockAcquire will
  // must provide lock per variable. And EnvLockWrap must 
  // provide different functionality depending on setenv() or
  // getenv() is going to be wrapped.

  // The purpose of this mutex is to 'solve' problem with
  // some third party libraries which use environment variables
  // as input arguments :(
  static Glib::Mutex& env_write_lock(void) {
    static Glib::Mutex* mutex = new Glib::Mutex;
    return *mutex;
  }

  // And this mutex is needed because it seems like none if
  // underlying functions provide proper thread protection  
  // of environment variables. Also calls to external libraries
  // using getenv() need to be protected by this lock.
  static SharedMutex& env_read_lock(void) {
    static SharedMutex* mutex = new SharedMutex;
    return *mutex;
  }

  class SharedMutexExclusive {
   private:
    SharedMutex& lock_;
   public:
    SharedMutexExclusive(SharedMutex& lock):lock_(lock) {
      lock_.lockExclusive();
    };
    ~SharedMutexExclusive(void) {
      lock_.unlockExclusive();
    };
  };

  class SharedMutexShared {
   private:
    SharedMutex& lock_;
   public:
    SharedMutexShared(SharedMutex& lock):lock_(lock) {
      lock_.lockShared();
    };
    ~SharedMutexShared(void) {
      lock_.unlockShared();
    };
  };

  std::string GetEnv(const std::string& var) {
    SharedMutexShared env_lock(env_read_lock());
#ifdef HAVE_GLIBMM_GETENV
    return Glib::getenv(var);
#else
    char* val = getenv(var.c_str());
    return val ? val : "";
#endif
  }

  std::string GetEnv(const std::string& var, bool &found) {
    SharedMutexShared env_lock(env_read_lock());
#ifdef HAVE_GLIBMM_GETENV
    return Glib::getenv(var, found);
#else
    char* val = getenv(var.c_str());
    found = (val != NULL);
    return val ? val : "";
#endif
  }

  std::list<std::string> GetEnv(void) {
    SharedMutexShared env_lock(env_read_lock());
#if defined(HAVE_GLIBMM_LISTENV) && defined(HAVE_GLIBMM_GETENV)
    std::list<std::string> envp = Glib::listenv();
    for(std::list<std::string>::iterator env = envp.begin(); 
                         env != envp.end(); ++env) {
      *env = *env + "=" + Glib::getenv(*env);
    };
    return envp;
#else
    std::list<std::string> envp;
    for(char** enventry = environ; *enventry; ++enventry) {
      envp.push_back(*enventry);
    };
    return envp;
#endif
  }

  bool SetEnv(const std::string& var, const std::string& value, bool overwrite) {
    SharedMutexExclusive env_lock(env_read_lock());
#ifndef TRICKED_ENVIRONMENT
#ifdef HAVE_GLIBMM_SETENV
    return Glib::setenv(var, value, overwrite);
#else
#ifdef HAVE_SETENV
    return (setenv(var.c_str(), value.c_str(), overwrite) == 0);
#else
    return (putenv(strdup((var + "=" + value).c_str())) == 0);
#endif
#endif
#else // TRICKED_ENVIRONMENT
    return TrickEnvRecord::Set(var, value, overwrite);
#endif // TRICKED_ENVIRONMENT
  }

  void UnsetEnv(const std::string& var) {
    SharedMutexExclusive env_lock(env_read_lock());
#ifndef TRICKED_ENVIRONMENT
#ifdef HAVE_GLIBMM_UNSETENV
    Glib::unsetenv(var);
#else
#ifdef HAVE_UNSETENV
    unsetenv(var.c_str());
#else
    putenv(strdup(var.c_str()));
#endif
#endif
#else // TRICKED_ENVIRONMENT
    // This is compromise and will not work if third party
    // code distinguishes between empty and unset variable.
    // But without this pair of setenv/unsetenv will 
    // definitely leak memory.
    TrickEnvRecord::Unset(var);
#endif // TRICKED_ENVIRONMENT
  }

  void EnvLockAcquire(void) {
    env_write_lock().lock();
  }

  void EnvLockRelease(void) {
    env_write_lock().unlock();
  }

  void EnvLockWrap(bool all) {
    if(all) {
      env_read_lock().lockExclusive();
    } else {
      env_read_lock().lockShared();
    }
  }

  void EnvLockUnwrap(bool all) {
    if(all) {
      env_read_lock().unlockExclusive();
    } else {
      env_read_lock().unlockShared();
    }
  }

  void EnvLockUnwrapComplete(void) {
    // This function is deprecated and its body removed because
    // there is no safe way to reset locks after call to fork().
  }

  static Glib::Mutex signal_lock;

  InterruptGuard::InterruptGuard() {
    signal_lock.lock();
    saved_sigint_handler = signal(SIGINT, SIG_IGN);
  }

  InterruptGuard::~InterruptGuard() {
    signal(SIGINT, saved_sigint_handler);
    signal_lock.unlock();
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
#warning USING THREAD UNSAFE strerror(). UPGRADE YOUR libc.
    return strerror(errnum);
#endif
  }
  
  static Glib::Mutex persistent_libraries_lock;
  static std::list<std::string> persistent_libraries_list;

  bool PersistentLibraryInit(const std::string& name) {
    // Library is made persistent by loading intermediate
    // module which depends on that library. So passed name
    // is name of that module. Modules usually reside in 
    // ARC_LOCATION/lib/arc. This approach is needed because
    // on some platforms shared libraries can't be dlopen'ed.
    std::string arc_lib_path = ArcLocation::Get();
    if(!arc_lib_path.empty()) 
      arc_lib_path = arc_lib_path + G_DIR_SEPARATOR_S + PKGLIBSUBDIR;
    std::string libpath = Glib::build_filename(arc_lib_path,"lib"+name+"."+G_MODULE_SUFFIX);

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
