#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ArcRequestAttribute.h"

using namespace Arc;

ArcRequestAttribute::ArcRequestAttribute(XMLNode& node, AttributeFactory* attrfactory) : RequestAttribute(node, att
rfactory), attrval(NULL), attrfactory(attrfactory) {
  Arc::XMLNode nd;
  
  id = (std::string)(node.Attribute("AttributeId")); 

  type = (std::string)(node.Attribute("Type"));

  issuer = (std::string)(node.Attribute("Issuer"));

  attrval = attrfactory->createValue(node, type);

  if(attrval == NULL)
    logger.msg(ERROR,"No Attribute exists, which can deal with type: %s", type.c_str());

  logger.msg(DEBUG, "Id= %s,Type= %s,Issuer= %s,Value= %s",id.c_str(), type.c_str(), issuer.c_str(), (attrval->encode()).c_str());

  node.New(node_);

/*
  if(!(node.Size())){
    avlist.push_back(attrfactory->createValue(node, type));
  }
  else{
    for(int i=0; i<node.Size(); i++)
      avlist.push_back(attrfactory->createValue(node.Child(i), type));
  }
*/
}

ArcRequestAttribute::ArcRequestAttribute() {

}

XMLNode ArcRequestAttribute::getNode() {
  return node_;
}

std::string ArcRequestAttribute::getAttributeId () const{
  return id;
}

void ArcRequestAttribute::setAttributeId (const std::string attributeId){
  id = attributeId;
}

std::string ArcRequestAttribute::getDataType () const{
  return type;
}

void ArcRequestAttribute::setDataType (const std::string dataType){
  type = dataType;
}

std::string ArcRequestAttribute::getIssuer () const{
  return issuer;
}

void ArcRequestAttribute::setIssuer (const std::string is){
  issuer = is;
}

/*AttrValList ArcRequestAttribute::getAttributeValueList () const{
}

void ArcRequestAttribute::setAttributeValueList (const AttrValList& attributeValueList){
}
*/
AttributeValue* ArcRequestAttribute::getAttributeValue() const{
  return attrval;
}

AttributeFactory* ArcRequestAttribute::getAttributeFactory() const {
  return attrfactory;
}

RequestAttribute* ArcRequestAttribute::duplicate(RequestAttribute* req_attr) {
  id = req_attr->getAttributeId();
  type = req_attr->getDataType();
  issuer = req_attr->getIssuer();
  node_ = (req_attr->getNode()).Child();
  attrval = (req_attr->getAttributeFactory())->createValue(node_, type);
  return this;
}


ArcRequestAttribute::~ArcRequestAttribute(){
  if(attrval)
    delete attrval;
  /*while(!avlist.empty()){
    delete (*(avlist.back()));
    avlist.pop_back();
  }
  */
}

