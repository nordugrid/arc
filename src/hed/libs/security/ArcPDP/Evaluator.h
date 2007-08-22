#ifndef __ARC_ARCEVALUATE_H__
#define __ARC_ARCEVALUATE_H__

#include "Request.h"
#include <list>
#include <fstream>
#include "../../../../libs/common/XMLNode.h"
#include "../../../../libs/common/Logger.h"

#include "PolicyStore.h"
#include "FnFactory.h"
#include "AttributeFactory.h"

/** Execute the policy evaluation, based on the request and policy */

namespace Arc {

class Evaluator {
private:
  PolicyStore *plstore;
  FnFactory* fnfactory;
  AttributeFactory* attrfactory;  

public:
  Evaluator (const Arc::ArcConfig& cfg);
  Evaluator (const std::string& cfgfile);
  virtual ~Evaluator();

  virtual Arc::Response* evaluate(const Arc::Request* request);
  virtual Arc::Response* evaluate(const std::string& reqfile);
  virtual Arc::Response* evaluate(Arc::EvaluationCtx* ctx)
};

} // namespace Arc

#endif /* __ARC_EVALUATOR_H__ */
