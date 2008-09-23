#ifndef __ARC_SEC_POLICY_H__
#define __ARC_SEC_POLICY_H__

#include <list>
#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include <arc/loader/LoadableClass.h>

#include "../EvaluationCtx.h"
#include "../Result.h"

namespace ArcSec {

class EvaluatorContext;

///Interface for containing and processing different types of policy.
/**Basically, each policy object is a container which includes a few elements 
 *e.g., ArcPolicySet objects includes a few ArcPolicy objects; ArcPolicy object 
 *includes a few ArcRule objects. There is logical relationship between ArcRules 
 *or ArcPolicies, which is called combining algorithm. According to algorithm, 
 *evaluation results from the elements are combined, and then the combined 
 *evaluation result is returned to the up-level. 
 */
class Policy : public Arc::LoadableClass {
protected:
  std::list<Policy*> subelements;
  static Arc::Logger logger; 
 
public:
  /// Template constructor - creates empty policy
  Policy() {};

  /// Template constructor - creates policy based on XML document
  /** If XML document is empty then empty policy is created. If it is not 
    empty then it must be valid policy document - otherwise created object
    should be invalid. */
  Policy(const Arc::XMLNode) {};  

  /// Template constructor - creates policy based on XML document
  /** If XML document is empty then empty policy is created. If it is not 
    empty then it must be valid policy document - otherwise created object
    should be invalid. This constructor is based on the policy node and i
    the EvaluatorContext which includes the factory objects for combining 
    algorithm and function */ 
  Policy(const Arc::XMLNode, EvaluatorContext*) {};
  virtual ~Policy(){};

  /// Returns true is object is valid.
  virtual operator bool(void) = 0;
  
  ///Evaluate whether the two targets to be evaluated match to each other
  virtual MatchResult match(EvaluationCtx*) = 0;

/**Evaluate policy
 * For the <Rule> of Arc, only get the "Effect" from rules;
 * For the <Policy> of Arc, combine the evaluation result from <Rule>;
 * For the <Rule> of XACML, evaluate the <Condition> node by using information from request, 
 * and use the "Effect" attribute of <Rule>;
 * For the <Policy> of XACML, combine the evaluation result from <Rule>
 */
  virtual Result eval(EvaluationCtx*) = 0;

  /**Add a policy element to into "this" object */
  virtual void addPolicy(Policy* pl){subelements.push_back(pl);};

  /**Set Evaluator Context for the usage in creating low-level policy object*/
  virtual void setEvaluatorContext(EvaluatorContext*) {};

  /**Parse XMLNode, and construct the low-level Rule object*/
  virtual void make_policy() {};

  /**Get the "Effect" attribute*/
  virtual std::string getEffect() = 0;
  
  /**Get eveluation result*/
  virtual EvalResult& getEvalResult() = 0;

  /**Set eveluation result*/
  virtual void setEvalResult(EvalResult& res) = 0;
};

} // namespace ArcSec

#endif /* __ARC_SEC_POLICY_H__ */

