#ifndef __ARC_DATABROKER_H__
#define __ARC_DATABROKER_H__

#include <arc/client/Broker.h>

namespace Arc {
  
  class DataBroker: public Broker {
    
  public:
    DataBroker(Config *cfg);
    ~DataBroker();
    static Plugin* Instance(PluginArgument* arg);
    bool CacheCheck(std::string& filename, ExecutionTarget& target);
    
  protected:
    void SortTargets();
  };
  
} // namespace Arc

#endif // __ARC_DATABROKER_H__
