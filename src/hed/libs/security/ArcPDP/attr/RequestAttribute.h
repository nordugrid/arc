#ifndef __ARC_REQUESTATTRIBUTE_H__
#define __ARC_REQUESTATTRIBUTE_H__

#include "AttributeValue.h"
#include "AttributeFactory.h"
#include <list>
#include <arc/XMLNode.h>
#include <arc/Logger.h>

namespace Arc {

//typedef std::list<AttributeValue*> AttrValList;

/**Parsing the attribute in Request.xml according to DateType*/ 

class RequestAttribute {
public:
  RequestAttribute(Arc::XMLNode& node, AttributeFactory* attrfactory);
  RequestAttribute();
  virtual ~RequestAttribute();
  
public:
  XMLNode getNode();
  std::string getAttributeId () const;
  void setAttributeId (const std::string attributeId);
  std::string getDataType () const;
  void setDataType (const std::string dataType);
  std::string getIssuer () const;
  void setIssuer (const std::string issuer);
  //AttrValList getAttributeValueList () const;
  //void setAttributeValueList (const AttrValList& attributeValueList);

  virtual AttributeValue* getAttributeValue() const;

  virtual AttributeFactory* getAttributeFactory() const;

  RequestAttribute& duplicate(RequestAttribute&);

//protect:
  //AttrValList avlist;

private:
 static Logger logger;
 XMLNode node_;
 std::string id;
 std::string type;
 std::string issuer;
 //AttrValList avlist;
 AttributeValue* attrval;
 AttributeFactory* attrfactory;
};

} // namespace Arc

#endif /* __ARC_REQUESTATTRIBUTE_H__ */
