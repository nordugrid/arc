#include "DataDeliveryRemoteComm.h"

namespace DataStaging {

  DataDeliveryRemoteComm::DataDeliveryRemoteComm(const DTR& request, const TransferParameters& params)
    : DataDeliveryComm(request, params) {
    // connect to service and make a new transfer request
  }

  DataDeliveryRemoteComm::~DataDeliveryRemoteComm() {
    // if transfer is still going, send cancellation request to service
  }

  void DataDeliveryRemoteComm::PullStatus() {
    // send query request to service and fill status_
  }


} // namespace DataStaging
