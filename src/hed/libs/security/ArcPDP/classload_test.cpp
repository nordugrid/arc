#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "../../../libs/loader/ClassLoader.h"
#include "../../../libs/loader/LoadableClass.h"
#include <arc/ArcConfig.h>
#include <arc/XMLNode.h>
#include "attr/ArcAttributeFactory.h"
#include "attr/AttributeValue.h"
int main(void)
{
    Arc::Config cfg("plugins.xml");
    Arc::ClassLoader classloader(&cfg);
    std::string id = "attr.factory";
    Arc::ArcAttributeFactory* attrfactory=NULL;
    attrfactory = dynamic_cast<Arc::ArcAttributeFactory*>(classloader.Instance(id, &cfg));

    if(attrfactory == NULL)
      std::cout<<"Can not dynamic produce ArcAttributeFactory!!"<<std::endl;
   
    Arc::XMLNode nd = cfg["ArcConfig"]["Plugins"]["Plugin"]; 
    Arc::AttributeValue * val;
    val = attrfactory->createValue(nd,"string");
    std::string tmp1 = val->encode();
    std::cout<<tmp1<<std::endl;
    
    delete attrfactory;
     
    
    Arc::ClassLoader classloader1(NULL);
    id = "attr.factory";
    attrfactory=NULL;
    attrfactory = dynamic_cast<Arc::ArcAttributeFactory*>(classloader.Instance(id, &cfg));

    if(attrfactory == NULL)
      std::cout<<"Can not dynamic produce ArcAttributeFactory!!"<<std::endl;
 
    val = attrfactory->createValue(nd,"string");
    std::string tmp2 = val->encode();
    std::cout<<tmp2<<std::endl;

 
    delete attrfactory;
    return 0;

}
