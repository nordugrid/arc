// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_OPENSSL_H__
#define __ARC_OPENSSL_H__

namespace Arc {
  /// This module contains various convenience utilities for using OpenSSL
  /** Application may be linked to this module instead of OpenSSL libraries
     directly. */

  /// This function initializes OpenSSL library.
  /** It may be called multiple times and makes sure everything is 
     done properly and OpenSSL may be used in multi-threaded environment.
     Because this function makes use of ArcLocation it is advisable
     to call it after ArcLocation::Init(). */
  bool OpenSSLInit(void);

  /// Prints chain of accumulaed OpenSSL errors if any available
  void HandleOpenSSLError(void);

  /// Prints chain of accumulaed OpenSSL errors if any available
  void HandleOpenSSLError(int code);

  int OpenSSLAppDataIndex(const std::string& id);

} // namespace Arc

#endif /* __ARC_OPENSSL_H__ */

