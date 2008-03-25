#ifndef __ARC_SEC_XACMLTARGET_H__
#define __ARC_SEC_XACMLTARGET_H__

#include <list>
#include <arc/XMLNode.h>
#include <arc/security/ArcPDP/fn/Function.h>
#include <arc/security/ArcPDP/attr/AttributeFactory.h>
#include <arc/security/ArcPDP/fn/FnFactory.h>
#include <arc/security/ArcPDP/Evaluator.h>

namespace ArcSec {

class XACMLTargetMatch {
  XACMLTarget(Arc::XMLNode& node, EvaluatorContext* ctx);

  virtual ~XACMLTargetMatch();

  virtual MatchResult match(EvaluationCtx* ctx);

private:
  AttributeFactory* attrfactory;
  FnFactoty* fnfactory;
  Arc::XMLNode matchnode;
  std::string matchId;

  Function* function;
};

class XACMLTargetMatchGroup {
  XACMLTargetMatchGroup(Arc::XMLNode& node, EvaluatorContext* ctx);

  virtual ~XACMLTargetMatchGroup();

  virtual MatchResult match(EvaluationCtx* ctx);
};

class XACMLTargetSection {
  XACMLSection(Arc::XMLNode& node, EvaluatorContext* ctx);

  virtual ~XACMLSection();

  virtual MatchResult match(EvaluationCtx* ctx);
};

///XACMLTarget class to parse and operate XACML specific <Target> node
class XACMLTarget {
public:
  /**Constructor - */
  XACMLTarget(Arc::XMLNode& node, EvaluatorContext* ctx);  

  virtual ~XACMLTarget();  

  virtual MatchResult match(EvaluationCtx* ctx);

private:
  Arc::XMLNode targetnode;
};

} // namespace ArcSec

#endif /* __ARC_SEC_XACMLTARGET_H__ */

