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

    //Get the "chile2" node with namespace "uri:bbb" inside the root node
    Arc::NS nsList;
    nsList.insert(std::pair<std::string, std::string>("test","uri:bbb"));
    Arc::XMLNode doc(xml_str);
    std::list<Arc::XMLNode> list = doc.XPathLookup("//test:child2", nsList);
    std::list<Arc::XMLNode>::iterator it;
    for ( it=list.begin() ; it != list.end(); it++ ){
      std::cout << (*it).Name() << std::endl;       
    }
    
    std::cout << "************************" << std::endl;
    
    //Get the "child2" node with namespace "uri:ccc" inside the root node
    nsList.erase("test");
    nsList.insert(std::pair<std::string, std::string>("test1","uri:ccc"));
    list = doc.XPathLookup("//test1:child2", nsList);
    for ( it=list.begin() ; it != list.end(); it++ ){
      std::cout << (*it).Name() << std::endl;
    }

   
    //Get the "parent"node with namespace "uri:bbb" inside the root node
    nsList.erase("test1");
    nsList.insert(std::pair<std::string, std::string>("test2","uri:bbb"));
    list = doc.XPathLookup("//test2:parent", nsList);
    Arc::XMLNode tmp;
    for ( it=list.begin() ; it != list.end(); it++ ){
      tmp=*it;
      std::cout << (*it).Name() << std::endl;
    }   

    //Find the "child2" node with namespace "uri:ccc" and inside the "parent" node, does not work.
    nsList.erase("test2");
    nsList.insert(std::pair<std::string, std::string>("test3","uri:bbb"));
    list = tmp.XPathLookup("//test3:child1", nsList);      //To illustrate only "root" can call XPathLookup method
    for ( it=list.begin() ; it != list.end(); it++ ){
      std::cout << (*it).Name() << std::endl;
    }
    
    return 0;
}

