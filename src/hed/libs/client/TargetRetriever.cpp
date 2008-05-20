#include <iostream>
#include "TargetRetriever.h"
#include <arc/ArcConfig.h>

namespace Arc {

  TargetRetriever::TargetRetriever(Arc::Config *cfg)
    : ACC() {

    m_url = (std::string)(*cfg)["URL"];

    ServiceType = (std::string)(*cfg)["URL"].Attribute("ServiceType");

  }

  TargetRetriever::~TargetRetriever() {}

} // namespace Arc
