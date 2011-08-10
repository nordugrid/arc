#ifndef DATADELIVERYREMOTECOMM_H_
#define DATADELIVERYREMOTECOMM_H_

#include <arc/XMLNode.h>
#include <arc/client/ClientInterface.h>

#include "DataDeliveryComm.h"

namespace DataStaging {

  /// This class contacts a remote service to make a Delivery request.
  class DataDeliveryRemoteComm : public DataDeliveryComm {
  public:
    DataDeliveryRemoteComm(const DTR& dtr, const TransferParameters& params);
    ~DataDeliveryRemoteComm();

    /// Read status from service
    virtual void PullStatus();

    /// Returns true if service is still processing request
    virtual operator bool() const { return valid; };
    /// Returns true if service is not processing request or down
    virtual bool operator!() const { return !valid; };

  private:
    /// Connection to service
    Arc::ClientSOAP* client;
    /// Full DTR ID
    std::string dtr_full_id;
    /// Flag to say whether transfer is running and service is still up
    bool valid;
    /// Logger object (main log, not DTR's log)
    static Arc::Logger logger;

    /// Fill Status object with data in node. If empty fields are initialised
    /// to default values.
    void FillStatus(const Arc::XMLNode& node = Arc::XMLNode());

    /// Set up delegation so the credentials can be used by the service
    bool SetupDelegation(Arc::XMLNode& op, const Arc::UserConfig& usercfg);
  };

} // namespace DataStaging

#endif /* DATADELIVERYREMOTECOMM_H_ */
