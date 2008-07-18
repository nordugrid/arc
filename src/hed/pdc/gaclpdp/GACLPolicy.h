#ifndef __ARC_SEC_GACLPOLICY_H__
#define __ARC_SEC_GACLPOLICY_H__

/*
#include <list>
#include <arc/XMLNode.h>
#include <arc/security/ArcPDP/alg/CombiningAlg.h>
#include <arc/security/ArcPDP/alg/AlgFactory.h>
#include <arc/security/ArcPDP/Evaluator.h>
*/
#include <arc/security/ArcPDP/policy/Policy.h>

namespace ArcSec {

class GACLPolicy : public Policy {
public:
  /**Constructor*/
  GACLPolicy(Arc::XMLNode* node);

  virtual ~GACLPolicy();  

  virtual Result eval(EvaluationCtx* ctx);

  //virtual void setEvaluatorContext(EvaluatorContext* evaluatorcontext) { evaluatorctx = evaluatorcontext; };

  /**Parse XMLNode, and construct the low-level Rule object*/
  //virtual void make_policy();

  virtual MatchResult match(EvaluationCtx* ctx) { };

  virtual std::string getEffect() { return ""; };

  virtual EvalResult& getEvalResult();

  virtual void setEvalResult(EvalResult& res);

private:
  //std::string id;
  //std::string version;
  //std::string description;

  /**Evaluator Context which contains factory object*/
  //EvaluatorContext* evaluatorctx;

  /**Algorithm factory*/
  //AlgFactory *algfactory;

  EvalResult evalres;

  /**Corresponding <Policy> node*/
  Arc::XMLNode policynode;

protected:
  static Arc::Logger logger;
};

} // namespace ArcSec

#endif /* __ARC_SEC_GACLPOLICY_H__ */

