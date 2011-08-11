#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "DataDeliveryComm.h"
#include "DataDeliveryRemoteComm.h"
#include "DataDeliveryLocalComm.h"

namespace DataStaging {

  DataDeliveryComm* DataDeliveryComm::CreateInstance(const DTR& dtr, const TransferParameters& params) {
    if (!dtr.get_delivery_endpoint() || dtr.get_delivery_endpoint() == DTR::LOCAL_DELIVERY)
      return new DataDeliveryLocalComm(dtr, params);
    return new DataDeliveryRemoteComm(dtr, params);
  }

  DataDeliveryComm::DataDeliveryComm(const DTR& dtr, const TransferParameters& params)
    : dtr_id(dtr.get_short_id()),transfer_params(params) {
    handler_= DataDeliveryCommHandler::getInstance();
    logger_ = dtr.get_logger();
  }

  DataDeliveryComm::Status DataDeliveryComm::GetStatus(void) const {
    Glib::Mutex::Lock lock(*(const_cast<Glib::Mutex*>(&lock_)));
    DataDeliveryComm::Status tmp = status_;
    return tmp;
  }

  DataDeliveryCommHandler::DataDeliveryCommHandler(void) {
    Glib::Mutex::Lock lock(lock_);
    Arc::CreateThreadFunction(&func,this);
  }

  void DataDeliveryCommHandler::Add(DataDeliveryComm* item) {
    Glib::Mutex::Lock lock(lock_);
    items_.push_back(item);
  }

  void DataDeliveryCommHandler::Remove(DataDeliveryComm* item) {
    Glib::Mutex::Lock lock(lock_);
    for(std::list<DataDeliveryComm*>::iterator i = items_.begin();
                        i!=items_.end();) {
      if(*i == item) {
        i=items_.erase(i);
      } else {
        ++i;
      }
    }
  }

  DataDeliveryCommHandler* DataDeliveryCommHandler::comm_handler = NULL;

  DataDeliveryCommHandler* DataDeliveryCommHandler::getInstance() {
    if(comm_handler) return comm_handler;
    return (comm_handler = new DataDeliveryCommHandler);
  }

  // This is a dedicated thread which periodically checks for
  // new state reported by comm instances and modifies states accordingly
  void DataDeliveryCommHandler::func(void* arg) {
    if(!arg) return;

    // disconnect from root logger since messages are logged to per-DTR Logger
    Arc::Logger::getRootLogger().setThreadContext();
    Arc::Logger::getRootLogger().removeDestinations();

    // We do not need extremely low latency, so this
    // thread simply polls for data 2 times per second.
    DataDeliveryCommHandler& it = *(DataDeliveryCommHandler*)arg;
    for(;;) {
      {
        Glib::Mutex::Lock lock(it.lock_);
        for(std::list<DataDeliveryComm*>::iterator i = it.items_.begin();
                  i != it.items_.end();++i) {
          DataDeliveryComm* comm = *i;
          if(comm)
            comm->PullStatus();
        }
      }
      Glib::usleep(500000);
    }
  }

} // namespace DataStaging
