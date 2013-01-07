// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/compute/Broker.h>

#include "BrokerPlugin.h"

namespace Arc {

  Logger BrokerPlugin::logger(Logger::getRootLogger(), "BrokerPlugin");

  bool BrokerPlugin::operator() (const ExecutionTarget&, const ExecutionTarget&) const {
    return true;
  }

  bool BrokerPlugin::match(const ExecutionTarget& et) const {
    if(!j) return false;
    return Broker::genericMatch(et,*j,uc);
  }

  void BrokerPlugin::set(const JobDescription& _j) const {
    j = &_j;
  }

  BrokerPluginLoader& Broker::getLoader() {
    // For C++ it would be enough to have 
    //   static BrokerPluginLoader loader;
    // But Java sometimes does not destroy objects causing
    // PluginsFactory destructor loop forever waiting for
    // plugins to exit.
    static BrokerPluginLoader* loader = NULL;
    if(!loader) {
      loader = new BrokerPluginLoader();
    }
    return *loader;
  }

  BrokerPluginLoader::BrokerPluginLoader() : Loader(BaseConfig().MakeConfig(Config()).Parent()) {}

  BrokerPluginLoader::~BrokerPluginLoader() {
    for (std::list<BrokerPlugin*>::iterator it = plugins.begin();
         it != plugins.end(); ++it) {
      delete *it;
    }
  }

  BrokerPlugin* BrokerPluginLoader::load(const UserConfig& uc, const std::string& name, bool keep_ownerskip) {
    return load(uc, NULL, name, keep_ownerskip);
  }

  BrokerPlugin* BrokerPluginLoader::load(const UserConfig& uc, const JobDescription& j, const std::string& name, bool keep_ownerskip) {
    return load(uc, &j, name, keep_ownerskip);
  }

  BrokerPlugin* BrokerPluginLoader::copy(const BrokerPlugin* p, bool keep_ownerskip) {
    if (p) {
      BrokerPlugin* bp = new BrokerPlugin(*p);
      if (bp) {
        if (keep_ownerskip) {
          plugins.push_back(bp);
        };
        return bp;
      };
    };
    return NULL;
  }
  
  BrokerPlugin* BrokerPluginLoader::load(const UserConfig& uc, const JobDescription* j, const std::string& name, bool keep_ownership) {
    BrokerPluginArgument arg(uc);
    if (name.empty()) {
      BrokerPlugin* p = new BrokerPlugin(&arg);
      if (!p) {
        return NULL;
      }
      if (j) {
        p->set(*j);
      }
      
      if (keep_ownership) {
        plugins.push_back(p);
      }
      return p;
    }

    if(!factory_->load(FinderLoader::GetLibrariesList(), "HED:BrokerPlugin", name)) {
      logger.msg(DEBUG, "Broker plugin \"%s\" not found.", name);
      return NULL;
    }

    BrokerPlugin *p = factory_->GetInstance<BrokerPlugin>("HED:BrokerPlugin", name, &arg, false);

    if (!p) {
      logger.msg(DEBUG, "Unable to load BrokerPlugin (%s)", name);
      return NULL;
    }

    if (j) {
      p->set(*j);
    }
    if (keep_ownership) {
      plugins.push_back(p);
    }
    logger.msg(INFO, "Broker %s loaded", name);
    return p;
  }

} // namespace Arc
