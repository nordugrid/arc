#ifndef __ARC_SEC_XACMLPOLICY_H__
#define __ARC_SEC_XACMLPOLICY_H__

#include <list>
#include <arc/XMLNode.h>
#include <arc/security/ArcPDP/policy/Policy.h>
#include <arc/security/ArcPDP/alg/CombiningAlg.h>
#include <arc/security/ArcPDP/alg/AlgFactory.h>
#include <arc/security/ArcPDP/Evaluator.h>

namespace ArcSec {

///XACMLPolicy class to parse and operate XACML specific <Policy> node
class XACMLPolicy : public Policy {
public:
  /**Constructor - */
  XACMLPolicy(Arc::XMLNode& node, EvaluatorContext* ctx);  

  virtual ~XACMLPolicy();  

  virtual Result eval(EvaluationCtx* ctx);

  virtual MatchResult match(EvaluationCtx* ctx);

  virtual std::string getEffect(){ return "Not_applicable";};

  virtual EvalResult& getEvalResult();

private:
  //std::list<Arc::Policy*> rules;
  std::string id;
  std::string version;
  
  /**The combining algorithm between lower-lever element, <Rule>*/
  CombiningAlg *comalg;
  std::string description;

  /**Algorithm factory*/
  AlgFactory *algfactory;

  EvalResult evalres;

  /**Corresponding <Policy> node*/
  Arc::XMLNode policynode;

protected:
  static Arc::Logger logger;

};

} // namespace ArcSec

#endif /* __ARC_SEC_XACMLPOLICY_H__ */

