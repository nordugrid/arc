#ifndef __ARC_ARCPDP_H__
#define __ARC_ARCPDP_H__

#include <stdlib.h>

#include <arc/ArcConfig.h>
#include <arc/security/ArcPDP/Evaluator.h>
#include <arc/security/PDP.h>

namespace Arc {

class ArcPDP : public PDP {
 public:
  static PDP* get_arc_pdp(Config *cfg, ChainContext *ctx);
  ArcPDP(Config* cfg);
  virtual ~ArcPDP();
  virtual bool isPermitted(std::string subject);
 private:
  Evaluator *eval;
};

} // namespace Arc

#endif /* __ARC_ARCPDP_H__ */
