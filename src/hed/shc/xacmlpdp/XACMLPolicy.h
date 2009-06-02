#ifndef __ARC_SEC_XACMLPOLICY_H__
#define __ARC_SEC_XACMLPOLICY_H__

#include <list>

#include <arc/XMLNode.h>
#include <arc/security/ArcPDP/policy/Policy.h>
#include <arc/security/ArcPDP/alg/CombiningAlg.h>
#include <arc/security/ArcPDP/alg/AlgFactory.h>
#include <arc/security/ArcPDP/Evaluator.h>

#include "XACMLTarget.h"

namespace ArcSec {

///XACMLPolicy class to parse and operate XACML specific <Policy> node
class XACMLPolicy : public Policy {
public:
  /**Constructor*/
  XACMLPolicy(void);

  /**Constructor*/
  XACMLPolicy(const Arc::XMLNode node);

  /**Constructor - */
  XACMLPolicy(const Arc::XMLNode node, EvaluatorContext* ctx);  

  virtual ~XACMLPolicy();  

  virtual operator bool(void) const { return (bool)policynode; };

  virtual Result eval(EvaluationCtx* ctx);

  virtual MatchResult match(EvaluationCtx* ctx);

  virtual std::string getEffect() const { return "Not_applicable";};

  virtual EvalResult& getEvalResult();

  virtual void setEvalResult(EvalResult& res) { evalres = res; };

  const char* getEvalName() const {   return "xacml.evaluator"; };

  const char* getName() const {   return "xacml.policy"; };

  static Arc::Plugin* get_policy(Arc::PluginArgument* arg);

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

  /**The object for containing <Target/> information*/
  XACMLTarget* target;

protected:
  static Arc::Logger logger;

};

} // namespace ArcSec

#endif /* __ARC_SEC_XACMLPOLICY_H__ */

