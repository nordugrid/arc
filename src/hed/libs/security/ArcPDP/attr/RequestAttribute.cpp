#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "RequestAttribute.h"

using namespace Arc;

Logger RequestAttribute::logger(Logger::rootLogger, "RequestAttribute");

RequestAttribute::RequestAttribute(XMLNode& node, AttributeFactory* attrfactory) : attrval(NULL), attrfactory(attrfactory) {
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

RequestAttribute::RequestAttribute() {

}

XMLNode RequestAttribute::getNode() {
  return node_;
}

std::string RequestAttribute::getAttributeId () const{
  return id;
}

void RequestAttribute::setAttributeId (const std::string attributeId){
  id = attributeId;
}

std::string RequestAttribute::getDataType () const{
  return type;
}

void RequestAttribute::setDataType (const std::string dataType){
  type = dataType;
}

std::string RequestAttribute::getIssuer () const{
  return issuer;
}

void RequestAttribute::setIssuer (const std::string is){
  issuer = is;
}

/*AttrValList RequestAttribute::getAttributeValueList () const{
}

void RequestAttribute::setAttributeValueList (const AttrValList& attributeValueList){
}
*/
AttributeValue* RequestAttribute::getAttributeValue() const{
  return attrval;
}

AttributeFactory* RequestAttribute::getAttributeFactory() const {
  return attrfactory;
}

RequestAttribute& RequestAttribute::duplicate(RequestAttribute& req_attr) {
  id = req_attr.getAttributeId();
  type = req_attr.getDataType();
  issuer = req_attr.getIssuer();
  node_ = (req_attr.getNode()).Child();
  attrval = (req_attr.getAttributeFactory())->createValue(node_, type);
  return *this;
}


RequestAttribute::~RequestAttribute(){
  if(attrval)
    delete attrval;
  /*while(!avlist.empty()){
    delete (*(avlist.back()));
    avlist.pop_back();
  }
  */
}

