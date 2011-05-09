#include <string>
#include <list>
#include <vector>

#include <string.h>

#include <arc/ArcConfig.h>
#include <arc/message/Message.h>
#include <arc/message/SecHandler.h>

namespace ArcSHCLegacy {

class LegacySecHandler : public ArcSec::SecHandler {
 private:
  std::list<std::string> conf_files_;

 public:
  LegacySecHandler(Arc::Config *cfg, Arc::ChainContext* ctx);
  virtual ~LegacySecHandler(void);
  static Arc::Plugin* get_sechandler(Arc::PluginArgument* arg);
  virtual bool Handle(Arc::Message* msg) const;
  operator bool(void) { return (conf_files_.size() > 0); };
  bool operator!(void) { return (conf_files_.size() <= 0); };
};


} // namespace ArcSHCLegacy

