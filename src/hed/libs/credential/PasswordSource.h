#ifndef __ARC_PASSWORD_SOURCE_H__
#define __ARC_PASSWORD_SOURCE_H__

#include <string>

namespace Arc {

/** \addtogroup credential
 *  @{ */

/// Obtain password from some source
/**
 * Pure virtual class meant to be extended with a specific mechanism to obtain
 * password.
 * \since Added in 4.0.0.
 **/
class PasswordSource {
 public:
  typedef enum {
    /// No password is returned. Authoritative. Not same as empty password.
    NO_PASSWORD = 0,
    /// Password is  provided. Authoritative.
    PASSWORD = 1,
    /// Request to cancel procedure which need password.
    CANCEL = 2
  } Result;
  virtual Result Get(std::string& password, int minsize, int maxsize) = 0;
  virtual ~PasswordSource(void) { };
};

/// No password
class PasswordSourceNone: public PasswordSource {
 public:
  virtual Result Get(std::string& password, int minsize, int maxsize);
};

/// Obtain password from a string
class PasswordSourceString: public PasswordSource {
 private:
  std::string password_;
 public:
  PasswordSourceString(const std::string& password);
  virtual Result Get(std::string& password, int minsize, int maxsize);
};

/// Obtain password from stream
class PasswordSourceStream: public PasswordSource {
 private:
  std::istream* password_;
 public:
  PasswordSourceStream(std::istream* password);
  virtual Result Get(std::string& password, int minsize, int maxsize);
};

/// Obtain password through OpenSSL user interface
class PasswordSourceInteractive: public PasswordSource {
 private:
  std::string prompt_;
  bool verify_;
 public:
  PasswordSourceInteractive(const std::string& prompt, bool verify);
  virtual Result Get(std::string& password, int minsize, int maxsize);
};

/** @} */

} // namespace Arc

#endif // __ARC_PASSWORD_SOURCE_H__

