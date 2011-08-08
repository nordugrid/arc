#ifndef DATADELIVERYLOCALCOMM_H_
#define DATADELIVERYLOCALCOMM_H_

#include <arc/Run.h>

#include "DataDeliveryComm.h"

namespace DataStaging {

  /// This class starts, monitors and controls a local Delivery process.
  class DataDeliveryLocalComm : public DataDeliveryComm {
  public:

    /// Starts child process
    DataDeliveryLocalComm(const DTR& dtr, const TransferParameters& params);
    /// This stops the child process
    ~DataDeliveryLocalComm();

    /// Read from stdout of child to get status
    virtual void PullStatus();

    /// Returns true if child process exists
    virtual operator bool() const { return (child_ != NULL); };
    /// Returns true if child process does not exist
    virtual bool operator!() const { return (child_ == NULL); };

  private:
    /// Child process
    Arc::Run* child_;
    /// Time last communication was received from child
    Arc::Time last_comm;
  };

} // namespace DataStaging

#endif /* DATADELIVERYLOCALCOMM_H_ */
