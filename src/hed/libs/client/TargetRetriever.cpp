#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/client/TargetRetriever.h>

namespace Arc {

  Logger TargetRetriever::logger(Logger::getRootLogger(), "TargetRetriever");

  TargetRetriever::TargetRetriever(Config *cfg)
    : ACC(cfg) {
    url = (std::string)(*cfg)["URL"];
    serviceType = (std::string)(*cfg)["URL"].Attribute("ServiceType");
  }

  TargetRetriever::~TargetRetriever() {}

} // namespace Arc
