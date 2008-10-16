#ifndef __ARC_SEC_ATTRIBUTEDESGINATOR_H__
#define __ARC_SEC_ATTRIBUTEDESGINATOR_H__

#include <string>
#include <list>
#include <arc/XMLNode.h>
#include <arc/security/ArcPDP/attr/AttributeValue.h>

namespace ArcSec {

//AttibuteDesignator is for the element <AttributeDesignator> in xacml policy schema, and
//in charge of getting attribute value from the request

class AttributeDesignator {
public:
  AttributeDesignator(Arc::XMLNode& node);
  virtual ~AttributeDesignator();

  virtual std::list<AttributeValue*> evaluate(EvaluationCtx* ctx);

private:
  std::string target;
  std::string id;
  std::string type;
  std::string issuer;
  
  boolean present;
};

} // namespace ArcSec

#endif /* __ARC_SEC_ATTRIBUTEDESGINATOR_H__ */

