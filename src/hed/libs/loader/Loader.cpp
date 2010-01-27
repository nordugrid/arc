#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/StringConv.h>

#include "Loader.h"

namespace Arc {

  Logger Loader::logger(Logger::rootLogger, "Loader");

  Loader::~Loader(void) {
    if(factory_) delete factory_;
  }

  Loader::Loader(XMLNode cfg) {
    factory_    = new PluginsFactory(cfg);
    for(int n = 0;; ++n) {
      XMLNode cn = cfg.Child(n);
      if(!cn) break;

      if(MatchXMLName(cn, "ModuleManager")) {
        continue;
      }

      if(MatchXMLName(cn, "Plugins")) {
        XMLNode n;
        for (int i = 0; (n = cn["Name"][i]) != false; i++) { 
            std::string name = (std::string)n;
            factory_->load(name);
        }
      }

      // Configuration processing is split to multiple functions - hence
      // ignoring all unknown elements.
      //logger.msg(WARNING, "Unknown element \"%s\" - ignoring", cn.Name());
    }

  }

} // namespace Arc
