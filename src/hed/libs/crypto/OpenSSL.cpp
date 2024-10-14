// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <openssl/crypto.h>
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

  bool OpenSSLInit(void) {
    Glib::Mutex::Lock flock(lock);
    if(!initialized) {
      if(!PersistentLibraryInit("modcrypto")) {
        logger().msg(WARNING, "Failed to lock arccrypto library in memory");
      };
      if(!OPENSSL_init_ssl(0, NULL)) { // Use all defaults, use system wide policies
        logger().msg(ERROR, "Failed to initialize OpenSSL library");
        HandleOpenSSLError();
        return false;
      };
      // We could RAND_seed() here. But since 0.9.7 OpenSSL
      // knows how to make use of OS specific source of random
      // data. I think it's better to let OpenSSL do a job.
      // Here we could also generate ephemeral DH key to avoid
      // time consuming genaration during connection handshake.
      // But is not clear if it is needed for currently used
      // connections types at all. Needs further investigation.
      // Using RSA key violates TLS (according to OpenSSL
      // documentation) hence we do not use it.
      //  A.K.
    };
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

