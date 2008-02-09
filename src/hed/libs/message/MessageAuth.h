#ifndef __ARC_MESSAGEAUTH_H__
#define __ARC_MESSAGEAUTH_H__

#include "MessageAttributes.h"

namespace Arc {

/// Contains authencity information, authorization tokens and decisions.
/** Currently this class only supports string keys and string values and is 
  to MessageAttributes. So far it's separation from MessageAttributes is 
  purely for convenience reasons. */
class MessageAuth: public MessageAttributes {
  public:
    MessageAuth(void) { };
    ~MessageAuth(void) { };
};

} // namespace Arc

#endif /* __ARC_MESSAGEAUTH_H__ */

