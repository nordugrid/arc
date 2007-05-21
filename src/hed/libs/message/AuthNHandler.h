#ifndef __ARC_AUTHNHANDLER_H__
#define __ARC_AUTHNHANDLER_H__

#include <stdlib.h>

#include "common/ArcConfig.h"
#include "MCC.h"
#include "Message.h"

namespace Arc {

class AuthNHandler {
 public:
  AuthNHandler(Arc::Config* cfg) { };
  virtual ~AuthNHandler(void) { };
  virtual MCC_Status process(Message& msg) { return -1; };
};

} // namespace Arc

#endif /* __ARC_AUTHNHANDLER_H__ */

