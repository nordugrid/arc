#ifndef __ARC_SEC_ATTRIBUTESELECTOR_H__
#define __ARC_SEC_ATTRIBUTESELECTOR_H__

#include <string>
#include <list>
#include <arc/XMLNode.h>
#include <arc/security/ArcPDP/attr/AttributeValue.h>

namespace ArcSec {

//AttibuteSelector is for the element <AttributeSelector> in xacml policy schema, and
//in charge of getting attribute value from the request, by using xpath

class AttributeSelector {
public:
  AttributeSelector(Arc::XMLNode& node, AttributeFactory* attrfactory);
  virtual ~AttributeSelector();

  virtual std::list<AttributeValue*> evaluate(EvaluationCtx* ctx);

private:
  std::string type;
  std::string reqctxpath; 
  //The <Policy> node from which the xpath searchs
  Arc::XMLNode policyroot;
  std::string xpathver; 
  boolean present;
};

} // namespace ArcSec

#endif /* __ARC_SEC_ATTRIBUTESELECTOR_H__ */

