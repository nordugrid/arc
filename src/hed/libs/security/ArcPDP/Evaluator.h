#ifndef __ARC_SEC_EVALUATOR_H__
#define __ARC_SEC_EVALUATOR_H__

#include <list>
#include <fstream>
#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include <arc/loader/LoadableClass.h>

#include "fn/FnFactory.h"
#include "attr/AttributeFactory.h"
#include "alg/AlgFactory.h"
#include "Request.h"
#include "Response.h"

/** Execute the policy evaluation, based on the request and policy */

namespace ArcSec {

class Evaluator : public Arc::LoadableClass {
protected:
  static Arc::Logger logger;
/*private:
  PolicyStore *plstore;
  FnFactory* fnfactory;
  AttributeFactory* attrfactory;  
  AlgFactory* algfactory;
  EvaluatorContext* context;
*/
public:
  Evaluator (Arc::XMLNode*) {};
  Evaluator (const char *) {};
  virtual ~Evaluator() {};

  virtual Response* evaluate(Request* request) = 0;
  virtual Response* evaluate(Arc::XMLNode& node) = 0;
  virtual Response* evaluate(const std::string& reqfile) = 0;

  virtual AttributeFactory* getAttrFactory () = 0;
  virtual FnFactory* getFnFactory () = 0;
  virtual AlgFactory* getAlgFactory () = 0;

protected:
  virtual Response* evaluate(EvaluationCtx* ctx) = 0;

private:
  virtual void parsecfg(Arc::XMLNode& cfg) = 0;
};

class EvaluatorContext {
  private:
    Evaluator* evaluator;
  public:
    EvaluatorContext(Evaluator* evaluator) : evaluator(evaluator) {};
    ~EvaluatorContext() {};
   public:
    /** Returns associated AttributeFactory object */
    operator AttributeFactory*()    { return evaluator->getAttrFactory(); };
    /** Returns associated FnFactory object */
    operator FnFactory*()        { return evaluator->getFnFactory(); };
    /** Returns associated AlgFactory object */
    operator AlgFactory*() { return evaluator->getAlgFactory(); };
  };

} // namespace ArcSec

#endif /* __ARC_SEC_EVALUATOR_H__ */
