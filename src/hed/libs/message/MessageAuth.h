#ifndef __ARC_MESSAGEAUTH_H__
#define __ARC_MESSAGEAUTH_H__

#include <string>
#include <map>

#include <arc/message/SecAttr.h>

namespace Arc {

/// Contains authencity information, authorization tokens and decisions.
/** This class only supports string keys and SecAttr values. */
class MessageAuth {
  private:
    std::map<std::string,SecAttr*> attrs_;
  public:
    MessageAuth(void);
    ~MessageAuth(void);
    /// Adds/overwrites security attribute stored under specified key
    void set(const std::string& key, SecAttr* value);
    /// Deletes security attribute stored under specified key
    void remove(const std::string& key);
    /// Retrieves reference to security attribute stored under specified key
    SecAttr* get(const std::string& key);
    /// Same as MessageAuth::get
    SecAttr* operator[](const std::string& key) { return get(key); }; 
    /// Returns properly catenated attributes in specified format
    bool Export(SecAttr::Format format,XMLNode &val) const;
};

}

#endif

