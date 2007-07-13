#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <iostream>
#include "ArcConfig.h"

namespace Arc {

static xmlDocPtr _parse(const char *filename) 
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
    return xmlReadMemory(xml_str.c_str(),xml_str.length(),NULL,NULL,0);
}

Config::Config(const char *filename)
{
    // create XMLNode
    xmlDocPtr doc = _parse(filename);
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

void Config::parse(const char *filename)
{
    // create XMLNode
    xmlDocPtr doc = _parse(filename);
    if(!doc) return;
    node_=(xmlNodePtr)doc;
    is_owner_=true;
}

} // namespace Arc
