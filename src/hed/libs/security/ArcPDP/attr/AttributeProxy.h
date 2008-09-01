#ifndef __ARC_SEC_ATTRIBUTEPROXY_H__
#define __ARC_SEC_ATTRIBUTEPROXY_H__

#include <list>
#include <fstream>
#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include "AttributeValue.h"

namespace ArcSec {

///Interface for creating the AttributeValue object, it will be used by AttributeFactory
/**The AttributeProxy object will be insert into AttributeFactoty; and the
 *getAttribute(node) method will be called inside AttributeFacroty.createvalue(node),
 *in order to create a specific AttributeValue 
 */ 
class AttributeProxy {
public:
  AttributeProxy() {};
  virtual ~AttributeProxy(){};
public:
 /**Create a AttributeValue object according to the information inside 
 *the XMLNode as parameter.
 */
  virtual AttributeValue* getAttribute(const Arc::XMLNode& node) = 0;
};

} // namespace ArcSec

#endif /* __ARC_SEC_ATTRIBUTEPROXY_H__ */

