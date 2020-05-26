#include <arc/FileAccess.h>
#include <arc/Utils.h>

#include "../a-rex/grid-manager/conf/UrlMapConfig.h"

#include "CandyPondGenerator.h"

namespace CandyPond {

  Arc::Logger CandyPondGenerator::logger(Arc::Logger::rootLogger, "CandyPondGenerator");

  CandyPondGenerator::CandyPondGenerator(const ARex::GMConfig& conf, bool with_arex) :
      generator_state(DataStaging::INITIATED),
      use_host_cert(false),
      scratch_dir(conf.ScratchDir()),
      run_with_arex(with_arex),
      config(conf),
      staging_conf(config) {

    scheduler = DataStaging::Scheduler::getInstance();

    if (run_with_arex) {
      // A-REX sets DTR configuration
      generator_state = DataStaging::RUNNING;
      return;
    }

    if (!staging_conf) return;

    // Convert A-REX configuration values to DTR configuration

    // TODO find location for DTR state log, should be different from A-REX's

    // Log level for DTR
    DataStaging::DTR::LOG_LEVEL = staging_conf.get_log_level();

    // Processing limits
    scheduler->SetSlots(staging_conf.get_max_processor(),
                        staging_conf.get_max_processor(),
                        staging_conf.get_max_delivery(),
                        staging_conf.get_max_emergency(),
                        staging_conf.get_max_prepared());

    // Transfer shares
    DataStaging::TransferSharesConf share_conf(staging_conf.get_share_type(),
                                               staging_conf.get_defined_shares());
    scheduler->SetTransferSharesConf(share_conf);

    // Transfer limits
    DataStaging::TransferParameters transfer_limits;
    transfer_limits.min_current_bandwidth = staging_conf.get_min_speed();
    transfer_limits.averaging_time = staging_conf.get_min_speed_time();
    transfer_limits.min_average_bandwidth = staging_conf.get_min_average_speed();
    transfer_limits.max_inactivity_time = staging_conf.get_max_inactivity_time();
    scheduler->SetTransferParameters(transfer_limits);

    // URL mappings
    ARex::UrlMapConfig url_map(config);
    scheduler->SetURLMapping(url_map);

    // Preferred pattern
    scheduler->SetPreferredPattern(staging_conf.get_preferred_pattern());

    // Delivery services
    scheduler->SetDeliveryServices(staging_conf.get_delivery_services());

    // Limit on remote delivery size
    scheduler->SetRemoteSizeLimit(staging_conf.get_remote_size_limit());

    // Set whether to use host cert for remote delivery
    use_host_cert = staging_conf.get_use_host_cert_for_remote_delivery();

    // End of configuration - start Scheduler thread
    scheduler->start();

    generator_state = DataStaging::RUNNING;
  }

  CandyPondGenerator::~CandyPondGenerator() {
    generator_state = DataStaging::STOPPED;
    if (!run_with_arex) scheduler->stop();
    // delete scheduler? it is possible another thread is using the static instance
  }

  void CandyPondGenerator::receiveDTR(DataStaging::DTR_ptr dtr) {

    // Take DTR out of processing map and add to finished jobs
    logger.msg(Arc::INFO, "DTR %s finished with state %s", dtr->get_id(), dtr->get_status().str());
    std::string jobid (dtr->get_parent_job_id());

    // Delete LogStreams and LogDestinations
    dtr->clean_log_destinations();

    // Add to finished jobs
    std::string error_msg;
    if (dtr->error()) error_msg = dtr->get_error_status().GetDesc() + ". ";
    finished_lock.lock();
    finished_jobs[jobid] += error_msg;
    finished_lock.unlock();

    // remove from processing jobs
    processing_lock.lock();
    std::pair<std::multimap<std::string, DataStaging::DTR_ptr>::iterator,
              std::multimap<std::string, DataStaging::DTR_ptr>::iterator> dtr_iterator = processing_dtrs.equal_range(jobid);

    if (dtr_iterator.first == dtr_iterator.second) {
      processing_lock.unlock();
      logger.msg(Arc::WARNING, "No active job id %s", jobid);
      return;
    }

    // remove this DTR from the processing list
    for (std::multimap<std::string, DataStaging::DTR_ptr>::iterator i = dtr_iterator.first; i != dtr_iterator.second; ++i) {
      if (i->second->get_id() == dtr->get_id()) {
        processing_dtrs.erase(i);
        break;
      }
    }
    processing_lock.unlock();

    // Move to scratch if necessary
    if (!dtr->error() && !scratch_dir.empty()) {
      // Get filename relative to session dir
      std::string session_file = dtr->get_destination()->GetURL().Path();
      std::string::size_type pos = session_file.find(jobid);
      if (pos == std::string::npos) {
        logger.msg(Arc::ERROR, "Could not determine session directory from filename %s", session_file);
        finished_lock.lock();
        finished_jobs[jobid] += "Could not determine session directory from filename for during move to scratch. ";
        finished_lock.unlock();
        return;
      }
      std::string scratch_file(scratch_dir+'/'+session_file.substr(pos));
      // Access session and scratch under mapped uid
      Arc::FileAccess fa;
      if (!fa.fa_setuid(dtr->get_local_user().get_uid(), dtr->get_local_user().get_gid()) ||
          !fa.fa_rename(session_file, scratch_file)) {
        logger.msg(Arc::ERROR, "Failed to move %s to %s: %s", session_file, scratch_file, Arc::StrError(errno));
        finished_lock.lock();
        finished_jobs[jobid] += "Failed to move file from session dir to scratch. ";
        finished_lock.unlock();
      }
    }
  }

