#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/ClassLoader.h>

#include "ArcAttributeFactory.h"
#include "AttributeProxy.h"
#include "StringAttribute.h"
#include "DateTimeAttribute.h"
#include "X500NameAttribute.h"
#include "AnyURIAttribute.h"


static Arc::LoadableClass* get_attr_factory (Arc::Config *cfg) {
    return new Arc::ArcAttributeFactory();
}

loader_descriptors __arc_attrfactory_modules__  = {
    { "attr.factory", 0, &get_attr_factory },
    { NULL, 0, NULL }
};

using namespace Arc;

void ArcAttributeFactory::initDatatypes(){
  /**Some Arc specified attribute types*/
  apmap.insert(std::pair<std::string, AttributeProxy*>(StringAttribute::getIdentifier(), new ArcAttributeProxy<StringAttribute>));
  apmap.insert(std::pair<std::string, AttributeProxy*>(DateTimeAttribute::getIdentifier(), new ArcAttributeProxy<DateTimeAttribute>));
  apmap.insert(std::pair<std::string, AttributeProxy*>(DateAttribute::getIdentifier(), new ArcAttributeProxy<DateAttribute>));
  apmap.insert(std::pair<std::string, AttributeProxy*>(TimeAttribute::getIdentifier(), new ArcAttributeProxy<TimeAttribute>));
  apmap.insert(std::pair<std::string, AttributeProxy*>(DurationAttribute::getIdentifier(), new ArcAttributeProxy<DurationAttribute>));
  apmap.insert(std::pair<std::string, AttributeProxy*>(PeriodAttribute::getIdentifier(), new ArcAttributeProxy<PeriodAttribute>));
  apmap.insert(std::pair<std::string, AttributeProxy*>(X500NameAttribute::getIdentifier(), new ArcAttributeProxy<X500NameAttribute>));
  apmap.insert(std::pair<std::string, AttributeProxy*>(AnyURIAttribute::getIdentifier(), new ArcAttributeProxy<AnyURIAttribute>));

 /** TODO:  other datetype............. */

}

ArcAttributeFactory::ArcAttributeFactory(){
  initDatatypes();
}

/**creat a AttributeValue according to the value in the XML node and the type; It should be the caller to release the AttributeValue Object*/
AttributeValue* ArcAttributeFactory::createValue(const XMLNode& node, const std::string& type){
  AttrProxyMap::iterator it;
  if((it=apmap.find(type)) != apmap.end())
    return ((*it).second)->getAttribute(node);
  else return NULL;
}

ArcAttributeFactory::~ArcAttributeFactory(){
  AttrProxyMap::iterator it;
  for(it = apmap.begin(); it != apmap.end(); it++){
    delete (*it).second;
    apmap.erase(it);
  }
}
