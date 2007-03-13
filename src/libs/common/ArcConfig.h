#ifndef __ARC_CONFIG_H__
#define __ARC_CONFIG_H__

#include <string>
#include <list>
#include "XMLNode.h"

namespace Arc {

class Config: public XMLNode {
    public:
        Config() { };
        Config(const char *filename);
        Config(const std::string &xml_str): XMLNode(xml_str) { };
        Config(XMLNode xml): XMLNode(xml) { };
        ~Config(void);
        void print(void);
};

}; // namespace Arc 

#endif /* __ARC_CONFIG_H__ */
