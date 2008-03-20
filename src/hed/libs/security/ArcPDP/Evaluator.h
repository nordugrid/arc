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

///Interface for policy evaluation.  Execute the policy evaluation, based on the request and policy

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

  /**Evaluates the request by using a Request object */
  virtual Response* evaluate(Request* request) = 0;

  /**Evaluates the request by using a XMLNode*/
  virtual Response* evaluate(Arc::XMLNode& node) = 0;

  /**Evaluates the request by using the input request file*/
  virtual Response* evaluate(const std::string& reqfile) = 0;

  /**The same as the above three ones, using policy file as argument*/
  virtual Response* evaluate(Request* request, std::string& policyfile) = 0;
  virtual Response* evaluate(Arc::XMLNode& node, std::string& policyfile) = 0;
  virtual Response* evaluate(const std::string& reqfile, std::string& policyfile) =0;

  /**Get the AttributeFactory object*/
  virtual AttributeFactory* getAttrFactory () = 0;

  /**Get the FnFactory object*/
  virtual FnFactory* getFnFactory () = 0;

  /**Get the AlgFactory object*/
  virtual AlgFactory* getAlgFactory () = 0;

protected:
  /**Evaluate the request by using the EvaluationCtx object (which includes the information about request)*/
  virtual Response* evaluate(EvaluationCtx* ctx) = 0;

private:
  /**Parse the configuration, and dynamically create PolicyStore, AttributeFactory, FnFactory and AlgFactoryy*/
  virtual void parsecfg(Arc::XMLNode& cfg) = 0;
};

///Context for evaluator. It includes the factories which will be used to create related objects
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
