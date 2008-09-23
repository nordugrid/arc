#ifndef __ARC_SEC_GACLPOLICY_H__
#define __ARC_SEC_GACLPOLICY_H__

#include <arc/security/ArcPDP/policy/Policy.h>

namespace ArcSec {

class GACLPolicy : public Policy {
public:
  GACLPolicy(void);

  GACLPolicy(const Source& source);

  GACLPolicy(const Arc::XMLNode source);

  virtual ~GACLPolicy();  

  virtual operator bool(void) { return (bool)policynode; };

  virtual Result eval(EvaluationCtx* ctx);

  virtual MatchResult match(EvaluationCtx* ctx) { };

  virtual std::string getEffect() { return ""; };

  virtual EvalResult& getEvalResult();

  virtual void setEvalResult(EvalResult& res);

  Arc::XMLNode getXML(void) { return policynode; };

private:

  EvalResult evalres;

  Arc::XMLNode policynode;

protected:
  static Arc::Logger logger;
};

} // namespace ArcSec

#endif /* __ARC_SEC_GACLPOLICY_H__ */

