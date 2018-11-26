// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <arc/Thread.h>
#include <arc/Logger.h>
#include <arc/Utils.h>

#include "OpenSSL.h"

#include <unistd.h>

namespace Arc {

  static Glib::Mutex lock;
  static bool initialized = false;
  static Glib::Mutex* ssl_locks = NULL;
  static int ssl_locks_num = 0;
  static std::map<std::string,int> app_data_indices;

  static Logger& logger(void) {
    static Logger* logger_ = new Logger(Logger::getRootLogger(), "OpenSSL");
    return *logger_;
  }

  // This class takes care of cleaning OpenSSL data stored per-thread.
  // Here assumption is that every thread dealing with OpenSSL either 
  // calls OpenSSLInit or is started by thread which called OpenSSLInit.
  class OpenSSLThreadCleaner: private ThreadDataItem {
   public:
    OpenSSLThreadCleaner(void);
    virtual ~OpenSSLThreadCleaner(void);
    virtual void Dup(void);
  };

  OpenSSLThreadCleaner::OpenSSLThreadCleaner(void):ThreadDataItem("arc.openssl.thread.cleaner") {
  }

  OpenSSLThreadCleaner::~OpenSSLThreadCleaner(void) {
#if (OPENSSL_VERSION_NUMBER < 0x10100000L)
    ERR_remove_state(0);
#endif
  }

  void OpenSSLThreadCleaner::Dup(void) {
    new OpenSSLThreadCleaner;
  }

  void HandleOpenSSLError(void) {
    HandleOpenSSLError(SSL_ERROR_NONE);
  }

  void HandleOpenSSLError(int code) {
    unsigned long e = (code==SSL_ERROR_NONE)?ERR_get_error():code;
    while(e != SSL_ERROR_NONE) {
      if(e == SSL_ERROR_SYSCALL) {
        // Hiding system errors
        //logger().msg(DEBUG, "SSL error: %d - system call failed",e);
      } else {
        const char* lib = ERR_lib_error_string(e);
        const char* func = ERR_func_error_string(e);
        const char* reason = ERR_reason_error_string(e);
        logger().msg(DEBUG, "SSL error: %d - %s:%s:%s",
                          e,
                          lib?lib:"",
                          func?func:"",
                          reason?reason:"");
      };
      e = ERR_get_error();
    };
  }

#if (OPENSSL_VERSION_NUMBER < 0x10100000L)
  static void ssl_locking_cb(int mode, int n, const char * s_, int n_){
    if(!ssl_locks) {
      logger().msg(FATAL, "SSL locks not initialized");
      _exit(-1);
    };
    if((n < 0) || (n >= ssl_locks_num)) {
      logger().msg(FATAL, "wrong SSL lock requested: %i of %i: %i - %s",n,ssl_locks_num,n_,s_);
      _exit(-1);
    };
    if(mode & CRYPTO_LOCK) {
      ssl_locks[n].lock();
    } else {
      ssl_locks[n].unlock();
    };
  }

  static unsigned long ssl_id_cb(void) {
#ifdef WIN32
    return (unsigned long)(GetCurrentThreadId());
#else
    return (unsigned long)(Glib::Thread::self());
#endif
  }

  //static void* ssl_idptr_cb(void) {
  //  return (void*)(Glib::Thread::self());
  //}
#endif

  bool OpenSSLInit(void) {
    Glib::Mutex::Lock flock(lock);
    if(!initialized) {
      if(!PersistentLibraryInit("modcrypto")) {
        logger().msg(WARNING, "Failed to lock arccrypto library in memory");
      };
#if (OPENSSL_VERSION_NUMBER < 0x10100000L)
      SSL_load_error_strings();
      if(!SSL_library_init()){
        logger().msg(ERROR, "Failed to initialize OpenSSL library");
        HandleOpenSSLError();
        ERR_free_strings();
        return false;
      };
#else
      if(!OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS |
                           OPENSSL_INIT_LOAD_CRYPTO_STRINGS |
                           OPENSSL_INIT_ADD_ALL_CIPHERS |
                           OPENSSL_INIT_ADD_ALL_DIGESTS |
#if (OPENSSL_VERSION_NUMBER < 0x10101000L)
                           OPENSSL_INIT_NO_LOAD_CONFIG,
#else
                           OPENSSL_INIT_LOAD_CONFIG,
#endif
                           NULL)) {
        logger().msg(ERROR, "Failed to initialize OpenSSL library");
        HandleOpenSSLError();
        return false;
      };
#endif
      // We could RAND_seed() here. But since 0.9.7 OpenSSL
      // knows how to make use of OS specific source of random
      // data. I think it's better to let OpenSSL do a job.
      // Here we could also generate ephemeral DH key to avoid
      // time consuming genaration during connection handshake.
      // But is not clear if it is needed for curently used
      // connections types at all. Needs further investigation.
      // Using RSA key violates TLS (according to OpenSSL
      // documentation) hence we do not use it.
      //  A.K.
    };
#if (OPENSSL_VERSION_NUMBER < 0x10100000L)
    // Always make sure our own locks are installed
    int num_locks = CRYPTO_num_locks();
    if(num_locks > 0) {
      if(num_locks != ssl_locks_num) {
        if(ssl_locks_num > 0) {
          logger().msg(ERROR, "Number of OpenSSL locks changed - reinitializing");
          ssl_locks_num=0;
          ssl_locks=NULL;
        };
      };
      if((!ssl_locks) || (!initialized)) {
        ssl_locks_num=0;
        ssl_locks=new Glib::Mutex[num_locks];
      };
      if(!ssl_locks) return false;
      ssl_locks_num=num_locks;
      CRYPTO_set_locking_callback(&ssl_locking_cb);
      CRYPTO_set_id_callback(&ssl_id_cb);
      //CRYPTO_set_idptr_callback(&ssl_idptr_cb);
    }
    if(!initialized) {
      OpenSSL_add_all_algorithms();
    }
#endif
    new OpenSSLThreadCleaner;
    initialized=true;
    return true;
  }


  int OpenSSLAppDataIndex(const std::string& id) {
    Glib::Mutex::Lock flock(lock);
    std::map<std::string,int>::iterator i = app_data_indices.find(id);
    if(i == app_data_indices.end()) {
      int n = SSL_CTX_get_ex_new_index(0,NULL,NULL,NULL,NULL);
      app_data_indices[id] = n;
      return n;
    }
    return i->second;
  }

} // namespace Arc

