#include "ArcAttributeFactory.h"
#include "AttributeProxy.h"

void ArcAttributeFactory::initDatatypes(){
  /**Some Arc specified attribute types*/
  apmap.insert(pair<std::string, AttributeProxy*>(StringAttribute.identify, new ArcAttributeProxy<StringAttribute>));
  apmap.insert(pair<std::string, AttributeProxy*>(DateAttribute.identify, new ArcAttributeProxy<DateAttribute>));
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
