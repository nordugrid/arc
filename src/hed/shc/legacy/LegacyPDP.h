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
  class cfgblock {
   public:
    std::string name;
    std::list<std::string> groups;
    bool exists;
    bool limited;
    cfgblock(const std::string& n):name(n),exists(false),limited(false) { };
  };
  class cfgfile {
   public:
    std::string filename;
    std::list<cfgblock> blocks;
    cfgfile(const std::string& fname):filename(fname) {};
  };
  bool any_;
  std::list<std::string> groups_;
  std::list<std::string> vos_;
 public:
  static Arc::Plugin* get_pdp(Arc::PluginArgument *arg);
  LegacyPDP(Arc::Config* cfg, Arc::PluginArgument* parg);
  virtual ~LegacyPDP();
  virtual ArcSec::PDPStatus isPermitted(Arc::Message *msg) const;
};

} // namespace ArcSHCLegacy
