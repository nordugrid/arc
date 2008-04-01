#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <iostream>
#include <string>
#include "ArcConfig.h"

namespace Arc {

Config::Config(const char *filename)
{
    ReadFromFile(filename);
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
    ReadFromFile(filename);
}

Config::Config(long cfg_ptr_addr)
{
    Config *cfg = (Config *)cfg_ptr_addr;
    cfg->New(*this);
}

Config::Config(const Config &cfg) : XMLNode()
{
    cfg.New(*this);
}

} // namespace Arc
