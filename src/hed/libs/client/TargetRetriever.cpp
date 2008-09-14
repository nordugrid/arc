#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/client/TargetRetriever.h>

namespace Arc {

  Logger TargetRetriever::logger(Logger::getRootLogger(), "TargetRetriever");

  TargetRetriever::TargetRetriever(Config *cfg, const std::string& flavour)
    : ACC(cfg, flavour) {
    url = (std::string)(*cfg)["URL"];
    serviceType = (std::string)(*cfg)["URL"].Attribute("ServiceType");
  }

  TargetRetriever::~TargetRetriever() {}

} // namespace Arc
