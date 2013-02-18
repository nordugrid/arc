#include <string>
#include <list>

#include <string.h>

#include <arc/ArcConfig.h>
#include <arc/message/Message.h>
#include <arc/security/PDP.h>

namespace ArcSHCLegacy {

class LegacyPDP : public ArcSec::PDP {
 friend class LegacyPDPCP;
 private:
  class cfgfile {
   public:
    std::string filename;
    std::list<std::string> blocknames;
    cfgfile(const std::string& fname):filename(fname) {};
  };
  std::list<cfgfile> blocks_;
  std::list<std::string> groups_;
  std::list<std::string> vos_;
 public:
  static Arc::Plugin* get_pdp(Arc::PluginArgument *arg);
  LegacyPDP(Arc::Config* cfg, Arc::PluginArgument* parg);
  virtual ~LegacyPDP();
  virtual ArcSec::PDPStatus isPermitted(Arc::Message *msg) const;
};

} // namespace ArcSHCLegacy
