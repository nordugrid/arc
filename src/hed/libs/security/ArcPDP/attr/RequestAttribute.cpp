#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "RequestAttribute.h"

using namespace Arc;

RequestAttribute::RequestAttribute(XMLNode& node, AttributeFactory* attrfactory){
  Arc::XMLNode nd;

  std::string xml;

  node.GetXML(xml);  //for testing
  std::cout<<"\n"<<xml<<std::endl;
  
  id = (std::string)(node.Attribute("AttributeId")); 

  type = (std::string)(node.Attribute("Type"));

  issuer = (std::string)(node.Attribute("Issuer"));

  attrval = attrfactory->createValue(node, type);

  if(attrval == NULL)
    std::cout<<"No Attribute exists, which can deal with type:"<<type<<std::endl;

  std::cout<<"Id--"<<id<<"  Type--"<<type<<"  Issurer--"<<issuer<<std::endl;

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


RequestAttribute::~RequestAttribute(){
  delete attrval;
  /*while(!avlist.empty()){
    delete (*(avlist.back()));
    avlist.pop_back();
  }
  */
}

