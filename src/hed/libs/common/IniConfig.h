// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_INICONFIG_H__
#define __ARC_INICONFIG_H__

#include <string>

#include <arc/XMLNode.h>

namespace Arc {

  class IniConfig
    : public XMLNode {
  public:
    IniConfig(const std::string& filename);
    ~IniConfig();
  };

} // namespace Arc

#endif /* __ARC_INICONFIG_H__ */
