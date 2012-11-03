// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATABROKERPLUGIN_H__
#define __ARC_DATABROKERPLUGIN_H__

#include <map>

#include <arc/UserConfig.h>
#include <arc/compute/Broker.h>
#include <arc/message/MCC.h>
#include <arc/message/PayloadSOAP.h>

namespace Arc {

  class DataBrokerPlugin : public BrokerPlugin {
  public:
    DataBrokerPlugin(BrokerPluginArgument* parg) : BrokerPlugin(parg), request(NULL) {}
    DataBrokerPlugin(const DataBrokerPlugin& dbp) : BrokerPlugin(dbp), cfg(dbp.cfg), request(dbp.request?new PayloadSOAP(*dbp.request):NULL), CacheMappingTable(dbp.CacheMappingTable) {}
    ~DataBrokerPlugin() { if (request) { delete request; }; };
    static Plugin* Instance(PluginArgument *arg) {
      BrokerPluginArgument *brokerarg = dynamic_cast<BrokerPluginArgument*>(arg);
      return brokerarg ? new DataBrokerPlugin(brokerarg) : NULL;
    }
    virtual bool match(const ExecutionTarget&) const;
    virtual bool operator()(const ExecutionTarget&, const ExecutionTarget&) const;
    virtual void set(const JobDescription& _j);

  protected:
    MCCConfig cfg;
    mutable PayloadSOAP * request;
    mutable std::map<std::string, long> CacheMappingTable;
  };

} // namespace Arc

#endif // __ARC_DATABROKERPLUGIN_H__
