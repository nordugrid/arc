#include "ArcRequestAttribute.h"

RequestAttribute::RequestAttribute(const XMLNode& node, const AttributeFactory* attrfactory){
  XMLNode nd;
  if(!(nd=node.Attribute("AttributeID"))){
    id = (std::sting)nd;
  }
  if(!(nd=node.Attribute("Type"))){
    type = (std::sting)nd;
  }
  if(!(nd=node.Attribute("Issuer"))){
    issuer = (std::sting)nd;
  }

  if(!(node.Size())){
    avlist.push_back(attrfactory->createValue(node, type));
  }
  else{
    for(int i=0; i<node.Size(); i++)
    avlist.push_back(attrfactory->createValue(node.Child(i), type));
  }
}

std::string RequestAttribute::getAttributeId () const{
 
}

void RequestAttribute::setAttributeId (const std::string attributeId){
}

std::string RequestAttribute::getDataType () const{
}

void RequestAttribute::setDataType (const std::string dataType){
}

std::string RequestAttribute::getIssuer () const{
}

void RequestAttribute::setIssuer (const std::string issuer){
}

AttrValList RequestAttribute::getAttributeValueList () const{
}

void RequestAttribute::setAttributeValueList (const AttrValList& attributeValueList){
}


