#ifndef __GM_CORE_CONFIG_H__
#define __GM_CORE_CONFIG_H__

#include <arc/Logger.h>
#include <arc/ArcConfigFile.h>

namespace ARex {

class GMConfig;

/// Parses configuration and fills GMConfig with information
class CoreConfig {
public:
  /// Parse config
  static bool ParseConf(GMConfig& config);
private:
  /// Parse ini-style config from stream cfile
  static bool ParseConfINI(GMConfig& config, Arc::ConfigFile& cfile);
  /// Function to check that LRMS scripts are available
  static void CheckLRMSBackends(const std::string& default_lrms);
  /// Function handle yes/no config commands
  static bool CheckYesNoCommand(bool& config_param, const std::string& name, std::string& rest);
  /// Logger
  static Arc::Logger logger;
};

} // namespace ARex

#endif // __GM_CORE_CONFIG_H__
