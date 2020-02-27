#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include <arc/Utils.h>

#include "LegacySecAttr.h"
#include "auth.h"
#include "ConfigParser.h"

#include "LegacySecHandler.h"

namespace ArcSHCLegacy {

Arc::Plugin* LegacySecHandler::get_sechandler(Arc::PluginArgument* arg) {
  ArcSec::SecHandlerPluginArgument* shcarg =
            arg?dynamic_cast<ArcSec::SecHandlerPluginArgument*>(arg):NULL;
  if(!shcarg) return NULL;
  LegacySecHandler* plugin = new LegacySecHandler((Arc::Config*)(*shcarg),(Arc::ChainContext*)(*shcarg),arg);
  if(!plugin) return NULL;
  if(!(*plugin)) { delete plugin; plugin = NULL; };
  return plugin;
}

LegacySecHandler::LegacySecHandler(Arc::Config *cfg,Arc::ChainContext* ctx,Arc::PluginArgument* parg):SecHandler(cfg,parg),attrname_("ARCLEGACY") {
  Arc::XMLNode attrname = (*cfg)["AttrName"];
  if((bool)attrname) {
    attrname_ = (std::string)attrname;
  };
  Arc::XMLNode conf_file = (*cfg)["ConfigFile"];
  while((bool)conf_file) {
    std::string filename = (std::string)conf_file;
    if(!filename.empty()) {
      conf_files_.push_back(filename);
    };
    ++conf_file;
  };
  if(conf_files_.size() <= 0) {
    logger.msg(Arc::ERROR, "LegacySecHandler: configuration file not specified");
  };
}

LegacySecHandler::~LegacySecHandler(void) {
}

class LegacySHCP: public ConfigParser {
 public:
  LegacySHCP(const std::string& filename, Arc::Logger& logger, AuthUser& auth/*, LegacySecAttr& sattr*/):
    ConfigParser(filename,logger),auth_(auth)/*,sattr_(sattr)*/,group_match_(0),vo_match_(false) {
  };

  virtual ~LegacySHCP(void) {
  };

 protected:
  virtual bool BlockStart(const std::string& id, const std::string& name) {
    group_match_ = AAA_NO_MATCH;
    group_name_ = "";
    vo_match_ = false;
    vo_name_ = "";
    return true;
  };

  virtual bool BlockEnd(const std::string& id, const std::string& name) {
    if(id == "authgroup") {
      if(group_name_.empty()) group_name_ = name;
      if((group_match_ == AAA_POSITIVE_MATCH) && !group_name_.empty()) {
        auth_.add_group(group_name_);
      };
    } else if(id == "userlist") {
      if(vo_name_.empty()) vo_name_ = name;
      if(vo_match_ && !vo_name_.empty()) {
        auth_.add_vo(vo_name_);
      };
    };
    return true;
  };

  virtual bool ConfigLine(const std::string& id, const std::string& name, const std::string& cmd, const std::string& line) {
    if(id == "authgroup") {
      if(group_match_ == AAA_NO_MATCH) {
        if(cmd == "name") {
          group_name_ = line;
        } else {
          group_match_ = auth_.evaluate((cmd + " " + line).c_str());
        };
      };
    } else if(id == "userlist") {
      if(!vo_match_) {
        if(cmd == "outfile") {
          if(!line.empty()) {
            // Because file=filename looks exactly like 
            // matching rule evaluate() can be used
            int r = auth_.evaluate((std::string("file ") + line).c_str());
            vo_match_ = (r == AAA_POSITIVE_MATCH);
          };
        } else if(cmd == "name") {
          vo_name_ = line;
        };
      };
    };
    return true;
  };

 private:
  AuthUser& auth_;
  /* LegacySecAttr& sattr_; */
  int group_match_;
  std::string group_name_;
  bool vo_match_;
  std::string vo_name_;
};


ArcSec::SecHandlerStatus LegacySecHandler::Handle(Arc::Message* msg) const {
  if(conf_files_.size() <= 0) {
    logger.msg(Arc::ERROR, "LegacySecHandler: configuration file not specified");
    return false;
  };
  Arc::SecAttr* attr = msg->AuthContext()->get(attrname_);
  if(attr) {
    LegacySecAttr* lattr = dynamic_cast<LegacySecAttr*>(attr);
    if(lattr) {
      // Information already collected
      return true;
    };
  };
  AuthUser auth(*msg);
  Arc::AutoPointer<LegacySecAttr> sattr(new LegacySecAttr(logger));
  for(std::list<std::string>::const_iterator conf_file = conf_files_.begin();
                             conf_file != conf_files_.end();++conf_file) {
    LegacySHCP parser(*conf_file,logger,auth /*,*sattr*/);
    if(!parser) return false;
    if(!parser.Parse()) return false;
  };
  // Pass all matched groups and VOs to LegacySecAttr
  {
    const std::list<std::string>& vos = auth.VOs();
    for(std::list<std::string>::const_iterator vo = vos.begin();
                                 vo != vos.end(); ++vo) sattr->AddVO(*vo);
  };
  {
    std::list<std::string> groups;
    auth.get_groups(groups);
    for(std::list<std::string>::const_iterator grp = groups.begin(); grp != groups.end(); ++grp) {
      const char* vo = auth.get_group_vo(*grp);
      const voms_t* voms = auth.get_group_voms(*grp);
      const scitokens_t* scitokens = auth.get_group_scitokens(*grp);
      //std::string glid = auth.get_group_globalid(*grp);
      std::list<std::string> vos;
      std::list<std::string> vomss;
      std::list<std::string> scitokenss;
      if((vo != NULL) && (*vo != '\0')) vos.push_back(vo);
      if(voms != NULL) {
        for(std::vector<voms_fqan_t>::const_iterator f = voms->fqans.begin();
                                           f != voms->fqans.end(); ++f) {
          std::string fqan;
          f->str(fqan);
          vomss.push_back(fqan);
        };
      };
      // We need something like fqan for tokens. Currently we only need to identify cleint.
      // For that combination of subject and issuer is enough.
      if(scitokens) {
        if(!scitokens->subject.empty() && !scitokens->issuer.empty()) {
          scitokenss.push_back(scitokens->issuer + "/" + scitokens->subject);
        };
      };
      sattr->AddGroup(*grp, vos, vomss, scitokenss);
    };
  };


  // Pass all matched groups and VOs to Message in SecAttr
  msg->AuthContext()->set(attrname_,sattr.Release());
  return true;
}


} // namespace ArcSHCLegacy

