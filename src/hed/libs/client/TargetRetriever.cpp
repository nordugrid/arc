#include "TargetRetriever.h"

namespace Arc {

  TargetRetriever::TargetRetriever(Arc::Config *cfg, const URL& url) : ACC(cfg), m_url(url){}
  TargetRetriever::~TargetRetriever(){}

} // namespace Arc
