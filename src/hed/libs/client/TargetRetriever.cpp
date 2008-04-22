#include "TargetRetriever.h"
#include <arc/ArcConfig.h>

namespace Arc {

  TargetRetriever::TargetRetriever(Arc::Config *cfg) : ACC(){
    m_url = (std::string) (*cfg)["URL"];
  }

  TargetRetriever::~TargetRetriever(){}

} // namespace Arc
