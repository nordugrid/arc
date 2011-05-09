#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <arc/Logger.h>

namespace ArcSHCLegacy {

class ConfigParser {
 public:
  ConfigParser(const std::string& filename, Arc::Logger& logger);
  virtual ~ConfigParser(void);
  bool Parse(void);
  operator bool(void) { return (bool)f_; };
  bool operator!(void) { return !(bool)f_; };
 protected:
  virtual bool BlockStart(const std::string& id, const std::string& name) = 0;
  virtual bool BlockEnd(const std::string& id, const std::string& name) = 0;
  virtual bool ConfigLine(const std::string& id, const std::string& name, const std::string& cmd, const std::string& line) = 0;
  Arc::Logger& logger_;
 private:
  std::string block_id_;
  std::string block_name_;
  std::ifstream f_;
};

} // namespace ArcSHCLegacy

