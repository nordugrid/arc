#ifndef __ARC_PDP_H__
#define __ARC_PDP_H__

#include <stdlib.h>

#include "common/ArcConfig.h"
#include "../message/Message.h"

namespace Arc {

class PDP {
 public:
  PDP(Arc::Config* cfg) { };
  virtual ~PDP(void) { };
  virtual bool isPermitted(std::string subject) {};
};

} // namespace Arc

#endif /* __ARC_PDP_H__ */

