// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/data/DMC.h>

#include "DMCLoader.h"

namespace Arc {

  DMCLoader::DMCLoader(Config& cfg)
    : Loader(cfg) {
    make_elements(cfg);
  }

  DMCLoader::~DMCLoader(void) {
    for (dmc_container_t::iterator dmc_i = dmcs_.begin();
         dmc_i != dmcs_.end(); dmc_i = dmcs_.begin()) {
      DMC *dmc = dmc_i->second;
      dmcs_.erase(dmc_i);
      if (dmc)
        delete dmc;
    }
  }

  void DMCLoader::make_elements(Config& cfg) {
    for (int n = 0;; ++n) {
      XMLNode cn = cfg.Child(n);
      if (!cn)
        break;
      Config cfg_(cn, cfg.getFileName());

      if (MatchXMLName(cn, "DataManager")) {
        std::string name = cn.Attribute("name");
        if (name.empty()) {
          logger.msg(ERROR, "DataManager has no name attribute defined");
          continue;
        }
        std::string id = cn.Attribute("id");
        if (id.empty()) {
          logger.msg(ERROR, "DataManager has no id attribute defined");
          continue;
        }
        DMCPluginArgument arg(&cfg);
        DMC *dmc = factory_->GetInstance<DMC>(DMCPluginKind, name, &arg);
        if (!dmc) {
          logger.msg(ERROR, "DataManager %s(%s) could not be created",
                     name, id);
          continue;
        }
        dmcs_[id] = dmc;
        logger.msg(INFO, "Loaded DataManager %s(%s)", name, id);
        continue;
      }

      //logger.msg(WARNING, "Unknown element \"%s\" - ignoring", cn.Name());
    }
  }

  DMC* DMCLoader::getDMC(const std::string& id) {
    dmc_container_t::iterator dmc = dmcs_.find(id);
    if (dmc != dmcs_.end())
      return dmc->second;
    return NULL;
  }

} // namespace Arc
