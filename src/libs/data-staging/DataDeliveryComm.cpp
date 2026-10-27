#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>

#include "DataDeliveryComm.h"
#include "DataDeliveryRemoteComm.h"
#include "DataDeliveryLocalComm.h"

namespace DataStaging {

  DataDeliveryComm* DataDeliveryComm::CreateInstance(DTR_ptr dtr, const TransferParameters& params) {
    if (!dtr->get_delivery_endpoint() || dtr->get_delivery_endpoint() == DTR::LOCAL_DELIVERY)
      return new DataDeliveryLocalComm(dtr, params);
    return new DataDeliveryRemoteComm(dtr, params);
  }

  DataDeliveryComm::DataDeliveryComm(DTR_ptr dtr, const TransferParameters& params)
    : status_pos_(0),transfer_params(params),logger_(dtr->get_logger()) {
  }

  DataDeliveryComm::Status DataDeliveryComm::GetStatus(void) const {
    std::unique_lock<std::mutex> lock(*(const_cast<std::mutex*>(&lock_)));
    DataDeliveryComm::Status tmp = status_;
    return tmp;
  }

  bool DataDeliveryComm::CheckComm(DTR_ptr dtr, std::vector<std::string>& allowed_dirs, std::string& load_avg) {
    if (!dtr->get_delivery_endpoint() || dtr->get_delivery_endpoint() == DTR::LOCAL_DELIVERY)
      return DataDeliveryLocalComm::CheckComm(dtr, allowed_dirs, load_avg);
    return DataDeliveryRemoteComm::CheckComm(dtr, allowed_dirs, load_avg);
  }

} // namespace DataStaging
