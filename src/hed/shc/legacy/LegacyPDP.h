#include <string>
#include <list>

#include <string.h>

#include <arc/ArcConfig.h>
#include <arc/message/Message.h>
#include <arc/security/PDP.h>

/**
 * Match authorization information collected by ArcLegacyHandler and 
 * stored in ARCLEGACY sec. attribute and provide decision based on configuration.
 * Decision is stored in message context sec. attribute under ARCLEGACYPDP tag.
 * For storing decision LegacyPDPAttr class is used.
 */
namespace ArcSHCLegacy {

class LegacyPDP : public ArcSec::PDP {
 friend class LegacyPDPCP;
 private:
  class cfgblock {
   public:
    std::string name;
    std::list<std::pair<bool,std::string> > groups;
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
  std::list<std::pair<bool,std::string> > groups_;
  std::list<std::string> vos_;
  std::string attrname_;
  std::string srcname_;
 public:
  static Arc::Plugin* get_pdp(Arc::PluginArgument *arg);
  LegacyPDP(Arc::Config* cfg, Arc::PluginArgument* parg);
  virtual ~LegacyPDP();
  virtual ArcSec::PDPStatus isPermitted(Arc::Message *msg) const;
};

} // namespace ArcSHCLegacy
