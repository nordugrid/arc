// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/loader/FinderLoader.h>

#include "ServiceEndpointRetriever.h"

namespace Arc {

int* ServiceEndpointRetrieverTESTControl::tcTimeout = NULL;
ServiceEndpointStatus* ServiceEndpointRetrieverTESTControl::tcStatus = NULL;

ServiceEndpointRetrieverPluginLoader::ServiceEndpointRetrieverPluginLoader()
  : Loader(BaseConfig().MakeConfig(Config()).Parent()) {}

ServiceEndpointRetrieverPluginLoader::~ServiceEndpointRetrieverPluginLoader() {
  for (std::list<ServiceEndpointRetrieverPlugin*>::iterator it = plugins.begin();
       it != plugins.end(); it++) {
    delete *it;
  }
}

ServiceEndpointRetrieverPlugin* ServiceEndpointRetrieverPluginLoader::load(const std::string& name) {
  if (name.empty()) {
    return NULL;
  }

  if(!factory_->load(FinderLoader::GetLibrariesList(), "HED:ServiceEndpointRetrieverPlugin", name)) {
    logger.msg(ERROR, "Unable to locate the \"%s\" plugin. Please refer to installation instructions and check if package providing support for %s plugin is installed", name, name);
    logger.msg(DEBUG, "ServiceEndpointRetrieverPlugin plugin \"%s\" not found.", name, name);
    return NULL;
  }

  ServiceEndpointRetrieverPlugin *p = factory_->GetInstance<ServiceEndpointRetrieverPlugin>("HED:ServiceEndpointRetrieverPlugin", name, NULL, false);

  if (!p) {
    logger.msg(ERROR, "Unable to locate the \"%s\" plugin. Please refer to installation instructions and check if package providing support for \"%s\" plugin is installed", name, name);
    logger.msg(DEBUG, "ServiceEndpointRetrieverPlugin %s could not be created.", name, name);
    return NULL;
  }

  plugins.push_back(p);
  logger.msg(DEBUG, "Loaded ServiceEndpointRetrieverPlugin %s", name);
  return p;
}

}
