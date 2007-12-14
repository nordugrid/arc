#ifndef __ARC_SEC_ALLOWPDP_H__
#define __ARC_SEC_ALLOWPDP_H__

#include <stdlib.h>

#include <arc/ArcConfig.h>
#include <arc/security/PDP.h>

namespace ArcSec {

/// This PDP always return true (allow)
class AllowPDP : public PDP {
 public:
  static PDP* get_allow_pdp(Arc::Config *cfg, Arc::ChainContext *ctx);
  AllowPDP(Arc::Config* cfg);
  virtual ~AllowPDP() {};
  virtual bool isPermitted(Arc::Message *msg);
};

} // namespace ArcSec

#endif /* __ARC_SEC_ALLOWPDP_H__ */
