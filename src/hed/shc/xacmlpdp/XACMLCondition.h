#ifndef __ARC_SEC_XACMLCONDITION_H__
#define __ARC_SEC_XACMLCONDITION_H__

#include <list>
#include <arc/XMLNode.h>
#include <arc/security/ArcPDP/attr/AttributeValue.h>
#include <arc/security/ArcPDP/fn/Function.h>
#include <arc/security/ArcPDP/attr/AttributeFactory.h>
#include <arc/security/ArcPDP/fn/FnFactory.h>
#include <arc/security/ArcPDP/Evaluator.h>

#include "./AttributeSelector.h"
#include "./AttributeDesignator.h"
#include "./XACMLApply.h"

namespace ArcSec {

///XACMLCondition class to parse and operate XACML specific <Condition> node.
class XACMLCondition {
public:
  /**Constructor - */
  XACMLCondition(Arc::XMLNode& node, EvaluatorContext* ctx);  
  virtual ~XACMLCondition();  
  std::list<AttributeValue*> evaluate(EvaluationCtx* ctx);

private:
  Arc::XMLNode condition_node;
  std::list<XACMLApply*> apply_list;

};

} // namespace ArcSec

#endif /* __ARC_SEC_XACMLCONDITION_H__ */

