// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ACCLoader.h"

namespace Arc {

  ACCLoader::ACCLoader(Config& cfg)
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

  void ACCLoader::make_elements(Config& cfg) {
    for (int n = 0;; ++n) {
      XMLNode cn = cfg.Child(n);
      if (!cn)
        break;
      Config cfg_(cn, cfg.getFileName());

      if (MatchXMLName(cn, "ArcClientComponent")) {
        std::string name = cn.Attribute("name");
        if (name.empty()) {
          logger.msg(ERROR, "ArcClientComponent has no name attribute defined");
          continue;
        }
        std::string id = cn.Attribute("id");
        if (id.empty()) {
          logger.msg(ERROR, "ArcClientComponent has no id attribute defined");
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


  ACC* ACCLoader::getACC(const std::string& id) {
    acc_container_t::iterator acc = accs_.find(id);
    if (acc != accs_.end())
      return acc->second;
    return NULL;
  }

} // namespace Arc
