#include <iostream>

#include <arc/ArcConfig.h>
#include <arc/client/TargetRetriever.h>

namespace Arc {

  TargetRetriever::TargetRetriever(Config *cfg)
    : ACC() {

    m_url = (std::string)(*cfg)["URL"];

    ServiceType = (std::string)(*cfg)["URL"].Attribute("ServiceType");

  }

  TargetRetriever::~TargetRetriever() {}

} // namespace Arc
