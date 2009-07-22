#ifndef __ARC_SEC_XACMLAPPLY_H__
#define __ARC_SEC_XACMLAPPLY_H__

#include <list>
#include <arc/XMLNode.h>
#include <arc/security/ArcPDP/attr/AttributeValue.h>
#include <arc/security/ArcPDP/fn/Function.h>
#include <arc/security/ArcPDP/attr/AttributeFactory.h>
#include <arc/security/ArcPDP/fn/FnFactory.h>
#include <arc/security/ArcPDP/Evaluator.h>

#include "./AttributeSelector.h"
#include "./AttributeDesignator.h"

namespace ArcSec {

//<Apply/>
class XACMLApply {
public:
  XACMLApply(Arc::XMLNode& node, EvaluatorContext* ctx);
  virtual ~XACMLApply();
  virtual std::list<AttributeValue*> evaluate(EvaluationCtx* ctx);

private:
  Arc::XMLNode applynode;
  std::string functionId;

  AttributeFactory* attrfactory;
  FnFactory* fnfactory;

  Function* function;

  /**Sub <Expression/>*, the first value of map is the apperance sequence 
  *in this <Apply/>, because sequance should be counted in case of function
  *such as  "less-or-equal"
  */
  std::map<int, AttributeValue*> attrval_list;
  std::map<int, XACMLApply*> sub_apply_list;
  std::map<int, AttributeDesignator*> designator_list;
  std::map<int, AttributeSelector*> selector_list;
};

} // namespace ArcSec

#endif /* __ARC_SEC_XACMLAPPLY_H__ */

