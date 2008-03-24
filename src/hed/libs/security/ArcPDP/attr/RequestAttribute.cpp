#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "RequestAttribute.h"

using namespace Arc;
using namespace ArcSec;

Logger RequestAttribute::logger(Logger::rootLogger, "RequestAttribute");

RequestAttribute::RequestAttribute(XMLNode& node, AttributeFactory* attrfactory) : attrval(NULL), attrfactory(attrfactory) {
  Arc::XMLNode nd;

  //Get the attribute of the node  
  id = (std::string)(node.Attribute("AttributeId")); 

  std::string tp = (std::string)(node.Attribute("Type"));
  if(tp.empty())
    tp = (std::string)(node.Attribute("DataType"));
  size_t found = tp.find_last_of(":");
  type = tp.substr(found+1);

  issuer = (std::string)(node.Attribute("Issuer"));

  //Create the attribute value object according to the data type
  attrval = attrfactory->createValue(node, type);

  if(attrval == NULL)
    logger.msg(ERROR,"No Attribute exists, which can deal with type: %s", type);

  logger.msg(DEBUG, "Id= %s,Type= %s,Issuer= %s,Value= %s",id, type, issuer, attrval->encode());

  //Copy the node parameter into this->node_, for the usage in duplicate method 
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
  node_ = req_attr.getNode();
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

