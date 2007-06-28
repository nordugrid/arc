#ifndef __ARC_ARCPDP_H__
#define __ARC_ARCPDP_H__

#include <stdlib.h>

#include "common/ArcConfig.h"
#include "../message/Message.h"

namespace Arc {

class ArcPDP : public PDP {
 public:
  static PDP* get_arc_pdp(Config *cfg,ChainContext *ctx);
  ArcPDP(Arc::Config* cfg);
  virtual ~ArcPDP(void) { };
  virtual bool isPermitted(std::string subject);
 private:
  std::string location;
};

} // namespace Arc

#endif /* __ARC_ARCPDP_H__ */
