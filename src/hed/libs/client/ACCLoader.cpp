// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include <arc/client/UserConfig.h>

#include "ACCLoader.h"

namespace Arc {

  ACCLoader::ACCLoader(const Config& cfg)
    : Loader(cfg) {
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

  ACC* ACCLoader::loadACC(const std::string& name, const UserConfig& ucfg) {
    if (name.empty()) return NULL;
    
    std::string id = "loadACCid_1";
    for (int i = 2; accs_.find(id) != accs_.end(); i++) {
      id = "loadACCid_" + tostring(i);
    }
    
    Config cfg(XMLNode(NS(), "ArcClientComponent"));
    cfg.NewAttribute("name") = name;
    cfg.NewAttribute("id") = id;
    if (ucfg.ConfTree()["Broker"]["Arguments"])
      cfg.NewChild("Arguments") = (std::string)ucfg.ConfTree()["Broker"]["Arguments"];
    ucfg.ApplyToConfig(cfg);

    ACCPluginArgument arg(&cfg);

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

  ACC* ACCLoader::getACC(const std::string& id) {
    acc_container_t::iterator acc = accs_.find(id);
    if (acc != accs_.end())
      return acc->second;
    return NULL;
  }

} // namespace Arc
