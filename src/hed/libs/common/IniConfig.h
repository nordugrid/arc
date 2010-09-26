// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_INICONFIG_H__
#define __ARC_INICONFIG_H__

#include <string>

#include <arc/ArcConfig.h>
#include <arc/XMLNode.h>

namespace Arc {

  class IniConfig
    : public XMLNode {
  public:
    // Dummy constructor.
    IniConfig() : XMLNode(NS(), "IniConfig") {}
    IniConfig(const std::string& filename);
    ~IniConfig();
    bool Evaluate(Config &cfg);
  };

} // namespace Arc

#endif /* __ARC_INICONFIG_H__ */
