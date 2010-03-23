#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/security/ClassLoader.h>

#include <arc/security/ArcPDP/attr/AttributeProxy.h>
#include <arc/security/ArcPDP/attr/StringAttribute.h>
#include <arc/security/ArcPDP/attr/DateTimeAttribute.h>
#include <arc/security/ArcPDP/attr/X500NameAttribute.h>
#include <arc/security/ArcPDP/attr/AnyURIAttribute.h>
#include <arc/security/ArcPDP/attr/GenericAttribute.h>

#include "XACMLAttributeProxy.h"
#include "XACMLAttributeFactory.h"

using namespace Arc;
namespace ArcSec {

Arc::Plugin* get_xacmlpdp_attr_factory (Arc::PluginArgument*) {
    return new ArcSec::XACMLAttributeFactory();
}

void XACMLAttributeFactory::initDatatypes(){
  //Some XACML specified attribute types
  apmap.insert(std::pair<std::string, AttributeProxy*>(StringAttribute::getIdentifier(), new XACMLAttributeProxy<StringAttribute>));
  apmap.insert(std::pair<std::string, AttributeProxy*>(DateTimeAttribute::getIdentifier(), new XACMLAttributeProxy<DateTimeAttribute>));
  apmap.insert(std::pair<std::string, AttributeProxy*>(DateAttribute::getIdentifier(), new XACMLAttributeProxy<DateAttribute>));
  apmap.insert(std::pair<std::string, AttributeProxy*>(TimeAttribute::getIdentifier(), new XACMLAttributeProxy<TimeAttribute>));
  apmap.insert(std::pair<std::string, AttributeProxy*>(DurationAttribute::getIdentifier(), new XACMLAttributeProxy<DurationAttribute>));
  apmap.insert(std::pair<std::string, AttributeProxy*>(PeriodAttribute::getIdentifier(), new XACMLAttributeProxy<PeriodAttribute>));
  apmap.insert(std::pair<std::string, AttributeProxy*>(X500NameAttribute::getIdentifier(), new XACMLAttributeProxy<X500NameAttribute>));
  apmap.insert(std::pair<std::string, AttributeProxy*>(AnyURIAttribute::getIdentifier(), new XACMLAttributeProxy<AnyURIAttribute>));
  apmap.insert(std::pair<std::string, AttributeProxy*>(GenericAttribute::getIdentifier(), new XACMLAttributeProxy<GenericAttribute>));

 /** TODO:  other datatype............. */

}

XACMLAttributeFactory::XACMLAttributeFactory(){
  initDatatypes();
}

AttributeValue* XACMLAttributeFactory::createValue(const XMLNode& node, const std::string& type){
  AttrProxyMap::iterator it;
  if((it=apmap.find(type)) != apmap.end())
    return ((*it).second)->getAttribute(node);
#if 0
  // This may look like hack, but generic attribute needs special treatment
  std::string value;
  if((bool)(const_cast<XMLNode&>(node).Child())) {
    value = (std::string)(const_cast<XMLNode&>(node).Child());
  //<Attribute AttributeId="" DataType=""><AttributeValue>abc</AttributeValue></Attribute>
  } else {
    value = (std::string)node; 
  }
  //<AttributeValue DataType="">abc</AttributeValue>

  std::size_t start;
  start = value.find_first_not_of(" \n\r\t");
  value = value.substr(start);
  std::size_t end;
  end = value.find_last_not_of(" \n\r\t");
  value = value.substr(0, end+1);

  GenericAttribute* attr = new GenericAttribute(value,
            (std::string)(const_cast<XMLNode&>(node).Attribute("AttributeId")));
  attr->setType(type);
  return attr;
#endif

  //For generic attributes, treat them as string
  if((it=apmap.find("string")) != apmap.end())
    return ((*it).second)->getAttribute(node);
  return NULL;
}

XACMLAttributeFactory::~XACMLAttributeFactory(){
  AttrProxyMap::iterator it;
  for(it = apmap.begin(); it != apmap.end(); it = apmap.begin()){
    AttributeProxy* attrproxy = (*it).second;
    if(attrproxy) delete attrproxy;
  }
}

} // namespace ArcSec

