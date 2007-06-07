#ifndef __ARC_HANDLER_H__
#define __ARC_HANDLER_H__

#include <stdlib.h>

#include "common/ArcConfig.h"
#include "MCC.h"
#include "Message.h"

namespace Arc {

class Handler {
 public:
  Handler(Arc::Config* cfg) { };
  virtual ~Handler(void) { };
  virtual MCC_Status process(Message& msg) { return -1; };
};

} // namespace Arc

#endif /* __ARC_HANDLER_H__ */