  bool CandyPondGenerator::addNewRequest(const Arc::User& user,
                                            const std::string& source,
                                            const std::string& destination,
                                            const Arc::UserConfig& usercfg,
                                            const std::string& jobid,
                                            int priority) {

    if (generator_state != DataStaging::RUNNING) return false;

    // Logger for this DTR. Uses a string stream to keep log in memory rather
    // than a file. LogStream keeps a reference to the stream so we have to use
    // a pointer. The LogDestinations are deleted when the DTR is received back.
    // TODO: provide access to this log somehow
    std::stringstream * stream = new std::stringstream();
    Arc::LogDestination * output = new Arc::LogStream(*stream);
    DataStaging::DTRLogger log(new Arc::Logger(Arc::Logger::getRootLogger(), "DataStaging"));
    log->removeDestinations();
    log->addDestination(*output);

    DataStaging::DTR_ptr dtr(new DataStaging::DTR(source, destination, usercfg, jobid, user.get_uid(), log));

    if (!(*dtr)) {
      logger.msg(Arc::ERROR, "Invalid DTR for source %s, destination %s", source, destination);
      log->deleteDestinations();
      return false;
    }
    // set retry count (tmp errors only)
    dtr->set_tries_left(staging_conf.get_max_retries());
    // set priority
    dtr->set_priority(priority);
    // set whether to use A-REX host certificate for remote delivery services
    dtr->host_cert_for_remote_delivery(use_host_cert);
    // use a separate share from A-REX downloads
    dtr->set_sub_share("candypond-download");

    // substitute cache paths based on user
    ARex::CacheConfig cache_params(config.CacheParams());
    cache_params.substitute(config, user);
    DataStaging::DTRCacheParameters cache_parameters;
    cache_parameters.cache_dirs = cache_params.getCacheDirs();
    // we are definitely going to download so read-only caches are not useful here
    dtr->set_cache_parameters(cache_parameters);
    dtr->registerCallback(this, DataStaging::GENERATOR);
    dtr->registerCallback(scheduler, DataStaging::SCHEDULER);

    processing_lock.lock();
    processing_dtrs.insert(std::pair<std::string, DataStaging::DTR_ptr>(jobid, dtr));
    processing_lock.unlock();

    // Avoid logging when possible during scheduler submission because it gets
    // blocked by LFC calls locking the environment
    Arc::LogLevel log_level = Arc::Logger::getRootLogger().getThreshold();
    Arc::Logger::getRootLogger().setThreshold(Arc::ERROR);
    DataStaging::DTR::push(dtr, DataStaging::SCHEDULER);
    Arc::Logger::getRootLogger().setThreshold(log_level);

    return true;
  }

  bool CandyPondGenerator::queryRequestsFinished(const std::string& jobid, std::string& error) {

    // First check currently processing DTRs
    processing_lock.lock();
    if (processing_dtrs.find(jobid) != processing_dtrs.end()) {
      logger.msg(Arc::VERBOSE, "DTRs still running for job %s", jobid);
      processing_lock.unlock();
      return false;
    }
    processing_lock.unlock();

    // Now check finished jobs
    finished_lock.lock();
    if (finished_jobs.find(jobid) != finished_jobs.end()) {
      logger.msg(Arc::VERBOSE, "All DTRs finished for job %s", jobid);
      error = finished_jobs[jobid];
      finished_lock.unlock();
      return true;
    }

    // Job not running or finished - report error
    logger.msg(Arc::WARNING, "Job %s not found", jobid);
    error = "Job not found";
    return true;
  }

} // namespace CandyPond
