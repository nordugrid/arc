#include "ArcConfig.h"
#include "XMLNode.h"
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <map>


int main(void)
{
    std::string xml_str = "";
    std::string str;
    std::ifstream f("testxpath.xml");

    // load content of file
    while (f >> str) {
        xml_str.append(str);
        xml_str.append(" ");
    }
    f.close();

    Arc::NS nsList;
    nsList.insert(std::pair<std::string, std::string>("test","uri:bbb"));
    std::list<Arc::XMLNode> list = Arc::XPathLookup(xml_str, (xmlChar*)"//test:child2", nsList);
    std::list<Arc::XMLNode>::iterator it;
    Arc::XMLNode* node;
    for ( it=list.begin() ; it != list.end(); it++ ){
      std::cout << (*it).Name() << std::endl;       
    }
    
    std::cout << "************************" << std::endl;
    
    nsList.erase("test");
    nsList.insert(std::pair<std::string, std::string>("test1","uri:ccc"));
    list = Arc::XPathLookup(xml_str, (xmlChar*)"//test1:child2", nsList);
    for ( it=list.begin() ; it != list.end(); it++ ){
      std::cout << (*it).Name() << std::endl;
    }
    
    return 0;
}

