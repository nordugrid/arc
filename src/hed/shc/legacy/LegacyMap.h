#include <string>
#include <list>
#include <vector>

#include <string.h>

#include <arc/ArcConfig.h>
#include <arc/message/Message.h>
#include <arc/message/SecHandler.h>

namespace ArcSHCLegacy {

class LegacyMap : public ArcSec::SecHandler {
 friend class LegacyMapCP;
 private:
  class cfgfile {
   public:
    std::string filename;
    std::list<std::string> blocknames;
    cfgfile(const std::string& fname):filename(fname) {};
  };
  std::list<cfgfile> blocks_;
 public:
  LegacyMap(Arc::Config *cfg, Arc::ChainContext* ctx);
  virtual ~LegacyMap(void);
  static Arc::Plugin* get_sechandler(Arc::PluginArgument* arg);
  virtual bool Handle(Arc::Message* msg) const;
  operator bool(void) { return (blocks_.size()>0); };
  bool operator!(void) { return (blocks_.size()<=0); };
};


} // namespace ArcSHCLegacy

