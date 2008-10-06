#include "Broker.h"

namespace Arc {

  class SmartBroker
    : public Broker {
  protected:
    SmartBroker(Arc::JobDescription jd);
    virtual ~SmartBroker();
    void sort_Targets() {};
  };

} // namespace Arc
