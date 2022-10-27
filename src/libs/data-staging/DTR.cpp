#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <utime.h>

#include <arc/FileLock.h>
#include <arc/GUID.h>
#include <arc/Utils.h>
#include <arc/data/FileInfo.h>

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
  Arc::LogLevel DTR::LOG_LEVEL(Arc::WARNING);

  DTR::DTR(const std::string& source,
           const std::string& destination,
           const Arc::UserConfig& usercfg,
           const std::string& jobid,
           const uid_t& uid,
           const std::list<DTRLogDestination>& logs,
           const std::string& logname)
    :  DTR_ID(""),
       source_url(source),
       destination_url(destination),
       cfg(usercfg),
       source_endpoint(source_url, cfg),
       destination_endpoint(destination_url, cfg),
       source_url_str(source_url.str()),
       destination_url_str(destination_url.str()),
       user(uid),
       parent_job_id(jobid),
       priority(50),
       transfershare("_default"),
       sub_share(""),
       tries_left(1),
       initial_tries(1),
       replication(false),
       force_registration(false),
       status(DTRStatus::NEW,"Created by the generator"),
       bytes_transferred(0),
       transfer_time(0),
       created(time(NULL)),
       cancel_request(false),
       bulk_start(false),
       bulk_end(false),
       source_supports_bulk(false),
       mandatory(true),
       delivery_endpoint(LOCAL_DELIVERY),
       use_host_cert_for_remote_delivery(false),
       current_owner(GENERATOR),
       log_destinations(logs),
       perf_record(perf_log)
  {
    logger = new Arc::Logger(Arc::Logger::getRootLogger(), logname.c_str());
    logger->addDestinations(get_log_destinations());

    // check that endpoints can be handled
    if (!source_endpoint || !(*source_endpoint)) {
      logger->msg(Arc::ERROR, "Could not handle endpoint %s", source);
      return;
    }
    if (!destination_endpoint || !(*destination_endpoint)) {
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

    // check for bulk support - call bulk methods with empty list
    std::list<Arc::DataPoint*> datapoints;
    if (source_endpoint->IsIndex()) {
      if (source_endpoint->Resolve(true, datapoints) == Arc::DataStatus::Success) source_supports_bulk = true;
    } else {
      std::list<Arc::FileInfo> files;
      if (source_endpoint->Stat(files, datapoints) == Arc::DataStatus::Success) source_supports_bulk = true;
    }

    cache_state = (source_endpoint->Cache() && destination_endpoint->Local()) ? CACHEABLE : NON_CACHEABLE;
    if (source_url.Option("failureallowed") == "yes" || destination_url.Option("failureallowed") == "yes") {
      mandatory = false;
    }
    
    /* Think how to populate transfer parameters */
    mark_modification();
    set_timeout(60);
    // setting ID last means all the previous steps have to pass for the DTR to be valid
    DTR_ID = Arc::UUID();
    // Prefix all log messages for this DTR with the short ID
    for (std::list<DTRLogDestination>::iterator dest = log_destinations.begin();
         dest != log_destinations.end(); ++dest) {
      (*dest)->setPrefix("DTR " + get_short_id() + ": ");
    }
  }

  std::list<Arc::LogDestination*> DTR::get_log_destinations() const {
    std::list<Arc::LogDestination*> log_dest;
    for (std::list<DTRLogDestination>::const_iterator dest = log_destinations.begin();
         dest != log_destinations.end(); ++dest) {
        log_dest.push_back(dest->Ptr());
    }
    return log_dest;
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
    // clear any transfer locations
    source_endpoint->ClearTransferLocations();
    // reset retry count to 1
    source_endpoint->SetTries(1);

    if (destination_endpoint->IsIndex()) {
      destination_endpoint->ClearLocations();
    }
    destination_endpoint->ClearTransferLocations();
    destination_endpoint->SetTries(1);

    // empty cache and map info
    cache_file.clear();
    mapped_source.clear();
    bytes_transferred = 0;
    transfer_time = 0;
    reset_error_status();
  }

  void DTR::set_id(const std::string& id) {
    // sanity check - regular expressions would be useful here
    if (id.length() != DTR_ID.length()) {
      logger->msg(Arc::WARNING, "Invalid ID: %s", id);
    } else {
      DTR_ID = id;
      // Change logging prefix to new ID
      for (std::list<DTRLogDestination>::iterator dest = log_destinations.begin();
           dest != log_destinations.end(); ++dest) {
        (*dest)->setPrefix("DTR " + get_short_id() + ": ");
      }
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
    initial_tries = tries;
    tries_left = initial_tries;
  }

  void DTR::decrease_tries_left() {
    if (tries_left > 0) tries_left--;
  }

  void DTR::set_status(DTRStatus stat)
  {
    logger->msg(Arc::VERBOSE, "%s->%s", status.str(), stat.str());
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
  }

  void DTR::set_transfer_time(unsigned long long int t) {
    transfer_time = t;
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
    next_process_time.SetTime(t.GetTime(), t.GetTimeNanoseconds());
  }

  bool DTR::bulk_possible() {
    if (status == DTRStatus::RESOLVE && source_supports_bulk) return true;
    if (status == DTRStatus::QUERY_REPLICA) {
      std::list<Arc::FileInfo> files;
      std::list<Arc::DataPoint*> datapoints;
      if (source_endpoint->CurrentLocationHandle()->Stat(files, datapoints) == Arc::DataStatus::Success) return true;
    }
    return false;
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

  void DTR::push(DTR_ptr dtr, StagingProcesses new_owner)
  {
    /* This function contains necessary operations
     * to pass the pointer to this DTR to another
     * process and make sure that the process accepted it
     */
    dtr->lock.lock();
    dtr->current_owner = new_owner;
    dtr->lock.unlock();

    std::list<DTRCallback*> callbacks = dtr->get_callbacks(dtr->proc_callback,dtr->current_owner);
    if (callbacks.empty())
      dtr->logger->msg(Arc::INFO, "No callback for %s defined", get_owner_name(dtr->current_owner));

    for (std::list<DTRCallback*>::iterator callback = callbacks.begin();
        callback != callbacks.end(); ++callback) {
      switch(dtr->current_owner) {
        case GENERATOR:
        case SCHEDULER:
        case PRE_PROCESSOR:
        case DELIVERY:
        case POST_PROCESSOR:
        {
          // call registered callback
          if (*callback)
            (*callback)->receiveDTR(dtr);
          else
            dtr->logger->msg(Arc::WARNING, "NULL callback for %s", get_owner_name(dtr->current_owner));
        } break;
        default: // impossible
          dtr->logger->msg(Arc::INFO, "Request to push to unknown owner - %u", (unsigned int)dtr->current_owner);
          break;
      }
    }
    dtr->mark_modification();
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
    return (status == DTRStatus::RELEASE_REQUEST || status == DTRStatus::FINALISE_REPLICA ||
            status == DTRStatus::REGISTER_REPLICA || status == DTRStatus::PROCESS_CACHE);
  }
  
  bool DTR::is_destined_for_delivery() const {
    return (status == DTRStatus::TRANSFER);
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
            status == DTRStatus::REPLICA_FINALISED ||
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
  
  void DTR::set_transfer_share(const std::string& share_name) {
    lock.lock();
    transfershare = share_name;
    if (!sub_share.empty())
      transfershare += "-" + sub_share;
    lock.unlock();
  }

  DTRCacheParameters::DTRCacheParameters(std::vector<std::string> caches,
                                         std::vector<std::string> drain_caches,
                                         std::vector<std::string> readonly_caches):
       cache_dirs(caches),
       drain_cache_dirs(drain_caches),
       readonly_cache_dirs(readonly_caches) {
  }

  DTRCredentialInfo::DTRCredentialInfo(const std::string& DN,
                                       const Arc::Time& expirytime,
                                       const std::list<std::string> vomsfqans):
       DN(DN),
       expirytime(expirytime),
       vomsfqans(vomsfqans) {
  }

  std::string DTRCredentialInfo::extractVOMSVO() const {
    if (vomsfqans.empty()) return "";
    std::vector<std::string> parts;
    Arc::tokenize(*(vomsfqans.begin()), parts, "/");
    return parts.at(0);
  }

  std::string DTRCredentialInfo::extractVOMSGroup() const {
    if (vomsfqans.empty()) return "";
    std::string vomsvo;
    for (std::list<std::string>::const_iterator i = vomsfqans.begin(); i != vomsfqans.end(); ++i) {
      std::vector<std::string> parts;
      Arc::tokenize(*i, parts, "/");
      if (vomsvo.empty()) vomsvo = parts.at(0);
      if (parts.size() > 1 && parts.at(1).find("Role=") != 0) {
        return std::string(vomsvo+":"+parts.at(1));
      }
    }
    return std::string(vomsvo + ":null");
  }

  std::string DTRCredentialInfo::extractVOMSRole() const {
    if (vomsfqans.empty()) return "";
    std::string vomsvo;
    for (std::list<std::string>::const_iterator i = vomsfqans.begin(); i != vomsfqans.end(); ++i) {
      std::vector<std::string> parts;
      Arc::tokenize(*i, parts, "/");
      if (vomsvo.empty()) vomsvo = parts.at(0);
      if (parts.size() > 1 && parts.at(1).find("Role=") == 0) {
        return std::string(parts.at(0)+":"+parts.at(1).substr(5));
      }
    }
    return std::string(vomsvo + ":null");
  }

  DTR_ptr createDTRPtr(const std::string& source,
                       const std::string& destination,
                       const Arc::UserConfig& usercfg,
                       const std::string& jobid,
                       const uid_t& uid,
                       const std::list<DTRLogDestination>& logs,
                       const std::string& logname) {
    return DTR_ptr(new DTR(source, destination, usercfg, jobid, uid, logs, logname));
  }

  DTRLogger createDTRLogger(Arc::Logger& parent,
                            const std::string& subdomain) {
    return DTRLogger(new Arc::Logger(parent, subdomain));
  }

} // namespace DataStaging
