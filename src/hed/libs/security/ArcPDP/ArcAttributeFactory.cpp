#include "ArcAttributeFactory.h"
#include "AttributeProxy.h"

void ArcAttributeFactory::initDatatypes(){
  /**Some Arc specified attribute types*/
  apmap.insert(pair<std::string, AttributeValue*>(StringAttribute.identify, new ArcAttributeProxy<StringAttribute>));
  apmap.insert(pair<std::string, AttributeValue*>(DateAttribute.identify, new ArcAttributeProxy<DateAttribute>));
  /** TODO:  other datetype............. */

}

ArcAttributeFactory::ArcAttributeFactory(){
  initDatatypes();
}

AttributeValue* ArcAttributeFactory::createValue(const XMLNode& node, const std::string& type){
  AttrProxyMap::iterator it; 
  if((it=apmap.find(type)) != apmap.end())
    return ((*it).second)->getAttribute(node);
  else return NULL;
}

AttributeFactory::~AttributeFactory(){
  AttrProxyMap::iterator it;
  for(it = apmap.begin(); it != apmap.end(); it++){
    delete (*it).second;
    apmap.erase(it);
  }
}
