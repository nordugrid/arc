#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <iostream>
#include "ArcConfig.h"

namespace Arc {
Config::Config(const char *filename)
{
    std::string xml_str = "";
    std::string str;
    std::ifstream f(filename);
    
    // load content of file
    while (f >> str) {
        xml_str.append(str);
        xml_str.append(" ");
    }
    f.close();
    // create XMLNode
    xmlDocPtr doc = xmlReadMemory(xml_str.c_str(),xml_str.length(),NULL,NULL,0);
    if(!doc) return;
    node_=(xmlNodePtr)doc;
    is_owner_=true;
}

Config::~Config(void) 
{
    // NOP
}

static void _print(XMLNode &node, int skip) {
    int n;
    for(n = 0; n < skip; n++) {
        std::cout << " ";
    }
    std::string content = (std::string)node;
    std::cout << "* " << node.Name() << "(" << node.Size() << ")" << " = " << content << std::endl;
    for (n = 0;;n++) {
        XMLNode _node = node.Child(n);
        if (!_node) {
            break;
        }
        _print(_node, skip+2);
    }
}

void Config::print(void)
{
    _print(*((XMLNode *)this), 0);
}

};
