#ifndef __ARC_PASSWORD_SOURCE_H__
#define __ARC_PASSWORD_SOURCE_H__

#include <string>

namespace Arc {

class PasswordSource {
 public:
  typedef enum {
    NO_PASSWORD = 0, // No password is returned. Authoritative. Not same as empty password.
    PASSWORD = 1,     // Password is  provided. Authoritative.
    CANCEL = 2       // Request to cancel procedure which need password.
  } Result;
  virtual Result Get(std::string& password, int minsize, int maxsize) = 0;
  virtual ~PasswordSource(void) { };
};

class PasswordSourceNone: public PasswordSource {
 public:
  virtual Result Get(std::string& password, int minsize, int maxsize);
};

class PasswordSourceString: public PasswordSource {
 private:
  std::string password_;
 public:
  PasswordSourceString(const std::string& password);
  virtual Result Get(std::string& password, int minsize, int maxsize);
};

class PasswordSourceStream: public PasswordSource {
 private:
  std::istream* password_;
 public:
  PasswordSourceStream(std::istream* password);
  virtual Result Get(std::string& password, int minsize, int maxsize);
};

class PasswordSourceInteractive: public PasswordSource {
 private:
  std::string prompt_;
  bool verify_;
 public:
  PasswordSourceInteractive(const std::string& prompt, bool verify);
  virtual Result Get(std::string& password, int minsize, int maxsize);
};

} // namespace Arc

#endif // __ARC_PASSWORD_SOURCE_H__

