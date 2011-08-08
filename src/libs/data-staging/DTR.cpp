#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/GUID.h>

#include "Processor.h"
#include "DataDelivery.h"
#include "Scheduler.h"

#include "DTR.h"

namespace DataStaging {

  static const char* const owner_name[] = {
    "GENERATOR",
    "SCHEDULER",
    "PRE-PROCESSOR",
    "DELIVERY",
    "POST-PROCESSOR"
  };

  static const char* get_owner_name(StagingProcesses proc) {
    if(((int)proc) < 0) return "";
    if(((int)proc) >= sizeof(owner_name)/sizeof(const char*)) return "";
    return owner_name[proc];
  }

  const Arc::URL DTR::LOCAL_DELIVERY("file:/local");

  DTR::DTR(const std::string& source,
           const std::string& destination,
           const Arc::UserConfig& usercfg,
           const std::string& jobid,
           const uid_t& uid,
           Arc::Logger* log)
    :  DTR_ID(""),
       source_url(source),
       destination_url(destination),
       cfg(usercfg),
       source_endpoint(source_url, cfg),
       destination_endpoint(destination_url, cfg),
       user(uid),
       parent_job_id(jobid),
       priority(50),
       transfershare("_default"),
       sub_share(""),
       tries_left(1),
       replication(false),
       force_registration(false),
       status(DTRStatus::NEW,"Created by the generator"),
       bytes_transferred(0),
       created(time(NULL)),
       cancel_request(false),
       delivery_endpoint(LOCAL_DELIVERY),
       current_owner(GENERATOR),
       logger(log)
  {
    if (!logger) {
      // use root logger if none is supplied
      logger = new Arc::Logger(Arc::Logger::getRootLogger(), "DTR");
    }
    log_destinations = logger->getDestinations();

    // check that endpoints can be handled
    if (!source_endpoint) {
      logger->msg(Arc::ERROR, "Could not handle endpoint %s", source);
      return;
    }
    if (!destination_endpoint) {
      logger->msg(Arc::ERROR, "Could not handle endpoint %s", destination);
      return;
    }
    // Some validation checks
    if (source_url == destination_url) {
      // It is possible to replicate inside an index service
      // The physical replicas will be checked in RESOLVING
      if (source_endpoint->IsIndex() && destination_endpoint->IsIndex()) {
        replication = true;
      } else {
        logger->msg(Arc::ERROR, "Source is the same as destination");
        set_error_status(DTRErrorStatus::SELF_REPLICATION_ERROR,
                         DTRErrorStatus::NO_ERROR_LOCATION,
                         "Cannot replicate a file to itself");
        return;
      }
    }
    // set insecure by default. Real value will come from configuration
    source_endpoint->SetSecure(false);
    destination_endpoint->SetSecure(false);

#ifdef WIN32
    cache_state = NON_CACHEABLE;
#else
    cache_state = (source_endpoint->Cache() && destination_endpoint->Local()) ? CACHEABLE : NON_CACHEABLE;
#endif
    
    /* Think how to populate transfer parameters */
    mark_modification();
    set_timeout(60);
    // setting ID last means all the previous steps have to pass for the DTR to be valid
    DTR_ID = Arc::UUID();
  }	
  
  DTR::DTR(const DTR& dtr)
    : DTR_ID(dtr.DTR_ID),
      source_url(dtr.source_url),
      destination_url(dtr.destination_url),
      cfg(dtr.cfg),
      source_endpoint(source_url, cfg),
      destination_endpoint(destination_url, cfg),
      cache_file(dtr.cache_file),
      cache_parameters(dtr.cache_parameters),
      cache_state(dtr.cache_state),
      user(dtr.user),
      parent_job_id(dtr.parent_job_id),
      priority(dtr.priority),
      transfershare(dtr.transfershare),
      sub_share(dtr.sub_share),
      tries_left(dtr.tries_left),
      replication(dtr.replication),
      force_registration(dtr.force_registration),
      mapped_source(dtr.mapped_source),
      status(dtr.status),
      error_status(dtr.error_status),
      bytes_transferred(dtr.bytes_transferred),
      timeout(dtr.timeout),
      created(dtr.created),
      next_process_time(dtr.next_process_time),
      cancel_request(dtr.cancel_request),
      delivery_endpoint(dtr.delivery_endpoint),
      current_owner(dtr.current_owner),
      logger(dtr.logger),
      log_destinations(dtr.log_destinations),
      proc_callback(dtr.proc_callback),
      lock()
  {
    // set insecure by default. Real value will come from configuration
    source_endpoint->SetSecure(false);
    destination_endpoint->SetSecure(false);

    mark_modification();
  }
  
