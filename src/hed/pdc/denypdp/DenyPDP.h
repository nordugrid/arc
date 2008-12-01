#ifndef __ARC_SEC_DENYPDP_H__
#define __ARC_SEC_DENYPDP_H__

#include <stdlib.h>

#include <arc/ArcConfig.h>
#include <arc/security/PDP.h>

namespace ArcSec {

/// This PDP always returns false (deny)
class DenyPDP : public PDP {
 public:
  static Arc::Plugin* get_deny_pdp(Arc::PluginArgument* arg);
  DenyPDP(Arc::Config* cfg);
  virtual ~DenyPDP() {};
  virtual bool isPermitted(Arc::Message *msg);
};

} // namespace ArcSec

#endif /* __ARC_SEC_DENYPDP_H__ */
