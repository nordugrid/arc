#ifndef __ARC_SECHANDLER_H__
#define __ARC_SECHANDLER_H__

#include <stdlib.h>

#include "common/ArcConfig.h"
#include "../message/Message.h"
#include "../../../libs/common/Logger.h"

namespace Arc {

class SecHandler {
 public:
  SecHandler(Arc::Config* cfg) { };
  virtual ~SecHandler(void) { };
  virtual bool Handle(Message* msg){};
 protected:
  static Logger logger;
};

} // namespace Arc

#endif /* __ARC_SECHANDLER_H__ */