  DTR::DTR():
    DTR_ID(""), // empty means invalid DTR
    source_endpoint(Arc::URL(),Arc::UserConfig()),
    destination_endpoint(Arc::URL(),Arc::UserConfig())
  {  

  }

  void DTR::registerCallback(DTRCallback* cb, StagingProcesses owner) {
    lock.lock();
    proc_callback[owner].push_back(cb);
    lock.unlock();
  }

  void DTR::reset() {
    // remove resolved locations
    if (source_endpoint->IsIndex()) {
      source_endpoint->ClearLocations();
    }
    // reset retry count to 1
    source_endpoint->SetTries(1);

    if (destination_endpoint->IsIndex()) {
      destination_endpoint->ClearLocations();
    }
    destination_endpoint->SetTries(1);

    // empty cache and map info
    cache_file.clear();
    mapped_source.clear();
    bytes_transferred = 0;
    reset_error_status();
  }

  void DTR::set_id(const std::string& id) {
    // sanity check - regular expressions would be useful here
    if (id.length() != DTR_ID.length()) {
      logger->msg(Arc::WARNING, "Invalid ID: %s", id);
    } else {
      DTR_ID = id;
    }
  }

  std::string DTR::get_short_id() const {
    if(DTR_ID.length() < 8) return DTR_ID;
    std::string short_id(DTR_ID.substr(0,4)+"..."+DTR_ID.substr(DTR_ID.length()-4));
    return short_id;
  }

  void DTR::set_priority(int pri)
  {
    // limit priority between 1 and 100
    if (pri <= 0) pri = 1;
    if (pri > 100) pri = 100;
    priority = pri;
    mark_modification();
  }
  
  void DTR::set_tries_left(unsigned int tries) {
    tries_left = tries;
  }

  void DTR::decrease_tries_left() {
    if (tries_left > 0) tries_left--;
  }

  void DTR::set_status(DTRStatus stat)
  {
    logger->msg(Arc::VERBOSE, "DTR %s: %s->%s", get_short_id(), status.str(), stat.str());
    lock.lock();
    status = stat;
    lock.unlock();
    mark_modification();
  }
  
  DTRStatus DTR::get_status() {
    lock.lock();
    DTRStatus s = status;
    lock.unlock();
    return s;
  }

  void DTR::set_error_status(DTRErrorStatus::DTRErrorStatusType error_stat,
                             DTRErrorStatus::DTRErrorLocation error_loc,
                             const std::string& desc)
  {
    lock.lock();
    error_status = DTRErrorStatus(error_stat, status.GetStatus(), error_loc, desc);
    lock.unlock();
    mark_modification();
  }

  void DTR::reset_error_status()
  {
    lock.lock();
    error_status = DTRErrorStatus();
    lock.unlock();
    mark_modification();
  }

  DTRErrorStatus DTR::get_error_status() {
    lock.lock();
    DTRErrorStatus s = error_status;
    lock.unlock();
    return s;
  }

  void DTR::set_bytes_transferred(unsigned long long int bytes) {
    bytes_transferred = bytes;
    mark_modification();
  }

  void DTR::set_cache_file(const std::string& filename)
  {
    cache_file = filename;
    mark_modification();
  }

  void DTR::set_cache_state(CacheState state)
  {
    cache_state = state;
    mark_modification();
  }

  void DTR::set_cancel_request()
  {
  	cancel_request = true;
  	// set process time to now so it is picked up straight away
  	set_process_time(0);
  	mark_modification();
  }
  
  void DTR::set_process_time(const Arc::Period& process_time) {
    Arc::Time t;
    t = t + process_time;
    next_process_time.SetTime(t.GetTime(), t.GetTimeNanosec());
  }

