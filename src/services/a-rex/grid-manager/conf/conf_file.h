#ifndef __GM_CORE_CONFIG_H__
#define __GM_CORE_CONFIG_H__

#include <arc/Logger.h>

class GMConfig;
namespace Arc {
  class XMLNode;
}

/// Parses configuration and fills GMConfig with information
class CoreConfig {
public:
  /// Parse config, either ini-style or XML
  static bool ParseConf(GMConfig& config);
private:
  /// Parse ini-style config from stream cfile
  static bool ParseConfINI(GMConfig& config, std::ifstream& cfile);
  /// Parse config from XML node
  static bool ParseConfXML(GMConfig& config, const Arc::XMLNode& cfg);
  /// Function to check that LRMS scripts are available
  static void CheckLRMSBackends(const std::string& default_lrms);
  /// Function handle yes/no config commands
  static bool CheckYesNoCommand(bool& config_param, const std::string& name, std::string& rest);
  /// Logger
  static Arc::Logger logger;
};

#endif // __GM_CORE_CONFIG_H__
