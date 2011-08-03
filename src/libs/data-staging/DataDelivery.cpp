#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "DataDeliveryComm.h"
#include "DataDelivery.h"

namespace DataStaging {

  Arc::Logger DataDelivery::logger(Arc::Logger::getRootLogger(), "DataStaging.DataDelivery");

  /// Wrapper class around DataDeliveryComm
  class DataDelivery::delivery_pair_t {
    public:
    DTR* dtr;
    DataDeliveryComm* comm;
    bool cancelled;
    delivery_pair_t(DTR* request, const TransferParameters& params);
    ~delivery_pair_t();
  };

  DataDelivery::delivery_pair_t::delivery_pair_t(DTR* request, const TransferParameters& params)
    :dtr(request),cancelled(false) {
    comm = DataDeliveryComm::CreateInstance(*request, params);
  }

  DataDelivery::delivery_pair_t::~delivery_pair_t() {
    delete comm;
  }

  DataDelivery::DataDelivery(): delivery_state(INITIATED) {
  }

  bool DataDelivery::start() {
    if(delivery_state == RUNNING || delivery_state == TO_STOP) return false;
    delivery_state = RUNNING;
    Arc::CreateThreadFunction(&main_thread,this);
    return true;
  }

  void DataDelivery::receiveDTR(DTR& dtr) {
    if(!dtr) {
      logger.msg(Arc::ERROR, "Received invalid DTR");
      dtr.set_error_status(DTRErrorStatus::INTERNAL_ERROR, DTRErrorStatus::ERROR_UNKNOWN, "Invalid DTR");
      dtr.set_status(DTRStatus::TRANSFERRED);
      dtr.push(SCHEDULER);
      return;
    }
    dtr.get_logger()->msg(Arc::INFO, "Delivery received new DTR %s with source: %s, destination: %s",
               dtr.get_id(), dtr.get_source()->CurrentLocation().str(), dtr.get_destination()->CurrentLocation().str());
    /*
     *  Change the status of the dtr to TRANSFERRING	 
     *  Start reading from the source into a buffer
     *  TODO: Complete reading the file   
     *  TODO: Do the checksome 
     */
    dtr.set_status(DTRStatus::TRANSFERRING);
    delivery_pair_t* d = new delivery_pair_t(&dtr, transfer_params);
    if(d->comm) {
      dtr_list_lock.lock();
      dtr_list.push_back(d);
      dtr_list_lock.unlock();
    } else {
      dtr.set_error_status(DTRErrorStatus::INTERNAL_ERROR, DTRErrorStatus::ERROR_UNKNOWN, "Failed to start Delivery process");
      dtr.set_status(DTRStatus::TRANSFERRED);
      dtr.push(SCHEDULER);
    }
    return;
  }

  bool DataDelivery::cancelDTR(DTR* request) {
    if(!request) {
      logger.msg(Arc::ERROR, "Received no DTR");
      return false;
    }
    if(!(*request)) {
      logger.msg(Arc::ERROR, "Received invalid DTR");
      request->set_status(DTRStatus::ERROR);
      return false;
    }
    dtr_list_lock.lock();
    for (std::list<delivery_pair_t*>::iterator i = dtr_list.begin(); i != dtr_list.end(); ++i) {
      delivery_pair_t* ip = *i;
      if (ip->dtr->get_id() == request->get_id()) {
        request->get_logger()->msg(Arc::INFO, "Cancelling DTR %s with source: %s, destination: %s",
                   request->get_id(), request->get_source()->str(), request->get_destination()->str());
        ip->cancelled = true;
        ip->dtr->set_status(DTRStatus::TRANSFERRING_CANCEL);
        dtr_list_lock.unlock();
        return true;
      }
    }
    // DTR is not in the active transfer list - log a warning
    dtr_list_lock.unlock();
    request->get_logger()->msg(Arc::WARNING, "DTR %s requested cancel but no active transfer",
                   request->get_id());
    request->set_status(DTRStatus::TRANSFERRED);
    request->push(SCHEDULER);
    return true;
  }

