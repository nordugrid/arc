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
    IniConfig(const std::string& filename);
    ~IniConfig();
    Config Evaluate();
  };

} // namespace Arc

#endif /* __ARC_INICONFIG_H__ */
