#ifndef __ARC_SEC_ARCEVALUATE_H__
#define __ARC_SEC_ARCEVALUATE_H__

#include <list>
#include <fstream>
#include <arc/XMLNode.h>
#include <arc/Logger.h>

#include "policy/PolicyStore.h"
#include "fn/FnFactory.h"
#include "attr/AttributeFactory.h"
#include "alg/AlgFactory.h"
#include "Request.h"
#include "Response.h"

/** Execute the policy evaluation, based on the request and policy */

namespace ArcSec {

class Evaluator {
friend class EvaluatorContext;
private:
  static Arc::Logger logger;
  PolicyStore *plstore;
  FnFactory* fnfactory;
  AttributeFactory* attrfactory;  
  AlgFactory* algfactory;
  
  EvaluatorContext* context;

public:
  Evaluator (Arc::XMLNode& cfg);
  Evaluator (const char * cfgfile);
  virtual ~Evaluator();

  virtual Response* evaluate(Request* request);
  virtual Response* evaluate(const std::string& reqfile);
  virtual Response* evaluate(EvaluationCtx* ctx);
  virtual Response* evaluate(Arc::XMLNode& node);

private:
  void parsecfg(Arc::XMLNode& cfg);
};


class EvaluatorContext {
  friend class Evaluator;
  private:
    Evaluator& evaluator;
    EvaluatorContext(Evaluator& evaluator) : evaluator(evaluator) {};
    ~EvaluatorContext() {};
   public:
    /** Returns associated AttributeFactory object */
    operator AttributeFactory*()    { return evaluator.attrfactory; };
    /** Returns associated FnFactory object */
    operator FnFactory*()        { return evaluator.fnfactory; };
    /** Returns associated AlgFactory object */
    operator AlgFactory*() { return evaluator.algfactory; };
  };


} // namespace ArcSec

#endif /* __ARC_SEC_EVALUATOR_H__ */
