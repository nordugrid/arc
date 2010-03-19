// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef WIN32
#include <arc/win32.h>
#endif

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <arc/Thread.h>
#include <arc/Logger.h>
#include <arc/Utils.h>

#include "OpenSSL.h"

namespace Arc {

  static Glib::Mutex lock;
  static bool initialized = false;
  static Glib::Mutex* ssl_locks = NULL;
  static int ssl_locks_num = 0;

  static Logger& logger(void) {
    static Logger* logger_ = new Logger(Logger::getRootLogger(), "OpenSSL");
    return *logger_;
  }

  void HandleOpenSSLError(void) {
    HandleOpenSSLError(SSL_ERROR_NONE);
  }

  void HandleOpenSSLError(int code) {
    unsigned long e = (code==SSL_ERROR_NONE)?ERR_get_error():code;
    while(e != SSL_ERROR_NONE) {
      if(e == SSL_ERROR_SYSCALL) {
        // Hiding system errors
        //logger().msg(ERROR, "SSL error: %d - system call failed",e);
      } else {
        const char* lib = ERR_lib_error_string(e);
        const char* func = ERR_func_error_string(e);
        const char* reason = ERR_reason_error_string(e);
        logger().msg(ERROR, "SSL error: %d - %s:%s:%s",
                          e,
                          lib?lib:"",
                          func?func:"",
                          reason?reason:"");
      };
      e = ERR_get_error();
    };
  }

  static void ssl_locking_cb(int mode, int n, const char * s_, int n_){
    if(!ssl_locks) {
      logger().msg(ERROR, "FATAL: SSL locks not initialized");
      _exit(-1);
    };
    if((n < 0) || (n >= ssl_locks_num)) {
      logger().msg(ERROR, "FATAL: wrong SSL lock requested: %i of %i: %i - %s",n,ssl_locks_num,n_,s_);
      _exit(-1);
    };
    if(mode & CRYPTO_LOCK) {
      ssl_locks[n].lock();
    } else {
      ssl_locks[n].unlock();
    };
  }

  static unsigned long ssl_id_cb(void) {
    return (unsigned long)(Glib::Thread::self());
  }

  //static void* ssl_idptr_cb(void) {
  //  return (void*)(Glib::Thread::self());
  //}

  bool OpenSSLInit(void) {
    Glib::Mutex::Lock flock(lock);
    if(!initialized) {
      if(!PersistentLibraryInit("modcrypto")) {
        logger().msg(WARNING, "Failed to lock arccrypto library in memory");
      };  
      SSL_load_error_strings();
      if(!SSL_library_init()){
        logger().msg(ERROR, "Failed to initialize OpenSSL library");
        HandleOpenSSLError();
        ERR_free_strings();
        return false;
      };
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
    initialized=true;
  }

} // namespace Arc

