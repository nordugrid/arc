// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include <arc/client/UserConfig.h>

#include "ACCLoader.h"

namespace Arc {

  ACCLoader::ACCLoader()
    : Loader(ACCConfig().MakeConfig(Config()).Parent()) {}

  ACCLoader::ACCLoader(const Config& cfg)
    : Loader(ACCConfig().MakeConfig(cfg).Parent()) {
    make_elements(cfg);
  }

  ACCLoader::~ACCLoader(void) {
    for (acc_container_t::iterator acc_i = accs_.begin();
         acc_i != accs_.end(); acc_i = accs_.begin()) {
      ACC *acc = acc_i->second;
      accs_.erase(acc_i);
      if (acc)
        delete acc;
    }
  }

  void ACCLoader::make_elements(const Config& cfg) {
    for (int i = 0; cfg.Child(i); i++) {
      Config cfg_(cfg.Child(i), cfg.getFileName());

      if (MatchXMLName(cfg.Child(i), "ArcClientComponent")) {
        std::string name = cfg.Child(i).Attribute("name");
        if (name.empty()) {
          logger.msg(ERROR, "ArcClientComponent has no name attribute defined");
          continue;
        }
        std::string id = cfg.Child(i).Attribute("id");
        if (id.empty()) {
          logger.msg(ERROR, "ArcClientComponent has no id attribute defined");
          continue;
        }
        if (accs_.find(id) != accs_.end()) {
          logger.msg(ERROR, "The id (%s) of the ACC %s is identical another ACC", id, name);
          continue;
        }
        ACCPluginArgument arg(&cfg_);
        ACC *acc = factory_->GetInstance<ACC>(ACCPluginKind, name, &arg);
        if (!acc) {
          logger.msg(ERROR, "ArcClientComponent %s(%s) could not be created",
                     name, id);
          continue;
        }
        accs_[id] = acc;
        logger.msg(INFO, "Loaded ArcClientComponent %s(%s)", name, id);
        continue;
      }

      //logger.msg(WARNING, "Unknown element \"%s\" - ignoring", cn.Name());
    }
  }

  ACC* ACCLoader::loadACC(const std::string& name, const XMLNode * cfg) {
    if (name.empty()) return NULL;
    
    std::string id = "loadACCid_1";
    for (int i = 2; accs_.find(id) != accs_.end(); i++) {
      id = "loadACCid_" + tostring(i);
    }

    // It seems that two Config objects are needed when creating the config
    // object which should be passed to the ACCPlugin.
    Config acccfg_;
    Config acccfg = acccfg_.NewChild("ArcClientComponent");
    acccfg.NewAttribute("name") = name;
    acccfg.NewAttribute("id") = id;
    if (cfg)
      for (int i = 0; cfg->Child(i); i++)
        acccfg.NewChild(cfg->Child(i));

    ACCPluginArgument arg(&acccfg);

    ACC *acc = factory_->GetInstance<ACC>(ACCPluginKind, name, &arg);
    if (!acc) {
      logger.msg(ERROR, "ArcClientComponent %s(%s) could not be created",
                 name, id);
      return NULL;
    }
    accs_[id] = acc;
    logger.msg(INFO, "Loaded ArcClientComponent %s(%s)", name, id);

    return acc;
  }

  ACC* ACCLoader::getACC(const std::string& id) const {
    acc_container_t::const_iterator acc = accs_.find(id);
    if (acc != accs_.end())
      return acc->second;
    return NULL;
  }

} // namespace Arc
