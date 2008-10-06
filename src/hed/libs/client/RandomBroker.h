#include "Broker.h"

namespace Arc {

  class RandomBroker
    : public Broker {
  protected:
    RandomBroker(Arc::JobDescription jd);
    virtual ~RandomBroker();
    void sort_Targets() {};
  };

} // namespace Arc
