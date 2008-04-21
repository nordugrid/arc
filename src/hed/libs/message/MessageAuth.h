#ifndef __ARC_MESSAGEAUTH_H__
#define __ARC_MESSAGEAUTH_H__

#include <string>
#include <map>

#include "SecAttr.h"

namespace Arc {

/// Contains authencity information, authorization tokens and decisions.
/** This class only supports string keys and SecAttr values. */
class MessageAuth {
  private:
    std::map<std::string,SecAttr*> attrs_;
  public:
    MessageAuth(void);
    ~MessageAuth(void);
    void set(const std::string& key, SecAttr* value);
    void remove(const std::string& key);
    SecAttr* get(const std::string& key);
    SecAttr* operator[](const std::string& key) { return get(key); }; 
};

}

#endif

