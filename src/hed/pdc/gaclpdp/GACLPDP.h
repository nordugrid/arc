#ifndef __ARC_SEC_GACLPDP_H__
#define __ARC_SEC_GACLPDP_H__

#include <arc/XMLNode.h>
#include <arc/ArcConfig.h>
#include <arc/security/PDP.h>
#include <arc/loader/Loader.h>
#include <arc/message/SecAttr.h>
/*
#include <stdlib.h>

#include <arc/ArcConfig.h>
#include <arc/security/ArcPDP/Evaluator.h>
*/

namespace ArcSec {


class GACLPDP: public PDP {
 public:
  static Arc::SecAttrFormat GACL;
  static PDP* get_gacl_pdp(Arc::Config *cfg, Arc::ChainContext *ctx);
  GACLPDP(Arc::Config* cfg);
  virtual ~GACLPDP();
  virtual bool isPermitted(Arc::Message *msg);
 private:
  std::list<std::string> select_attrs;
  std::list<std::string> reject_attrs;
  std::list<std::string> policy_locations;
  Arc::XMLNodeContainer policy_docs;
 protected:
  static Arc::Logger logger;
};

} // namespace ArcSec

#endif /* __ARC_SEC_GACLPDP_H__ */

