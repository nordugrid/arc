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
    DTR_ptr dtr;
    TransferParameters params;
    DataDeliveryComm* comm;
    bool cancelled;
    Arc::SimpleCounter thread_count;
    delivery_pair_t(DTR_ptr request, const TransferParameters& params);
    ~delivery_pair_t();
    void start();
  };

  DataDelivery::delivery_pair_t::delivery_pair_t(DTR_ptr request, const TransferParameters& params)
    :dtr(request),params(params),comm(NULL),cancelled(false) {}

  DataDelivery::delivery_pair_t::~delivery_pair_t() {
    if (comm) delete comm;
  }

  void DataDelivery::delivery_pair_t::start() {
    comm = DataDeliveryComm::CreateInstance(dtr, params);
  }

  DataDelivery::DataDelivery(): delivery_state(INITIATED) {
  }

  bool DataDelivery::start() {
    if(delivery_state == RUNNING || delivery_state == TO_STOP) return false;
    delivery_state = RUNNING;
    Arc::CreateThreadFunction(&main_thread,this);
    return true;
  }

  void DataDelivery::receiveDTR(DTR_ptr dtr) {
    if(!(*dtr)) {
      logger.msg(Arc::ERROR, "Received invalid DTR");
      dtr->set_error_status(DTRErrorStatus::INTERNAL_LOGIC_ERROR, DTRErrorStatus::ERROR_UNKNOWN, "Invalid DTR");
      dtr->set_status(DTRStatus::TRANSFERRED);
      DTR::push(dtr, SCHEDULER);
      return;
    }
    dtr->get_logger()->msg(Arc::INFO, "Delivery received new DTR %s with source: %s, destination: %s",
               dtr->get_id(), dtr->get_source()->CurrentLocation().str(), dtr->get_destination()->CurrentLocation().str());

    dtr->set_status(DTRStatus::TRANSFERRING);
    delivery_pair_t* d = new delivery_pair_t(dtr, transfer_params);
    dtr_list_lock.lock();
    dtr_list.push_back(d);
    dtr_list_lock.unlock();
    cond.signal();
    return;
  }

  bool DataDelivery::cancelDTR(DTR_ptr request) {
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
        cond.signal();
        return true;
      }
    }
    // DTR is not in the active transfer list, probably because it just finished
    dtr_list_lock.unlock();
    request->get_logger()->msg(Arc::WARNING, "DTR %s requested cancel but no active transfer",
                   request->get_id());
    // if request is already TRANSFERRED, no need to push to Scheduler again
    if (request->get_status() != DTRStatus::TRANSFERRED) {
      request->set_status(DTRStatus::TRANSFERRED);
      DTR::push(request, SCHEDULER);
    }
    return true;
  }

  bool DataDelivery::stop() {
    if(delivery_state != RUNNING) return false;
    delivery_state = TO_STOP;
    cond.signal();
    run_signal.wait();
    delivery_state = STOPPED;
    return true;
  }

  void DataDelivery::SetTransferParameters(const TransferParameters& params) {
    transfer_params = params;
  }

  void DataDelivery::start_delivery(void* arg) {
    delivery_pair_t* dp = (delivery_pair_t*)arg;
    dp->start();
  }

  void DataDelivery::stop_delivery(void* arg) {
    delivery_pair_t* dp = (delivery_pair_t*)arg;
    delete dp->comm;
    dp->comm = NULL;
    // In case transfer finished before getting cancel signal, delete destination
    if (dp->cancelled || dp->dtr->error()) dp->dtr->get_destination()->Remove();
  }

  bool DataDelivery::delete_delivery_pair(delivery_pair_t* dp) {
    bool res = Arc::CreateThreadFunction(&stop_delivery, dp, &dp->thread_count);
    if (res) {
      res = dp->thread_count.wait(300*1000);
    }
    if (res) delete dp;
    return res;
  }

  // Delete DTR destination, called after losing contact with delivery process
  static void delete_dtr_destination(DTR_ptr dtr) {
    Arc::URL dest(dtr->get_destination()->CurrentLocation());
    // Check for TURL
    if (!dtr->get_destination()->TransferLocations().empty()) {
      dest = dtr->get_destination()->TransferLocations().front();
    }
    // Check for cache file
    if ((dtr->get_cache_state() == CACHEABLE) && !dtr->get_cache_file().empty()) {
      dest = dtr->get_cache_file();
    }
    dtr->get_logger()->msg(Arc::VERBOSE, "Cleaning up after failure: deleting %s", dest.str());
    Arc::DataHandle h(dest, dtr->get_usercfg());
    if (h) h->Remove();
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
    Arc::Logger::getRootLogger().setThreshold(DTR::LOG_LEVEL);

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
        // first check for cancellation
        if (dp->cancelled) {
          dtr_list_lock.lock();
          d = dtr_list.erase(d);
          dtr_list_lock.unlock();

          // deleting delivery_pair_t kills the spawned process
          // Do this before passing back to Scheduler to avoid race condition
          // of DTR being deleted before Comm object has finished with it.
          // With ThreadedPointer this may not be a problem any more.
          DTR_ptr tmp = dp->dtr;
          if (!delete_delivery_pair(dp)) {
            tmp->get_logger()->msg(Arc::ERROR, "Failed to delete delivery object or deletion timed out");
          }
          tmp->get_job_perf_record().End("SchedulerTransferTime_"+tmp->get_delivery_endpoint().Host());
          tmp->set_status(DTRStatus::TRANSFERRED);
          DTR::push(tmp, SCHEDULER);
          continue;
        }
        // check for new transfer
        if (!dp->comm) {
          dp->dtr->get_job_perf_record().Start(dp->dtr->get_short_id());
          // Connecting to a remote delivery service can hang in rare cases,
          // so launch a separate thread with a timeout
          bool res = Arc::CreateThreadFunction(&start_delivery, dp, &dp->thread_count);
          if (res) {
            res = dp->thread_count.wait(300*1000);
          }
          if (!res) {
            // error or timeout - in this case do not delete dp since if the
            // thread timed out it may wake up at some point. Better to have a
            // small memory leak than seg fault.
            dtr_list_lock.lock();
            d = dtr_list.erase(d);
            dtr_list_lock.unlock();

            DTR_ptr tmp = dp->dtr;
            tmp->set_error_status(DTRErrorStatus::INTERNAL_PROCESS_ERROR,
                                      DTRErrorStatus::NO_ERROR_LOCATION,
                                      "Failed to start thread to start delivery or thread timed out");
            tmp->get_job_perf_record().End("SchedulerTransferTime_"+tmp->get_delivery_endpoint().Host());
            tmp->set_status(DTRStatus::TRANSFERRED);
            DTR::push(tmp, SCHEDULER);

          } else {
            dtr_list_lock.lock();
            ++d;
            dtr_list_lock.unlock();
          }
          continue;
        }
        // ongoing transfer - get status
        DataDeliveryComm::Status status;
        status = dp->comm->GetStatus();
        dp->dtr->set_bytes_transferred(status.transferred);

        if((status.commstatus == DataDeliveryComm::CommExited) ||
           (status.commstatus == DataDeliveryComm::CommClosed) ||
           (status.commstatus == DataDeliveryComm::CommFailed)) {
          // Transfer finished - either successfully or with error
          dtr_list_lock.lock();
          d = dtr_list.erase(d);
          dtr_list_lock.unlock();

          if ((status.commstatus == DataDeliveryComm::CommFailed) ||
              (status.error != DTRErrorStatus::NONE_ERROR)) {

            if (status.error == DTRErrorStatus::NONE_ERROR) {
              // Lost track of process - delete destination so it can be tried again
              delete_dtr_destination(dp->dtr);
              status.error = DTRErrorStatus::INTERNAL_PROCESS_ERROR;
            }
            dp->dtr->set_error_status(status.error,status.error_location,
                     status.error_desc[0]?status.error_desc:dp->comm->GetError().c_str());
          }
          else if (status.checksum[0]) {
            dp->dtr->get_destination()->SetCheckSum(status.checksum);
          }
          dp->dtr->get_logger()->msg(Arc::INFO, "Transfer finished: %llu bytes transferred %s", status.transferred,
                                     (status.checksum[0] ? ": checksum "+std::string(status.checksum) : " "));
          timespec dummy;
          dp->dtr->get_job_perf_log().Log("DeliveryTransferTime_"+dp->dtr->get_delivery_endpoint().Host(),
                   dp->dtr->get_short_id()+"\t"+Arc::tostring(status.transfer_time), dummy, dummy);
          dp->dtr->set_transfer_time(status.transfer_time);

          DTR_ptr tmp = dp->dtr;
          if (!delete_delivery_pair(dp)) {
            tmp->get_logger()->msg(Arc::ERROR, "Failed to delete delivery object or deletion timed out");
          }
          tmp->get_job_perf_record().End("SchedulerTransferTime_"+tmp->get_delivery_endpoint().Host());
          tmp->set_status(DTRStatus::TRANSFERRED);
          DTR::push(tmp, SCHEDULER);
          continue;
        }
        if(!(*(dp->comm))) {
          // Error happened - either delivery process is stuck or could not start
          dtr_list_lock.lock();
          d = dtr_list.erase(d);
          dtr_list_lock.unlock();
          std::string comm_err = dp->comm->GetError();

          if (status.commstatus == DataDeliveryComm::CommInit) {
            if (comm_err.empty()) comm_err = "Failed to start delivery process";
            if (dp->dtr->get_delivery_endpoint() == DTR::LOCAL_DELIVERY) {
              // Serious problem, so mark permanent error
              dp->dtr->set_error_status(DTRErrorStatus::INTERNAL_LOGIC_ERROR,
                                        DTRErrorStatus::ERROR_TRANSFER,
                                        comm_err);
            }
            else {
              // Failing to start on remote service should be retried
              dp->dtr->add_problematic_delivery_service(dp->dtr->get_delivery_endpoint());
              dp->dtr->set_error_status(DTRErrorStatus::INTERNAL_PROCESS_ERROR,
                                        DTRErrorStatus::ERROR_TRANSFER,
                                        comm_err);
            }
          }
          else {
            if (comm_err.empty()) comm_err = "Connection with delivery process lost";
            // delete destination so it can be tried again
            delete_dtr_destination(dp->dtr);
            dp->dtr->set_error_status(DTRErrorStatus::INTERNAL_PROCESS_ERROR,
                                      DTRErrorStatus::ERROR_TRANSFER,
                                      comm_err);
          }
          DTR_ptr tmp = dp->dtr;
          if (!delete_delivery_pair(dp)) {
            tmp->get_logger()->msg(Arc::ERROR, "Failed to delete delivery object or deletion timed out");
          }
          tmp->get_job_perf_record().End("SchedulerTransferTime_"+tmp->get_delivery_endpoint().Host());
          tmp->set_status(DTRStatus::TRANSFERRED);
          DTR::push(tmp, SCHEDULER);
          continue;
        }
        dtr_list_lock.lock();
        ++d;
        dtr_list_lock.unlock();
      }
      	
      // Go through main loop every half a second or when new transfer arrives
      cond.wait(100);
    }
    // Kill any transfers still running
    dtr_list_lock.lock();
    for (std::list<delivery_pair_t*>::iterator d = dtr_list.begin(); d != dtr_list.end();) {
      DTR_ptr tmp = (*d)->dtr;
      if (!delete_delivery_pair(*d)) {
        tmp->get_logger()->msg(Arc::ERROR, "Failed to delete delivery object or deletion timed out");
      }
      d = dtr_list.erase(d);
    }
    dtr_list_lock.unlock();

    logger.msg(Arc::INFO, "Data delivery loop exited");
    run_signal.signal();
  }

  
} // namespace DataStaging
