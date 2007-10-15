#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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

    std::cout << "Get the \"child2\" node with namespace \"uri:bbb\" inside the root node" << std::endl;

    Arc::NS nsList;
    nsList.insert(std::pair<std::string, std::string>("test","uri:bbb"));
    Arc::XMLNode root(xml_str);
    std::list<Arc::XMLNode> list = root.XPathLookup("//test:child2", nsList);
    std::list<Arc::XMLNode>::iterator it;
    for ( it=list.begin() ; it != list.end(); it++ ){
      std::cout << (*it).FullName() << ":" << (std::string)(*it) << std::endl;       
    }
    
    std::cout << "************************" << std::endl;
    
    std::cout << "Get the \"child2\" node with namespace \"uri:ccc\" inside the root node" << std::endl;

    nsList.erase("test");
    nsList.insert(std::pair<std::string, std::string>("test1","uri:ccc"));
    list = root.XPathLookup("//test1:child2", nsList);
    for ( it=list.begin() ; it != list.end(); it++ ){
      std::cout << (*it).FullName() << ":" << (std::string)(*it) << std::endl;
    }

    std::cout << "************************" << std::endl;
   
    std::cout << "Get the \"parent\" node with namespace \"uri:bbb\" inside the root node" << std::endl;

    nsList.erase("test1");
    nsList.insert(std::pair<std::string, std::string>("test2","uri:bbb"));
    list = root.XPathLookup("//test2:parent", nsList);
    Arc::XMLNode tmp;
    for ( it=list.begin() ; it != list.end(); it++ ){
      tmp=*it;
      std::cout << (*it).FullName() << ":" << (std::string)(*it) << std::endl;
    }   

    tmp = root.Child();
    std::string tmpndstr;
    tmp.GetXML(tmpndstr);

    std::cout << "************************" << std::endl;
   
    std::cout<<"Now the current node is: "<< tmpndstr <<std::endl;

    std::cout<<"Looking for the \"child2\" node with namespace \"uri:ccc\"" <<std::endl;

    nsList.erase("test2");
    nsList.insert(std::pair<std::string, std::string>("test3","uri:ccc"));
    list = tmp.XPathLookup("//test3:child2", nsList);
    for ( it=list.begin() ; it != list.end(); it++ ){
      std::cout << (*it).FullName() << ":" << (std::string)(*it) << std::endl;
      std::cout <<"Can get the node by search from the non-root node"<< std::endl;
    }
    
    return 0;
}