  std::list<DTRCallback*> DTR::get_callbacks(const std::map<StagingProcesses,std::list<DTRCallback*> >& proc_callback, StagingProcesses owner) {
    std::list<DTRCallback*> l;
    lock.lock();
    std::map<StagingProcesses,std::list<DTRCallback*> >::const_iterator c = proc_callback.find(owner);
    if(c == proc_callback.end()) {
      lock.unlock();
      return l;
    }
    l = c->second;
    lock.unlock();
    return l;
  }

  void DTR::push(StagingProcesses new_owner)
  {
  	/* This function contains necessary operations
  	 * to pass the pointer to this DTR to another
  	 * process and make sure that the process accepted it
  	 */
    lock.lock();
    current_owner = new_owner;
    lock.unlock();

    std::list<DTRCallback*> callbacks = get_callbacks(proc_callback,current_owner);
    if (callbacks.empty())
      logger->msg(Arc::INFO, "DTR %s: No callback for %s defined", get_short_id(), get_owner_name(current_owner));

    for (std::list<DTRCallback*>::iterator callback = callbacks.begin();
        callback != callbacks.end(); ++callback) {
      switch(current_owner) {
        case GENERATOR:
        case SCHEDULER:
        case PRE_PROCESSOR:
        case DELIVERY:
        case POST_PROCESSOR:
        {
          // call registered callback
          if (*callback)
            (*callback)->receiveDTR(*this);
          else
            logger->msg(Arc::WARNING, "DTR %s: NULL callback for %s", get_short_id(), get_owner_name(current_owner));
        } break;
        default: // impossible
          logger->msg(Arc::INFO, "DTR %s: Request to push to unknown owner - %u", get_short_id(), (unsigned int)current_owner);
          break;
      }
    }
    mark_modification();
  }
  
  bool DTR::suspend()
  {
    /* This function will contain necessary operations
     * to stop the transfer in the DTR
     */
    mark_modification();
    return true;
  }
  
  bool DTR::is_destined_for_pre_processor() const {
  	return (status == DTRStatus::PRE_CLEAN || status == DTRStatus::CHECK_CACHE ||
  	   status == DTRStatus::RESOLVE || status == DTRStatus::QUERY_REPLICA ||
  	   status == DTRStatus::STAGE_PREPARE);
  }
  
  bool DTR::is_destined_for_post_processor() const {
  	return (status == DTRStatus::RELEASE_REQUEST || status == DTRStatus::REGISTER_REPLICA ||
  	   status == DTRStatus::PROCESS_CACHE);
  }
  
  bool DTR::is_destined_for_delivery() const {
  	return (status == DTRStatus::TRANSFER_WAIT);
  }
  
  bool DTR::came_from_pre_processor() const {
  	return (status == DTRStatus::PRE_CLEANED || status == DTRStatus::CACHE_WAIT ||
   	   status == DTRStatus::CACHE_CHECKED || status == DTRStatus::RESOLVED ||
   	   status == DTRStatus::REPLICA_QUERIED || 
   	   status == DTRStatus::STAGING_PREPARING_WAIT || 
   	   status == DTRStatus::STAGED_PREPARED);
  }
  
  bool DTR::came_from_post_processor() const {
  	return (status == DTRStatus::REQUEST_RELEASED || 
  	   status == DTRStatus::REPLICA_REGISTERED ||
   	   status == DTRStatus::CACHE_PROCESSED);
  }
  
  bool DTR::came_from_delivery() const {
  	return (status == DTRStatus::TRANSFERRED);
  }
  
  bool DTR::came_from_generator() const {
  	return (status == DTRStatus::NEW);
  }
  
  bool DTR::is_in_final_state() const {
  	return (status == DTRStatus::DONE || 
  	   status == DTRStatus::CANCELLED ||
   	   status == DTRStatus::ERROR);
  }
  
  void DTR::set_transfer_share(std::string share_name) {
  	lock.lock(); 
  	transfershare = share_name;
  	if (!sub_share.empty())
  	  transfershare += "-" + sub_share;
  	lock.unlock();
  }

  CacheParameters::CacheParameters(std::vector<std::string> caches,
                  std::vector<std::string> remote_caches,
                  std::vector<std::string> drain_caches):
       cache_dirs(caches),
       remote_cache_dirs(remote_caches),
       drain_cache_dirs(drain_caches) {
}

} // namespace DataStaging
