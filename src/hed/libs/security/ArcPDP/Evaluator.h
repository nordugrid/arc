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

typedef enum {
  /** Evaluation is carried out till any non-matching policy found
    and all matching policies are discarded from reported list.
    This is a default behavior. */
  EvaluatorFailsOnDeny,
  /** Evaluation is carried out till any non-matching policy found */
  EvaluatorStopsOnDeny,
  /** Evaluation is carried out till any matching policy found */
  EvaluatorStopsOnPermit,
  /** Evaluation is done till all policies are checked. */
  EvaluatorStopsNever
} EvaluatorCombiningAlg;

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

  /**Evaluates the request by using a Request object.
    Evaluation is done till at least one of policies is satisfied. */
  virtual Response* evaluate(Request* request) = 0;

  /**Evaluates the request by using a XMLNode*/
  virtual Response* evaluate(Arc::XMLNode& node) = 0;

  /**Evaluates the request by using the input request file*/
  virtual Response* evaluate(const char* reqfile) = 0;

 /**Evaluates the request by using a string as input*/
  virtual Response* evaluate(std::string& reqstring) = 0;

  /**Evaluate the specified request against the specified policy. Using policy file as argument
  *All of the existing policy inside the evaluator will be repalaced by the policy argument*/
  virtual Response* evaluate(Request* request, const char* policyfile) = 0;
  virtual Response* evaluate(Arc::XMLNode& node, const char* policyfile) = 0;
  virtual Response* evaluate(const char* reqfile, const char* policyfile) =0;
  virtual Response* evaluate(std::string& reqstr, const char* policyfile) =0;

  /**Evaluate the specified request against the specified policy. Using policy string as argument
  *All of the existing policy inside the evaluator will be repalaced by the policy argument*/
  virtual Response* evaluate(Request* request, std::string& policystr) = 0;
  virtual Response* evaluate(Arc::XMLNode& node, std::string& policystr) = 0;
  virtual Response* evaluate(const char* reqfile, std::string& policystr) =0;
  virtual Response* evaluate(std::string& reqstr, std::string& policystr) =0;

  /**Evaluate the specified request against the specified policy. Using policy XMLNode as argument
  *All of the existing policy inside the evaluator will be repalaced by the policy argument*/
  virtual Response* evaluate(Request* request, Arc::XMLNode& policynode) = 0;
  virtual Response* evaluate(Arc::XMLNode& node, Arc::XMLNode& policynode) = 0;
  virtual Response* evaluate(const char* reqfile, Arc::XMLNode& policynode) =0;
  virtual Response* evaluate(std::string& reqstr, Arc::XMLNode& policynode) =0;

  /**Evaluate the specified request against the specified policy. Using policy object as argument
  *All of the existing policy inside the evaluator will be repalaced by the policy argument*/
  virtual Response* evaluate(Request* request, BasePolicy* policyobj) = 0;
  virtual Response* evaluate(Arc::XMLNode& node, BasePolicy* policyobj) = 0;
  virtual Response* evaluate(const char* reqfile, BasePolicy* policyobj) =0;
  virtual Response* evaluate(std::string& reqstr, BasePolicy* policyobj) =0;

  /**Get the AttributeFactory object*/
  virtual AttributeFactory* getAttrFactory () = 0;

  /**Get the FnFactory object*/
  virtual FnFactory* getFnFactory () = 0;

  /**Get the AlgFactory object*/
  virtual AlgFactory* getAlgFactory () = 0;

  /**Add policy to the evaluator, use filename as argument*/
  virtual void addPolicy(const char* policyfile,const std::string& id = "") = 0;

  /**Add policy to the evaluator, use file string as argument*/
  virtual void addPolicy(std::string& policystr,const std::string& id = "") = 0;

  /**Add policy to the evaluator, use XMLNode as argument*/
  virtual void addPolicy(Arc::XMLNode& policynode,const std::string& id = "") = 0;

  /**Add policy to the evaluator, use Policy object as argument*/
  virtual void addPolicy(BasePolicy* policy,const std::string& id = "") = 0;

  virtual void setCombiningAlg(EvaluatorCombiningAlg alg) = 0;

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
