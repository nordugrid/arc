#ifndef __ARC_SEC_ARCRULE_H__
#define __ARC_SEC_ARCRULE_H__

#include <arc/XMLNode.h>
#include <list>

#include <arc/security/ArcPDP/policy/Policy.h>
#include <arc/security/ArcPDP/fn/Function.h>
#include <arc/security/ArcPDP/alg/CombiningAlg.h>
#include <arc/security/ArcPDP/attr/AttributeFactory.h>
#include <arc/security/ArcPDP/fn/FnFactory.h>
#include <arc/security/ArcPDP/Evaluator.h>

#include "./XACMLTarget.h"

namespace ArcSec {
///XACMLRule class to parse XACML specific <Rule> node
class XACMLRule : public Policy {

public:
  XACMLRule(Arc::XMLNode& node, EvaluatorContext* ctx);  

  virtual std::string getEffect();

  virtual Result eval(EvaluationCtx* ctx);

  virtual MatchResult match(EvaluationCtx* ctx);

  virtual ~XACMLRule();

  virtual EvalResult& getEvalResult();

private:
  std::string effect;
  std::string id;
  std::string version;
  std::string description;

  AttributeFactory* attrfactory;
  FnFactory* fnfactory;

  EvalResult evalres;
  Arc::XMLNode rulenode;

  XACMLTarget* target;

protected:
  static Arc::Logger logger;
};

} // namespace ArcSec

#endif /* __ARC_SEC_ARCRULE_H__ */

