#ifndef __ARC_SEC_ARCPOLICY_H__
#define __ARC_SEC_ARCPOLICY_H__

#include <list>
#include <arc/XMLNode.h>
#include <arc/security/ArcPDP/policy/Policy.h>
#include <arc/security/ArcPDP/alg/CombiningAlg.h>
#include <arc/security/ArcPDP/alg/AlgFactory.h>
#include <arc/security/ArcPDP/Evaluator.h>

namespace ArcSec {

///ArcPolicy class to parse and operate Arc specific <Policy> node
class ArcPolicy : public Policy {
public:
  /**Constructor*/
  ArcPolicy(void);

  /**Constructor*/
  ArcPolicy(const Arc::XMLNode node);

  /**Constructor*/
  ArcPolicy(const Arc::XMLNode node, EvaluatorContext* ctx);  

  virtual ~ArcPolicy();  

  virtual operator bool(void) const { return (bool)policynode; };

  virtual Result eval(EvaluationCtx* ctx);

  virtual void setEvaluatorContext(EvaluatorContext* evaluatorcontext) { evaluatorctx = evaluatorcontext; };

  /**Parse XMLNode, and construct the low-level Rule object*/
  virtual void make_policy();

  virtual MatchResult match(EvaluationCtx* ctx);

  virtual std::string getEffect() const { return "Not_applicable";};

  virtual EvalResult& getEvalResult();

  virtual void setEvalResult(EvalResult& res);

  virtual const char* getEvalName() const;

  virtual const char* getName() const;

  static Arc::Plugin* get_policy(Arc::PluginArgument* arg);

private:
  //std::list<Arc::Policy*> rules;
  std::string id;
  std::string version;
  
  /**The combining algorithm between lower-lever element, <Rule>*/
  CombiningAlg *comalg;
  std::string description;

  /**Evaluator Context which contains factory object*/
  EvaluatorContext* evaluatorctx;

  /**Algorithm factory*/
  AlgFactory *algfactory;

  EvalResult evalres;

  /**Corresponding <Policy> node*/
  Arc::XMLNode policynode;

  /**Top element of policy tree*/
  Arc::XMLNode policytop;

protected:
  static Arc::Logger logger;
};

} // namespace ArcSec

#endif /* __ARC_SEC_ARCPOLICY_H__ */

