#ifndef __ARC_SIMPLEPDP_H__
#define __ARC_SIMPLEPDP_H__

#include <stdlib.h>

#include <arc/ArcConfig.h>
#include <arc/security/PDP.h>

namespace Arc {

class SimpleListPDP : public PDP {
 public:
  static PDP* get_simplelist_pdp(Config *cfg, ChainContext *ctx);
  SimpleListPDP(Config* cfg);
  virtual ~SimpleListPDP() {};
  virtual bool isPermitted(std::string subject);
 private:
  std::string location;
};

} // namespace Arc

#endif /* __ARC_SIMPLELISTPDP_H__ */
