#ifndef __ARC_ARCEVALUATE_H__
#define __ARC_ARCEVALUATE_H__

#include "Request.h"
#include <list>
#include <fstream>
#include "common/XMLNode.h"
#include "common/Logger.h"

#include "policy/PolicyStore.h"
#include "fn/ArcFnFactory.h"
#include "attr/ArcAttributeFactory.h"
#include "Response.h"

/** Execute the policy evaluation, based on the request and policy */

namespace Arc {

class Evaluator {
private:
  PolicyStore *plstore;
  FnFactory* fnfactory;
  AttributeFactory* attrfactory;  

public:
  Evaluator (const XMLNode& cfg);
  Evaluator (const std::string& cfgfile);
  virtual ~Evaluator();

  virtual Arc::Response* evaluate(const Arc::Request* request);
  virtual Arc::Response* evaluate(const std::string& reqfile);
  virtual Arc::Response* evaluate(Arc::EvaluationCtx* ctx);
};

} // namespace Arc

#endif /* __ARC_EVALUATOR_H__ */
