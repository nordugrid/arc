// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_INICONFIG_H__
#define __ARC_INICONFIG_H__

#include <string>

#include <arc/ArcConfig.h>
#include <arc/XMLNode.h>

namespace Arc {

  /// Class representing "ini-style" configuration.
  /** It provides a way to convert configuration to XML for use with HED
   *  internally.
   *  \see Profile
   *  \headerfile IniConfig.h arc/IniConfig.h
   */
  class IniConfig
    : public XMLNode {
  public:
    /// Dummy constructor.
    IniConfig() : XMLNode(NS(), "IniConfig") {}
    /// Read configuration from specified filename.
    IniConfig(const std::string& filename);
    ~IniConfig();
    /// Evaluate configuration against the standard profile.
    bool Evaluate(Config &cfg);
  };

} // namespace Arc

#endif /* __ARC_INICONFIG_H__ */
