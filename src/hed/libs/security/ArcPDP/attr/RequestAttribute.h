#ifndef __ARC_ATTRIBUTE_H__
#define __ARC_ATTRIBUTE_H__

#include "AttributeValue.h"
#include "AttributeFactory.h"
#include <list>
#include <arc/XMLNode.h>

namespace Arc {

//typedef std::list<AttributeValue*> AttrValList;

/**Parsing the attribute in Request.xml according to DateType*/

class RequestAttribute {
public:
  RequestAttribute(Arc::XMLNode& node, AttributeFactory* attrfactory);
  virtual ~RequestAttribute();
  
public:
  std::string getAttributeId () const;
  void setAttributeId (const std::string attributeId);
  std::string getDataType () const;
  void setDataType (const std::string dataType);
  std::string getIssuer () const;
  void setIssuer (const std::string issuer);
  //AttrValList getAttributeValueList () const;
  //void setAttributeValueList (const AttrValList& attributeValueList);

  AttributeValue* getAttributeValue() const;

//protect:
  //AttrValList avlist;

private:
 std::string id;
 std::string type;
 std::string issuer;
 //AttrValList avlist;
 AttributeValue* attrval;
};

} // namespace Arc

#endif /* __ARC_ATTRIBUTE_H__ */
