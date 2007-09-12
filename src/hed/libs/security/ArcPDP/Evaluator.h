#ifndef __ARC_ARCEVALUATE_H__
#define __ARC_ARCEVALUATE_H__

#include <list>
#include <fstream>
#include <arc/XMLNode.h>
#include <arc/Logger.h>

#include "policy/PolicyStore.h"
#include "fn/ArcFnFactory.h"
#include "attr/ArcAttributeFactory.h"
#include "Request.h"
#include "Response.h"

/** Execute the policy evaluation, based on the request and policy */

namespace Arc {

class Evaluator {
private:
  PolicyStore *plstore;
  FnFactory* fnfactory;
  AttributeFactory* attrfactory;  

public:
  Evaluator (XMLNode& cfg);
  Evaluator (const char * cfgfile);
  virtual ~Evaluator();

  virtual Arc::Response* evaluate(Arc::Request* request);
  virtual Arc::Response* evaluate(const std::string& reqfile);
  virtual Arc::Response* evaluate(Arc::EvaluationCtx* ctx);

private:
  void parsecfg(XMLNode& cfg);
};

} // namespace Arc

#endif /* __ARC_EVALUATOR_H__ */
