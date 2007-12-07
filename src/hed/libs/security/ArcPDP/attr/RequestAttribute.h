#ifndef __ARC_SEC_REQUESTATTRIBUTE_H__
#define __ARC_SEC_REQUESTATTRIBUTE_H__

#include "AttributeValue.h"
#include "AttributeFactory.h"
#include <list>
#include <arc/XMLNode.h>
#include <arc/Logger.h>

namespace ArcSec {

//typedef std::list<AttributeValue*> AttrValList;

///Wrapper which includes AttributeValue object which is generated according to date type of one spefic node in Request.xml
class RequestAttribute {
public:
  /**Constructor - create attribute value object according to the "Type" in the node
  <Attribute AttributeId="urn:arc:subject:voms-attribute" Type="string">urn:mace:shibboleth:examples</Attribute>
  */
  RequestAttribute(Arc::XMLNode& node, AttributeFactory* attrfactory);

  RequestAttribute();

  virtual ~RequestAttribute();
  
public:
  Arc::XMLNode getNode();
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
  
  /**Duplicate the parameter into "this"*/
  RequestAttribute& duplicate(RequestAttribute&);

//protect:
  //AttrValList avlist;

private:
 static Arc::Logger logger;

 /**the <Attribute> node*/
 Arc::XMLNode node_;

 /**id of this <Attribute>, it could be useful if the policy specify <AttributeDesignator> to get value from request*/
 std::string id;

 /**data type of <Attribute> node, it a important factor for generating the different AttributeValue objects*/
 std::string type;

 /**issuer of the value of <Attribute>; it could be useful if the policy */
 std::string issuer;

 //AttrValList avlist;

 /**the AttributeValue object*/
 AttributeValue* attrval;

 /**the AttributeFactory which is used to generate the AttributeValue object*/
 AttributeFactory* attrfactory;
};

} // namespace ArcSec

#endif /* __ARC_SEC_REQUESTATTRIBUTE_H__ */
