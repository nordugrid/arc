#ifndef __ARC_ATTRIBUTE_H__
#define __ARC_ATTRIBUTE_H__

#include "AttributeValue.h"
#include <list>

namespace Arc {

typedef std::list<AttributeValue*> AttrValList;

class Attribute {
public:
  std::string getAttributeId () const;
  void setAttributeId (const std::string attributeId);
  std::string getDataType () const;
  void setDataType (const std::string dataType);
  std::string getIssuer () const;
  void setIssuer (const std::string issuer);
  AttrValList getAttributeValueList () const;
  void setAttributeValueList (const AttrValList& attributeValueList);
};

} // namespace Arc

#endif /* __ARC_ATTRIBUTE_H__ */
