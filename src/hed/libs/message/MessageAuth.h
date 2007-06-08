#ifndef __ARC_MESSAGEAUTH_H__
#define __ARC_MESSAGEAUTH_H__

#include <stdlib.h>

#include "Message.h"

namespace Arc {

typedef std::string AuthObject;

/** Class MessageAuth will contain authencity information, 
  authorization tokens and decisions. */
class MessageAuth {
  private:
    std::map<std::string,std::list<AuthObject> > properties_;
  public:
    void set(const std::string& key,const AuthObject& value);
    AuthObject get(const std::string& key,int index = 0);
    void remove(const std::string& key);
};

} // namespace Arc

#endif /* __ARC_MESSAGEAUTH_H__ */

