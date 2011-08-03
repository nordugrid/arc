#include "DataDeliveryRemoteComm.h"

namespace DataStaging {

  DataDeliveryRemoteComm::DataDeliveryRemoteComm(const DTR& request, const TransferParameters& params)
    : DataDeliveryComm(request, params)
  {}

  DataDeliveryRemoteComm::~DataDeliveryRemoteComm() {};

  DataDeliveryComm::Status DataDeliveryRemoteComm::GetStatus() const {
    return DataDeliveryComm::Status();
  }

  void DataDeliveryRemoteComm::PullStatus() {}


} // namespace DataStaging
