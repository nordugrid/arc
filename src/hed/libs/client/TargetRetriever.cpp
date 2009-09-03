// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/client/TargetRetriever.h>
#include <arc/client/UserConfig.h>
#include <arc/loader/FinderLoader.h>

namespace Arc {

  Logger TargetRetriever::logger(Logger::getRootLogger(), "TargetRetriever");

  TargetRetriever::TargetRetriever(const Config& cfg,
                                   const UserConfig& usercfg,
                                   const std::string& flavour)
    : flavour(flavour),
      usercfg(usercfg),
      url((std::string)(cfg)["URL"]),
      serviceType((std::string)(cfg)["URL"].Attribute("ServiceType")) {}

  TargetRetriever::~TargetRetriever() {}

  TargetRetrieverLoader::TargetRetrieverLoader()
    : Loader(BaseConfig().MakeConfig(Config()).Parent()) {}

  TargetRetrieverLoader::~TargetRetrieverLoader() {
    for (std::list<TargetRetriever*>::iterator it = targetretrievers.begin();
         it != targetretrievers.end(); it++)
      delete *it;
  }

  TargetRetriever* TargetRetrieverLoader::load(const std::string& name,
                                               const Config& cfg,
                                               const UserConfig& usercfg) {
    if (name.empty())
      return NULL;

    PluginList list = FinderLoader::GetPluginList("HED:TargetRetriever");
    factory_->load(list[name], "HED:TargetRetriever");

    TargetRetrieverPluginArgument arg(cfg, usercfg);
    TargetRetriever *targetretriever =
      factory_->GetInstance<TargetRetriever>("HED:TargetRetriever", name, &arg);

    if (!targetretriever) {
      logger.msg(ERROR, "TargetRetriever %s could not be created", name);
      return NULL;
    }

    targetretrievers.push_back(targetretriever);
    logger.msg(INFO, "Loaded TargetRetriever %s", name);
    return targetretriever;
  }

} // namespace Arc
