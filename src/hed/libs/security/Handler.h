#ifndef __ARC_HANDLER_H__
#define __ARC_HANDLER_H__

#include <stdlib.h>

#include "common/ArcConfig.h"
#include "../message/Message.h"

namespace Arc {

class Handler {
 public:
  Handler(Arc::Config* cfg) { };
  virtual ~Handler(void) { };
  
  virtual void *MakePDP(Arc::Config* cfg){};
  virtual bool SecHandle(Message* msg){};
};

} // namespace Arc

#endif /* __ARC_HANDLER_H__ */

