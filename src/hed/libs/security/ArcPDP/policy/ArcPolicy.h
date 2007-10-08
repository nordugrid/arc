#ifndef __ARC_SEC_ARCPOLICY_H__
#define __ARC_SEC_ARCPOLICY_H__

#include <list>
#include <arc/XMLNode.h>
#include <arc/security/ArcPDP/policy/Policy.h>
#include <arc/security/ArcPDP/alg/CombiningAlg.h>
#include <arc/security/ArcPDP/alg/AlgFactory.h>
#include <arc/security/ArcPDP/Evaluator.h>

namespace ArcSec {

/**ArcPolicy class to parsing Arc specific policy format*/

class ArcPolicy : public Policy {

public:
  ArcPolicy(Arc::XMLNode& node, EvaluatorContext* ctx);  

  virtual ~ArcPolicy();  

  virtual Result eval(EvaluationCtx* ctx);

  virtual MatchResult match(EvaluationCtx* ctx);

  virtual std::string getEffect(){ return "NOT Effect";};

private:
  static Arc::Logger logger;
 // std::list<Arc::Policy*> rules;
  std::string id;
  std::string version;
  CombiningAlg *comalg;
  std::string description;

  AlgFactory *algfactory;

};

} // namespace ArcSec

#endif /* __ARC_SEC_ARCPOLICY_H__ */

