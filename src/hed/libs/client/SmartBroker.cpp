#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/SmartBroker.h>

namespace Arc {

  SmartBroker::SmartBroker(Arc::JobDescription jd) : Broker( jd) {}
  SmartBroker::~SmartBroker() {}

} // namespace Arc
