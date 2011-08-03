#ifndef DATADELIVERYREMOTECOMM_H_
#define DATADELIVERYREMOTECOMM_H_

#include "DataDeliveryComm.h"

namespace DataStaging {

  /// This class contacts a remote service to make a Delivery request.
  class DataDeliveryRemoteComm : public DataDeliveryComm {
  public:
    DataDeliveryRemoteComm(const DTR& request, const TransferParameters& params);
    ~DataDeliveryRemoteComm();

    /// Obtain status of transfer
    virtual DataDeliveryComm::Status GetStatus() const;

    /// Read status from service
    virtual void PullStatus();

    /// Returns true if service is still processing request
    virtual operator bool() const { return true; };
    /// Returns true if service is not processing request
    virtual bool operator!() const { return false; };
  private:
  };

} // namespace DataStaging

#endif /* DATADELIVERYREMOTECOMM_H_ */
