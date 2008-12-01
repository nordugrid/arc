#ifndef __ARC_SEC_EVALUATOR_H__
#define __ARC_SEC_EVALUATOR_H__

#include <list>
#include <fstream>
#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include <arc/security/ClassLoader.h>
#include <arc/security/ArcPDP/Source.h>

#include "fn/FnFactory.h"
#include "attr/AttributeFactory.h"
#include "alg/AlgFactory.h"
#include "Request.h"
#include "Response.h"

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

///Interface for policy evaluation.  Execute the policy evaluation, based on the request and policy
class Evaluator : public Arc::LoadableClass {
protected:
  static Arc::Logger logger;
public:
  Evaluator (Arc::XMLNode*) {};
  Evaluator (const char *) {};
  virtual ~Evaluator() {};

  /**Evaluates the request by using a Request object.
    Evaluation is done till at least one of policies is satisfied. */
  virtual Response* evaluate(Request* request) = 0;

  /**Evaluates the request by using a specified source */
  virtual Response* evaluate(const Source& request) = 0;

  /**Evaluate the specified request against the policy from specified source. 
  *All of the existing policy inside the evaluator will be replaced by the policy argument*/
  virtual Response* evaluate(Request* request, const Source& policy) = 0;

  /**Evaluate the request from specified source against the policy from specified source.
  *All of the existing policy inside the evaluator will be replaced by the policy argument*/
  virtual Response* evaluate(const Source& request, const Source& policy) = 0;

  /**Evaluate the specified request against the specified policy.
  *All of the existing policy inside the evaluator will be repalaced by the policy argument*/
  virtual Response* evaluate(Request* request, Policy* policyobj) = 0;

  /**Evaluate the request from specified source against the specified policy.
  *All of the existing policy inside the evaluator will be repalaced by the policy argument*/
  virtual Response* evaluate(const Source& request, Policy* policyobj) = 0;

  /**Get the AttributeFactory object*/
  virtual AttributeFactory* getAttrFactory () = 0;

  /**Get the FnFactory object*/
  virtual FnFactory* getFnFactory () = 0;

  /**Get the AlgFactory object*/
  virtual AlgFactory* getAlgFactory () = 0;

  /**Add policy from specified source to the evaluator. Policy will be marked with id. */
  virtual void addPolicy(const Source& policy,const std::string& id = "") = 0;

  /**Add policy to the evaluator. Policy will be marked with id. */
  virtual void addPolicy(Policy* policy,const std::string& id = "") = 0;

  /**Specifies one of simple combining algorithms. In case of multiple policies their results will be combined using this algorithm. */
  virtual void setCombiningAlg(EvaluatorCombiningAlg alg) = 0;

  /**Specifies loadable combining algorithms. In case of multiple policies their results will be combined using this algorithm. To switch to simple algorithm specify NULL argument. */
  virtual void setCombiningAlg(CombiningAlg* alg = NULL) = 0;

  /**Get the name of this evaluator*/
  virtual const char* getName(void) const = 0;
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
