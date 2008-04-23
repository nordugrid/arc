#ifndef __ARC_SEC_DENYPDP_H__
#define __ARC_SEC_DENYPDP_H__

#include <stdlib.h>

#include <arc/ArcConfig.h>
#include <arc/security/PDP.h>
#include <arc/loader/PDPLoader.h>

namespace ArcSec {

/// This PDP always returns false (deny)
class DenyPDP : public PDP {
 public:
  static PDP* get_deny_pdp(Arc::Config *cfg, Arc::ChainContext *ctx);
  DenyPDP(Arc::Config* cfg);
  virtual ~DenyPDP() {};
  virtual bool isPermitted(Arc::Message *msg);
};

} // namespace ArcSec

#endif /* __ARC_SEC_DENYPDP_H__ */