  bool DataDelivery::stop() {
    if(delivery_state != RUNNING) return false;
    delivery_state = TO_STOP;
    run_signal.wait();
    delivery_state = STOPPED;
    return true;
  }

  void DataDelivery::SetTransferParameters(const TransferParameters& params) {
    transfer_params = params;
  }

  void DataDelivery::main_thread (void* arg) {
    DataDelivery* it = (DataDelivery*)arg;
    it->main_thread();
  }

  void DataDelivery::main_thread (void) {
    // disconnect from root logger so
    // messages are logged to per-DTR Logger
    Arc::Logger::getRootLogger().setThreadContext();
    Arc::Logger::getRootLogger().removeDestinations();

    while(delivery_state != TO_STOP){
      dtr_list_lock.lock();
      std::list<delivery_pair_t*>::iterator d = dtr_list.begin();
      dtr_list_lock.unlock();
      for(;;) {
        dtr_list_lock.lock();
        if(d == dtr_list.end()) {
          dtr_list_lock.unlock();
          break;
        }
        dtr_list_lock.unlock();
        delivery_pair_t* dp = *d;
        DataDeliveryComm::Status status;
        status = dp->comm->GetStatus();
        // TODO: fill status into DTR
        //std::cerr<<"Time: "<<status.timestamp
        //         <<", Comm. Status: "<<status.commstatus
        //         <<", Status: "<<status.status
        //         <<", Bytes: "<<status.transfered<<"/"<<status.size<<std::endl;

        // check for cancellation
        if (dp->cancelled) {
          dtr_list_lock.lock();
          d = dtr_list.erase(d);
          dtr_list_lock.unlock();

          dp->dtr->set_status(DTRStatus::TRANSFERRED);
          dp->dtr->push(SCHEDULER);
          // deleting delivery_pair_t kills the spawned process
          delete dp;
          continue;
        }
        if((status.commstatus == DataDeliveryComm::CommExited) ||
           (status.commstatus == DataDeliveryComm::CommClosed) ||
           (status.commstatus == DataDeliveryComm::CommFailed)) {
          // Transfer finished - either successfully or with error
          // comm.GetError()
          dtr_list_lock.lock();
          d = dtr_list.erase(d);
          dtr_list_lock.unlock();
          if((status.commstatus == DataDeliveryComm::CommFailed) ||
             (status.error != DTRErrorStatus::NONE_ERROR)) {
            if(status.error == DTRErrorStatus::NONE_ERROR)
              status.error = DTRErrorStatus::INTERNAL_ERROR;
            dp->dtr->set_error_status(status.error,status.error_location,
                     status.error_desc[0]?status.error_desc:dp->comm->GetError().c_str());
          } else if (status.checksum) {
            dp->dtr->get_destination()->SetCheckSum(status.checksum);
          }
          dp->dtr->get_logger()->msg(Arc::INFO, "DTR %s: Transfer finished: %llu bytes transferred %s",
                                     dp->dtr->get_short_id(), status.transfered,
                                     (status.checksum[0] ? ": checksum "+std::string(status.checksum) : " "));
          dp->dtr->set_status(DTRStatus::TRANSFERRED);
          dp->dtr->push(SCHEDULER);
          delete dp;
          continue;
        }
        if(!(dp->comm)) {
          // Error happened
          // comm.GetError()
          dtr_list_lock.lock();
          d = dtr_list.erase(d);
          dtr_list_lock.unlock();
          dp->dtr->set_error_status(DTRErrorStatus::INTERNAL_ERROR,DTRErrorStatus::ERROR_TRANSFER,
                   dp->comm->GetError().empty()?"Connection with delivery process lost":dp->comm->GetError());
          dp->dtr->set_status(DTRStatus::TRANSFERRED);
          dp->dtr->push(SCHEDULER);
          delete dp;
          continue;
        }
        dtr_list_lock.lock();
        ++d;
        dtr_list_lock.unlock();
      }
      	
      // TODO: replace with condition
      Glib::usleep(500000);
    }
    logger.msg(Arc::INFO, "Data delivery loop exited");
    run_signal.signal();
  }

  
} // namespace DataStaging
