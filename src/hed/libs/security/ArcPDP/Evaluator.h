#ifndef __ARC_ARCEVALUATE_H__
#define __ARC_ARCEVALUATE_H__

#include <list>
#include <fstream>
#include <arc/XMLNode.h>
#include <arc/Logger.h>

#include "policy/PolicyStore.h"
#include "fn/ArcFnFactory.h"
#include "attr/ArcAttributeFactory.h"
#include "alg/ArcAlgFactory.h"
#include "Request.h"
#include "Response.h"

/** Execute the policy evaluation, based on the request and policy */

namespace Arc {

class Evaluator {
friend class EvaluatorContext;
private:
  static Logger logger;
  PolicyStore *plstore;
  FnFactory* fnfactory;
  AttributeFactory* attrfactory;  
  AlgFactory* algfactory;
  
  EvaluatorContext* context;

public:
  Evaluator (XMLNode& cfg);
  Evaluator (const char * cfgfile);
  virtual ~Evaluator();

 // virtual Arc::Response* evaluate(Arc::Request* request);
  virtual Arc::Response* evaluate(const std::string& reqfile);
  virtual Arc::Response* evaluate(Arc::EvaluationCtx* ctx);
  virtual Arc::Response* evaluate(XMLNode& node);

private:
  void parsecfg(XMLNode& cfg);
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


} // namespace Arc

#endif /* __ARC_EVALUATOR_H__ */
