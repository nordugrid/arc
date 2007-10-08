#ifndef __ARC_SEC_SIMPLEPDP_H__
#define __ARC_SEC_SIMPLEPDP_H__

#include <stdlib.h>

#include <arc/ArcConfig.h>
#include <arc/security/PDP.h>

namespace ArcSec {

class SimpleListPDP : public PDP {
 public:
  static PDP* get_simplelist_pdp(Arc::Config *cfg, Arc::ChainContext *ctx);
  SimpleListPDP(Arc::Config* cfg);
  virtual ~SimpleListPDP() {};
  virtual bool isPermitted(Arc::Message *msg);
 private:
  std::string location;
};

} // namespace ArcSec

#endif /* __ARC_SEC_SIMPLELISTPDP_H__ */
