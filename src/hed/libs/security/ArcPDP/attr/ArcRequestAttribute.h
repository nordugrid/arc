#ifndef __ARC_ARCREQUESTATTRIBUTE_H__
#define __ARC_ARCREQUESTATTRIBUTE_H__

#include "AttributeValue.h"
#include "AttributeFactory.h"
#include <list>
#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include "attr/RequestAttribute.h"

namespace Arc {

//typedef std::list<AttributeValue*> AttrValList;

/**Parsing the attribute in Request.xml according to DateType*/ 

class ArcRequestAttribute : public RequestAttribute {
public:
  ArcRequestAttribute(Arc::XMLNode& node, AttributeFactory* attrfactory);
  ArcRequestAttribute();
  virtual ~ArcRequestAttribute();
  
public:
  virtual XMLNode getNode();
  std::string getAttributeId () const;
  void setAttributeId (const std::string attributeId);
  virtual std::string getDataType () const;
  virtual void setDataType (const std::string dataType);
  std::string getIssuer () const;
  void setIssuer (const std::string issuer);
  //AttrValList getAttributeValueList () const;
  //void setAttributeValueList (const AttrValList& attributeValueList);

  virtual AttributeValue* getAttributeValue() const;

  virtual AttributeFactory* getAttributeFactory() const;

  virtual RequestAttribute* duplicate(RequestAttribute*);

//protect:
  //AttrValList avlist;

private:
 XMLNode node_;
 std::string id;
 std::string type;
 std::string issuer;
 //AttrValList avlist;
 AttributeValue* attrval;
 AttributeFactory* attrfactory;
};

} // namespace Arc

#endif /* __ARC_ARCREQUESTATTRIBUTE_H__ */
