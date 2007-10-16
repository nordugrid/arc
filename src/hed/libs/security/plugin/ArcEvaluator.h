#ifndef __ARC_SEC_ARCEVALUATOR_H__
#define __ARC_SEC_ARCEVALUATOR_H__

#include <list>
#include <fstream>
#include <arc/XMLNode.h>
#include <arc/Logger.h>

#include <arc/security/ArcPDP/Evaluator.h>
#include <arc/security/ArcPDP/fn/FnFactory.h>
#include <arc/security/ArcPDP/attr/AttributeFactory.h>
#include <arc/security/ArcPDP/alg/AlgFactory.h>
#include <arc/security/ArcPDP/Request.h>
#include <arc/security/ArcPDP/Response.h>
#include "PolicyStore.h"

/** Execute the policy evaluation, based on the request and policy */

namespace ArcSec {

class ArcEvaluator : public Evaluator {
friend class EvaluatorContext;
private:
  static Arc::Logger logger;
  PolicyStore *plstore;
  FnFactory* fnfactory;
  AttributeFactory* attrfactory;  
  AlgFactory* algfactory;
  
  EvaluatorContext* context;

public:
  ArcEvaluator (Arc::XMLNode& cfg);
  ArcEvaluator (const char * cfgfile);
  virtual ~ArcEvaluator();

  virtual Response* evaluate(Request* request);
  virtual Response* evaluate(const std::string& reqfile);
  virtual Response* evaluate(EvaluationCtx* ctx);
  virtual Response* evaluate(Arc::XMLNode& node);

  virtual AttributeFactory* getAttrFactory () { return attrfactory;};
  virtual FnFactory* getFnFactory () { return fnfactory; };
  virtual AlgFactory* getAlgFactory () { return algfactory; };

private:
  virtual void parsecfg(Arc::XMLNode& cfg);
};

} // namespace ArcSec

#endif /* __ARC_SEC_ARCEVALUATOR_H__ */
