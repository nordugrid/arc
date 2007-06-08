#ifndef __ARC_PDP_H__
#define __ARC_PDP_H__

#include <stdlib.h>

#include "common/ArcConfig.h"
#include "MCC.h"
#include "Message.h"

namespace Arc {

class PDP {
 public:
  PDP(Arc::Config* cfg) { };
  virtual ~PDP(void) { };
  virtual MCC_Status process(Message& msg) { MCC_Status(); };
};

} // namespace Arc

#endif /* __ARC_PDP_H__ */

