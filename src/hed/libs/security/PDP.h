#ifndef __ARC_PDP_H__
#define __ARC_PDP_H__

#include <stdlib.h>

#include "common/ArcConfig.h"
#include "../message/Message.h"
#include "../../../libs/common/Logger.h"

namespace Arc {

class PDP {
 public:
  PDP(Arc::Config* cfg) { };
  virtual ~PDP(void) { };
  virtual bool isPermitted(std::string subject) {};
 protected:
  static Logger logger;
};

} // namespace Arc

#endif /* __ARC_PDP_H__ */

