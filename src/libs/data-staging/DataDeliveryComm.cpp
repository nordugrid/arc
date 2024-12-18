#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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
    : status_pos_(0),transfer_params(params),logger_(dtr->get_logger()),handler_(NULL) {
  }

  DataDeliveryCommHandler& DataDeliveryComm::GetHandler() {
    if(handler_) return *handler_;
    return *(handler_ = DataDeliveryCommHandler::getInstance(DeliveryId()));
  }

  DataDeliveryComm::Status DataDeliveryComm::GetStatus(void) const {
    Glib::Mutex::Lock lock(*(const_cast<Glib::Mutex*>(&lock_)));
    DataDeliveryComm::Status tmp = status_;
    return tmp;
  }

  bool DataDeliveryComm::CheckComm(DTR_ptr dtr, std::vector<std::string>& allowed_dirs, std::string& load_avg) {
    if (!dtr->get_delivery_endpoint() || dtr->get_delivery_endpoint() == DTR::LOCAL_DELIVERY)
      return DataDeliveryLocalComm::CheckComm(dtr, allowed_dirs, load_avg);
    return DataDeliveryRemoteComm::CheckComm(dtr, allowed_dirs, load_avg);
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

  Glib::Mutex DataDeliveryCommHandler::comm_lock;
  std::map<std::string, DataDeliveryCommHandler*> DataDeliveryCommHandler::comm_handler;

  DataDeliveryCommHandler* DataDeliveryCommHandler::getInstance(std::string const & id) {
    Glib::Mutex::Lock lock(comm_lock);
    std::map<std::string, DataDeliveryCommHandler*>::iterator it = comm_handler.find(id);
    if(it != comm_handler.end()) return it->second;
    return (comm_handler[id] = new DataDeliveryCommHandler);
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
