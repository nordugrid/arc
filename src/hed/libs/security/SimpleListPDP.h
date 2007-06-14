#ifndef __ARC_SIMPLEPDP_H__
#define __ARC_SIMPLEPDP_H__

#include <stdlib.h>

#include "common/ArcConfig.h"
#include "../message/Message.h"

namespace Arc {

class SimpleListPDP : public PDP {
 public:
  SimpleListPDP(Arc::Config* cfg);
  virtual ~SimpleListPDP(void) { };
  virtual bool isPermitted(std::string subject); 
 private:
  std::string location;
};

} // namespace Arc

#endif /* __ARC_SIMPLELISTPDP_H__ */

