#include <string>
#include <list>
#include <vector>

#include <string.h>

#include <arc/ArcConfig.h>
#include <arc/message/Message.h>
#include <arc/message/SecHandler.h>

namespace ArcSHCLegacy {

/**
 Processes configuration and evaluates groups to which requestor belongs.
 Obtained result is stored in message context as LegacySecAttr security 
 attribute under ARCLEGACY tag.
*/
class LegacySecHandler : public ArcSec::SecHandler {
 private:
  std::list<std::string> conf_files_;

 public:
  LegacySecHandler(Arc::Config *cfg, Arc::ChainContext* ctx, Arc::PluginArgument* parg);
  virtual ~LegacySecHandler(void);
  static Arc::Plugin* get_sechandler(Arc::PluginArgument* arg);
  virtual ArcSec::SecHandlerStatus Handle(Arc::Message* msg) const;
  operator bool(void) { return (conf_files_.size() > 0); };
  bool operator!(void) { return (conf_files_.size() <= 0); };
};


} // namespace ArcSHCLegacy

