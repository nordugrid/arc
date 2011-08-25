#include <sys/stat.h>
#include <fcntl.h>

#include <arc/FileUtils.h>
#include <arc/data/FileCache.h>

#include <arc/data-staging/Scheduler.h>

#include "../files/delete.h"
#include "../conf/conf_map.h"

#include "dtr_generator.h"

Arc::Logger DTRInfo::logger(Arc::Logger::getRootLogger(), "DTRInfo");

DTRInfo::DTRInfo(const JobUsers& users) {
  for (JobUsers::const_iterator i = users.begin(); i != users.end(); ++i) {
    jobusers[i->get_uid()] = &(*i);
  }
}

void DTRInfo::receiveDTR(DataStaging::DTR& dtr) {
  // write state info to job.id.input for example
}

Arc::Logger DTRGenerator::logger(Arc::Logger::getRootLogger(), "Generator");

bool compare_job_description(JobDescription first, JobDescription second) {
  return first.get_local()->priority > second.get_local()->priority;
}

void DTRGenerator::main_thread(void* arg) {
  ((DTRGenerator*)arg)->thread();
}

void DTRGenerator::thread() {

  // set up logging - to avoid logging DTR logs to the main A-REX log
  // we disconnect the root logger while submitting to the Scheduler
  Arc::Logger::getRootLogger().setThreadContext();

  while (generator_state != DataStaging::TO_STOP) {
    // look at event queue and deal with any events.
    // This method of iteration should be thread-safe because events
    // are always added to the end of the list

    // take cancelled jobs first so we can ignore other DTRs in those jobs
    event_lock.lock();
    std::list<std::string>::iterator it_cancel = jobs_cancelled.begin();
    while (it_cancel != jobs_cancelled.end()) {
      event_lock.unlock();
      processCancelledJob(*it_cancel);
      event_lock.lock();
      it_cancel = jobs_cancelled.erase(it_cancel);
    }

    // next DTRs sent back from the Scheduler
    std::list<DataStaging::DTR>::iterator it_dtrs = dtrs_received.begin();
    while (it_dtrs != dtrs_received.end()) {
      event_lock.unlock();
      processReceivedDTR(*it_dtrs);
      event_lock.lock();
      // delete DTR Logger and LogDestinations
      const std::list<Arc::LogDestination*> log_dests = it_dtrs->get_logger()->getDestinations();
      for (std::list<Arc::LogDestination*>::const_iterator i = log_dests.begin(); i != log_dests.end(); ++i)
        delete *i;
      delete it_dtrs->get_logger();

      it_dtrs = dtrs_received.erase(it_dtrs);
    }

    // finally new jobs
    std::list<JobDescription>::iterator it_jobs = jobs_received.begin();
    // it can happen that the list grows faster than the jobs are processed
    // so here we only process for a small time to avoid blocking other
    // jobs finishing
    Arc::Time limit(Arc::Time() + Arc::Period(30));

    // sort the list by job priority
    jobs_received.sort(compare_job_description);
    while (it_jobs != jobs_received.end() && Arc::Time() < limit) {
      event_lock.unlock();
      processReceivedJob(*it_jobs);
      event_lock.lock();
      it_jobs = jobs_received.erase(it_jobs);
    }
    event_lock.unlock();
    Glib::usleep(50000);
  }
  // stop scheduler - cancels all DTRs and waits for them to complete
  scheduler.stop();
  run_condition.signal();
  logger.msg(Arc::INFO, "Exiting Generator thread");
}

DTRGenerator::DTRGenerator(const JobUsers& users,
                           void (*kicker_func)(void*),
                           void* kicker_arg) :
    generator_state(DataStaging::INITIATED),
    info(users),
    kicker_func(kicker_func),
    kicker_arg(kicker_arg) {
  //if (generator_state == DataStaging::RUNNING || generator_state == DataStaging::TO_STOP)
  //  return false;
  generator_state = DataStaging::RUNNING;
  for (JobUsers::const_iterator i = users.begin(); i != users.end(); ++i) {
    jobusers[i->get_uid()] = &(*i);
  }
  // set the DTR dump file to the first control dir registered
  if (!jobusers.empty())
    scheduler.SetDumpLocation(std::string(jobusers.begin()->second->ControlDir()+"/dtrstate.log"));

  //  DataStaging::DTR::registerCallback(&DTRGenerator::receiveDTR);

  JobsListConfig& jcfg = users.Env().jobs_cfg();

  // Converting old configuration values to something
  // useful for new framework

  // processing limits
  int max_processing;
  int max_processing_emergency;
  int max_downloads;
  jcfg.GetMaxJobsLoad(max_processing,max_processing_emergency,max_downloads);
  if (max_processing > 0 && max_downloads > 0) max_processing *= max_downloads;
  if (max_processing_emergency > 0 && max_downloads > 0) max_processing_emergency *= max_downloads;
  scheduler.SetSlots(max_processing,max_processing,max_processing,max_processing_emergency);

  // transfer shares
  DataStaging::TransferShares shares;
  shares.set_reference_shares(jcfg.GetLimitedShares());
  shares.set_share_type(jcfg.GetShareType());
  scheduler.SetTransferShares(shares);

  // transfer limits
  DataStaging::TransferParameters transfer_limits;
  unsigned long long int min_speed, min_average_speed;
  time_t min_time, max_inactivity_time;
  jcfg.GetSpeedControl(min_speed, min_time, min_average_speed, max_inactivity_time);
  transfer_limits.min_current_bandwidth = min_speed;
  transfer_limits.averaging_time = min_time;
  transfer_limits.min_average_bandwidth = min_average_speed;
  transfer_limits.max_inactivity_time = max_inactivity_time;
  scheduler.SetTransferParameters(transfer_limits);

  // URL mappings
  UrlMapConfig url_map(users.Env());
  scheduler.SetURLMapping(url_map);

  // preferred pattern
  scheduler.SetPreferredPattern(jcfg.GetPreferredPattern());

  scheduler.start();

  Arc::CreateThreadFunction(&main_thread, this);
  return; // true;
}

DTRGenerator::~DTRGenerator() {
  if (generator_state != DataStaging::RUNNING)
    return;
  generator_state = DataStaging::TO_STOP;
  run_condition.wait();
  generator_state = DataStaging::STOPPED;
}

void DTRGenerator::receiveDTR(DataStaging::DTR& dtr) {

  if (generator_state == DataStaging::INITIATED || generator_state == DataStaging::STOPPED) {
    logger.msg(Arc::ERROR, "DTRGenerator is not running!");
    return;
  } else if (generator_state == DataStaging::TO_STOP) {
    logger.msg(Arc::VERBOSE, "Received DTR %s during Generator shutdown - may not be processed", dtr.get_id());
    // still a chance to process this DTR so don't return
  }
  event_lock.lock();
  dtrs_received.push_back(dtr);
  event_lock.unlock();
}

void DTRGenerator::receiveJob(const JobDescription& job) {

  if (generator_state != DataStaging::RUNNING) {
    logger.msg(Arc::WARNING, "DTRGenerator is not running!");
  }

  // Add to jobs list even if Generator is stopped, so that A-REX doesn't
  // think that staging has finished.
  event_lock.lock();
  jobs_received.push_back(job);
  event_lock.unlock();
}

void DTRGenerator::cancelJob(const JobDescription& job) {

  if (generator_state != DataStaging::RUNNING) {
    logger.msg(Arc::WARNING, "DTRGenerator is not running!");
  }

  event_lock.lock();
  jobs_cancelled.push_back(job.get_id());
  event_lock.unlock();
}

bool DTRGenerator::queryJobFinished(JobDescription& job) {

  // Data staging is finished if the job is in finished_jobs and
  // not in active_dtrs or jobs_received.

  // check if this job is still in the received jobs queue
  event_lock.lock();
  for (std::list<JobDescription>::iterator i = jobs_received.begin(); i != jobs_received.end(); ++i) {
    if (*i == job) {
      event_lock.unlock();
      return false;
    }
  }
  event_lock.unlock();

  // check if any DTRs in this job are still active
  lock.lock();
  if (active_dtrs.find(job.get_id()) != active_dtrs.end()) {
    lock.unlock();
    return false;
  }
  std::map<std::string, std::string>::iterator i = finished_jobs.find(job.get_id());
  if (i == finished_jobs.end()) {
    // not in active or finished - must have been picked up already
    // or nothing to transfer
    lock.unlock();
    return true;
  }
  // add failure to job if any DTR failed
  if (!i->second.empty())
    job.AddFailure(i->second);
  finished_jobs.erase(i);
  lock.unlock();
  return true;
}


bool DTRGenerator::processReceivedDTR(DataStaging::DTR& dtr) {

  std::string jobid(dtr.get_parent_job_id());
  logger.msg(Arc::DEBUG, "%s: Received DTR %s to copy file %s in state %s",
                          jobid, dtr.get_id(), dtr.get_source()->str(), dtr.get_status().str());

  // JobUser object
  std::map<uid_t, const JobUser*>::iterator it = jobusers.find(dtr.get_local_user().get_uid());
  if (it == jobusers.end()) {
    it = jobusers.find(0);
    if (it == jobusers.end()) {
      // check if already cancelled
      if (dtr.get_status() != DataStaging::DTRStatus::CANCELLED) {
        logger.msg(Arc::ERROR, "%s: No configured user found for uid %i",
                   dtr.get_parent_job_id(), dtr.get_local_user().get_uid());
        // cancel other DTRs (which will also fail here anyway)
        scheduler.cancelDTRs(jobid);
        lock.lock();
        finished_jobs[jobid] = std::string("Internal configuration error in data staging");
        lock.unlock();
      }
      lock.lock();
      active_dtrs.erase(jobid);
      lock.unlock();
      return false;
    }
  }
  const JobUser* jobuser(it->second);

  // create JobDescription object to pass to job_..write_file methods
  std::string session_dir(jobuser->SessionRoot(jobid) + '/' + jobid);
  JobDescription job(jobid, session_dir);
  job.set_uid(dtr.get_local_user().get_uid(), dtr.get_local_user().get_gid());
  std::string dtr_transfer_statistics;
  
  if (dtr.error() && dtr.get_status() != DataStaging::DTRStatus::CANCELLED) {
    // for uploads, report error but let other transfers continue
    // for downloads, cancel all other transfers
    logger.msg(Arc::ERROR, "%s: DTR %s to copy file %s failed",
               jobid, dtr.get_id(), dtr.get_source()->str());

    lock.lock();
    if (!dtr.get_source()->Local() && finished_jobs.find(jobid) == finished_jobs.end()) { // download
      // cancel other DTRs and erase from our list unless error was already reported
      logger.msg(Arc::INFO, "%s: Cancelling other DTRs", jobid);
      scheduler.cancelDTRs(jobid);
    }
    // add error to finished jobs
    finished_jobs[jobid] += std::string("Failed in data staging: " + dtr.get_error_status().GetDesc() + '\n');
    lock.unlock();
  }
  else if (dtr.get_status() != DataStaging::DTRStatus::CANCELLED) {
    // remove from job.id.input/output files on success
    // find out if download or upload by checking which is remote file
    std::list<FileData> files;

    if (dtr.get_source()->Local()) {
      // output files
      dtr_transfer_statistics = "outputfile:url=" + dtr.get_destination()->str() + ',';

      if (!job_output_read_file(jobid, *jobuser, files)) {
        logger.msg(Arc::WARNING, "%s: Failed to read list of output files", jobid);
      } else {
        // go through list and take out this file
        for (std::list<FileData>::iterator i = files.begin(); i != files.end();) {
          // compare 'standard' URLs
          Arc::URL file_lfn(i->lfn);
          Arc::URL dtr_lfn(dtr.get_destination()->str());

          // check if it is in a dynamic list - if so remove from it
          if (i->pfn.size() > 1 && i->pfn[1] == '@') {
            std::string dynamic_output(session_dir+'/'+i->pfn.substr(2));
            std::list<FileData> dynamic_files;
            if (!job_Xput_read_file(dynamic_output, dynamic_files)) {
              logger.msg(Arc::WARNING, "%s: Failed to read dynamic output files in %s", jobid, dynamic_output);
            } else {
              logger.msg(Arc::DEBUG, "%s: Going through files in list %s", jobid, dynamic_output);
              for (std::list<FileData>::iterator dynamic_file = dynamic_files.begin();
                   dynamic_file != dynamic_files.end(); ++dynamic_file) {
                if (Arc::URL(dynamic_file->lfn).str() == dtr_lfn.str()) {
                  logger.msg(Arc::DEBUG, "%s: Removing %s from dynamic output file %s", jobid, dtr_lfn.str(), dynamic_output);
                  dynamic_files.erase(dynamic_file);
                  if (!job_Xput_write_file(dynamic_output, dynamic_files))
                    logger.msg(Arc::WARNING, "%s: Failed to write back dynamic output files in %s", jobid, dynamic_output);
                  break;
                }
              }
            }
          }
          if (file_lfn.str() == dtr_lfn.str()) {
          	struct stat st;
            Arc::FileStat(job.SessionDir() + i->pfn, &st, true);
            dtr_transfer_statistics += "size=" + Arc::tostring(st.st_size) + ',';
            i = files.erase(i);
          } else {
            ++i;
          }
        }

        // write back .output file
        if (!job_output_write_file(job, *jobuser, files)) {
          logger.msg(Arc::WARNING, "%s: Failed to write list of output files", jobid);
        }
      }
      dtr_transfer_statistics += "starttime=" + dtr.get_creation_time().str(Arc::UTCTime) + ',';
      dtr_transfer_statistics += "endtime=" + Arc::Time().str(Arc::UTCTime); 
    }
    else if (dtr.get_destination()->Local()) {
      // input files
      struct stat st;
      Arc::FileStat(dtr.get_destination()->str(), &st, true);
      dtr_transfer_statistics = "inputfile:url=" + dtr.get_source()->str() + ',';
      if (!job_input_read_file(jobid, *jobuser, files)) {
        logger.msg(Arc::WARNING,"%s: Failed to read list of input files", jobid);
      } else {
        // go through list and take out this file
        for (std::list<FileData>::iterator i = files.begin(); i != files.end();) {
          // compare 'standard' URLs
          Arc::URL file_lfn(i->lfn);
          Arc::URL dtr_lfn(dtr.get_source()->str());
          if (file_lfn.str() == dtr_lfn.str()) {
            struct stat st;
            Arc::FileStat(job.SessionDir() + i->pfn, &st, true);
            dtr_transfer_statistics += "size=" + Arc::tostring(st.st_size) + ',';
            i = files.erase(i);
            break;
          } else {
            ++i;
          }
        }
        // write back .input file
        if (!job_input_write_file(job, *jobuser, files)) {
          logger.msg(Arc::WARNING, "%s: Failed to write list of input files", jobid);
        }
      }
      dtr_transfer_statistics += "starttime=" + dtr.get_creation_time().str(Arc::UTCTime) + ',';
      dtr_transfer_statistics += "endtime=" + Arc::Time().str(Arc::UTCTime) + ',';
      if (dtr.get_cache_state() == DataStaging::CACHE_ALREADY_PRESENT)
        dtr_transfer_statistics += "fromcache=yes";
      else
        dtr_transfer_statistics += "fromcache=no"; 
    }
    else {
      // transfer between two remote endpoints, shouldn't happen...
      logger.msg(Arc::WARNING, "%s: Received DTR with two remote endpoints!");
    }
  }

  // get DTRs for this job id
  lock.lock();
  std::pair<std::multimap<std::string, std::string>::iterator,
            std::multimap<std::string, std::string>::iterator> dtr_iterator = active_dtrs.equal_range(jobid);

  if (dtr_iterator.first == dtr_iterator.second) {
    lock.unlock();
    logger.msg(Arc::WARNING, "No active job id %s", jobid);
    return true;
  }
  
  // Print transfer statistics
  std::string fname = jobuser->ControlDir() + "/job." + job.get_id() + ".statistics";
  std::ofstream f(fname.c_str(),std::ios::out | std::ios::app);
  if(f.is_open() ) {
    f << dtr_transfer_statistics << std::endl;  
  }
  f.close();
  
  // remove this DTR from list
  for (std::multimap<std::string, std::string>::iterator i = dtr_iterator.first; i != dtr_iterator.second; ++i) {
    if (i->second == dtr.get_id()) {
      active_dtrs.erase(i);
      break;
    }
  }

  // check if any DTRs left from this job, if so return
  if (active_dtrs.find(jobid) != active_dtrs.end()) {
    lock.unlock();
    return true;
  }

  // no DTRs left, clean up session dir if upload or failed download
  if (dtr.get_source()->Local()) {
    std::list<FileData> files;
    // if error, don't remove dynamic output files so that resume will work
    if (!job_output_read_file(jobid, *jobuser, files))
      logger.msg(Arc::WARNING, "%s: Failed to read list of output files, can't clean up session dir", jobid);
    else {
      if (finished_jobs.find(jobid) != finished_jobs.end() && !finished_jobs[jobid].empty()) {
        for (std::list<FileData>::iterator i = files.begin(); i != files.end(); ++i) {
          if (i->pfn.size() > 1 && i->pfn[1] == '@') {
            FileData fd(std::string(i->pfn.erase(1,1)), "");
            files.push_back(fd);
          }
        }
      }
      if (delete_all_files(job.SessionDir(), files, true) == 2)
        logger.msg(Arc::WARNING, "%s: Failed to clean up session dir", jobid);
    }
  }
  else if (finished_jobs.find(jobid) != finished_jobs.end() && !finished_jobs[jobid].empty()) {
    // clean all files still in input list which could be half-downloaded
    std::list<FileData> files;
    if (!job_input_read_file(jobid, *jobuser, files))
      logger.msg(Arc::WARNING, "%s: Failed to read list of input files, can't clean up session dir", jobid);
    else if (delete_all_files(job.SessionDir(), files, false, true, false) == 2)
      logger.msg(Arc::WARNING, "%s: Failed to clean up session dir", jobid);
  }
  // add to finished jobs (without overwriting existing error)
  finished_jobs[jobid] += "";

  // log summary to DTR log
  const std::list<Arc::LogDestination*> log_dests = Arc::Logger::getRootLogger().getDestinations();
  Arc::Logger::getRootLogger().removeDestinations();

  if (finished_jobs[jobid].empty())
    dtr.get_logger()->msg(Arc::INFO, "All %s %s successfully",
                          dtr.get_source()->Local() ? "uploads":"downloads",
                          (dtr.get_status() == DataStaging::DTRStatus::CANCELLED) ? "cancelled":"finished");
  else
    dtr.get_logger()->msg(Arc::INFO, "Some %s failed",
                          dtr.get_source()->Local() ? "uploads":"downloads");

  lock.unlock();
  Arc::Logger::getRootLogger().addDestinations(log_dests);

  logger.msg(Arc::INFO, "%s: Data staging finished", jobid);

  // wake up GM thread
  if (kicker_func)
    (*kicker_func)(kicker_arg);

  return true;
}


bool DTRGenerator::processReceivedJob(const JobDescription& job) {

  JobId jobid(job.get_id());
  logger.msg(Arc::VERBOSE, "%s: Received data staging request to %s files", jobid,
             (job.get_state() == JOB_STATE_PREPARING ? "download" : "upload"));

  // JobUser object
  std::map<uid_t, const JobUser*>::iterator it = jobusers.find(job.get_uid());
  if (it == jobusers.end()) {
    it = jobusers.find(0);
    if (it == jobusers.end()) {
      logger.msg(Arc::ERROR, "%s: No configured user found for uid %i", jobid, job.get_uid());
      lock.lock();
      finished_jobs[jobid] = std::string("Internal configuration error in data staging");
      lock.unlock();
      if (kicker_func) (*kicker_func)(kicker_arg);
      return false;
    }
  }
  const JobUser* jobuser(it->second);
  const GMEnvironment& env = jobuser->Env();
  
  // Create a file for the transfer statistics and fix its permissions
  std::string fname = jobuser->ControlDir() + "/job." + jobid + ".statistics";
  std::ofstream f(fname.c_str(),std::ios::out | std::ios::app);
  f.close();
  fix_file_permissions(fname);

  // read in input/output files
  std::list<FileData> files;
  bool replication = false;

  // output files need to be read whether PREPARING or FINISHING
  std::list<FileData> output_files;
  if (!job_output_read_file(jobid, *jobuser, output_files)) {
    logger.msg(Arc::ERROR, "%s: Failed to read list of output files", jobid);
    lock.lock();
    finished_jobs[jobid] = std::string("Failed to read list of output files");
    lock.unlock();
    if (kicker_func) (*kicker_func)(kicker_arg);
    return false;
  }

  if (job.get_state() == JOB_STATE_PREPARING) {
    if (!job_input_read_file(jobid, *jobuser, files)) {
      logger.msg(Arc::ERROR, "%s: Failed to read list of input files", jobid);
      lock.lock();
      finished_jobs[jobid] = std::string("Failed to read list of input files");
      lock.unlock();
      if (kicker_func) (*kicker_func)(kicker_arg);
      return false;
    }
    // check for duplicates (see bug 1285)
    for (std::list<FileData>::iterator i = files.begin(); i != files.end(); i++) {
      for (std::list<FileData>::iterator j = files.begin(); j != files.end(); j++) {
        if (i != j && j->pfn == i->pfn) {
          logger.msg(Arc::ERROR, "%s: Duplicate file in list of input files: %s", jobid, i->pfn);
          lock.lock();
          finished_jobs[jobid] = std::string("Duplicate file in list of input files: " + i->pfn);
          lock.unlock();
          if (kicker_func) (*kicker_func)(kicker_arg);
          return false;
        }
      }
    }
    // check if any input files are also output files downloadable by user (bug 1387)
    for (std::list<FileData>::iterator j = output_files.begin(); j != output_files.end(); j++) {
      for (std::list<FileData>::iterator i = files.begin(); i != files.end(); i++) {
        if (i->pfn == j->pfn && j->lfn.empty() && i->lfn.find(':') != std::string::npos) {
          Arc::URL u(i->lfn);
          std::string opt = u.Option("cache");
          // don't add copy option if exists or current option is "no" or "renew"
          if (opt.empty() || !(opt == "no" || opt == "renew" || opt == "copy")) {
            u.AddOption("cache", "copy", true);
            i->lfn = u.fullstr();
          }
        }
      }
    }
    // pre-clean session dir before downloading
    if (delete_all_files(job.SessionDir(), files, false, true, false) == 2) {
      logger.msg(Arc::ERROR, "%s: Failed to clean up session dir", jobid);
      lock.lock();
      finished_jobs[jobid] = std::string("Failed to clean up session dir before downloading inputs");
      lock.unlock();
      if (kicker_func) (*kicker_func)(kicker_arg);
      return false;
    }
  }
  else if (job.get_state() == JOB_STATE_FINISHING) {
    files = output_files;
    std::list<FileData>::iterator it;
    // add any output files dynamically added by the user during the job
    for (it = files.begin(); it != files.end() ; ++it) {
      if (it->pfn.find("@") == 1) { // GM puts a slash on the front of the local file
        std::string outputfilelist = job.SessionDir() + std::string("/") + it->pfn.substr(2);
        logger.msg(Arc::INFO, "%s: Reading output files from user generated list in %s", jobid, outputfilelist);
        if (!job_Xput_read_file(outputfilelist, files)) {
          logger.msg(Arc::ERROR, "%s: Error reading user generated output file list in %s", jobid, outputfilelist);
          lock.lock();
          finished_jobs[jobid] = std::string("Error reading user generated output file list");
          lock.unlock();
          if (kicker_func) (*kicker_func)(kicker_arg);
          return false;
        }
        it->pfn.erase(1, 1);
      }
    }
    // check if any files share the same LFN, if so allow overwriting existing LFN
    for (it = files.begin(); it != files.end(); it++) {
      bool done = false;
      for (std::list<FileData>::iterator it2 = files.begin(); it2 != files.end(); it2++) {
        if (it != it2 && !it->lfn.empty() && !it2->lfn.empty()) {
          // error if lfns (including locations) are identical
          if (it->lfn == it2->lfn) {
            logger.msg(Arc::ERROR, "%s: Two identical output destinations: %s", jobid, it->lfn);
            lock.lock();
            finished_jobs[jobid] = std::string("Two identical output destinations: " + it->lfn);
            lock.unlock();
            if (kicker_func) (*kicker_func)(kicker_arg);
            return false;
          }
          Arc::URL u_it(it->lfn);
          Arc::URL u_it2(it2->lfn);
          if (u_it == u_it2) {
            // error if pfns are different
            if (it->pfn != it2->pfn) {
              logger.msg(Arc::ERROR, "%s: Cannot upload two different files %s and %s to same LFN: %s", jobid, it->pfn, it2->pfn, it->lfn);
              lock.lock();
              finished_jobs[jobid] = std::string("Cannot upload two different files to same LFN: " + it->lfn);
              lock.unlock();
              if (kicker_func) (*kicker_func)(kicker_arg);
              return false;
            }
            replication = true;
            done = true;
            break;
          }
        }
      }
      if (done)
        break;
    }
    // pre-clean session dir before uploading
    if (delete_all_files(job.SessionDir(), files, true) == 2) {
      logger.msg(Arc::ERROR, "%s: Failed to clean up session dir", jobid);
      lock.lock();
      finished_jobs[jobid] = std::string("Failed to clean up session dir before uploading outputs");
      lock.unlock();
      if (kicker_func) (*kicker_func)(kicker_arg);
      return false;
    }
  }
  else {
    // bad state
    logger.msg(Arc::ERROR, "%s: Received job in a bad state: %s", jobid, job.get_state_name());
    lock.lock();
    finished_jobs[jobid] = std::string("Logic error: DTR Generator received job in a bad state");
    lock.unlock();
    if (kicker_func) (*kicker_func)(kicker_arg);
    return false;
  }
  Arc::initializeCredentialsType cred_type(Arc::initializeCredentialsType::SkipCredentials);
  Arc::UserConfig usercfg(cred_type);
  usercfg.ProxyPath(job_proxy_filename(jobid, *jobuser));
  usercfg.UtilsDirPath(jobuser->ControlDir());
  usercfg.CACertificatesDirectory(env.cert_dir_loc());
  // TODO: chelonia bartenders

  // create job.id.errors file with correct permissions to add to Logger
  job_errors_mark_put(job, *jobuser);

  if (files.empty()) {
    // nothing to do so wake up GM thread and return
    if (kicker_func) (*kicker_func)(kicker_arg);
    return true;
  }

  for (std::list<FileData>::iterator i = files.begin(); i != files.end(); ++i) {
    if (i->lfn.find(":") == std::string::npos)
      continue; // user down/uploadable file

    std::string source;
    std::string destination;
    if (job.get_state() == JOB_STATE_PREPARING) {
      source = i->lfn;
      destination = "file://" + job.SessionDir() + i->pfn;
    }
    else {
      source = "file://" + job.SessionDir() + i->pfn;
      destination = i->lfn;
    }
    // logger for these DTRs. Logger and LogDestination should be deleted when DTR is received back
    Arc::Logger * dtr_log = new Arc::Logger(Arc::Logger::getRootLogger(), "DataStaging.DTR");
    Arc::LogFile * dest = new Arc::LogFile(job_errors_filename(jobid, *jobuser));
    dest->setReopen(true);
    dtr_log->addDestination(*dest);

    // create DTR and send to Scheduler
    DataStaging::DTR dtr(source, destination, usercfg, jobid, job.get_uid(), dtr_log);
    // set retry count (tmp errors only)
    dtr.set_tries_left(jobuser->Env().jobs_cfg().MaxRetries());
    // allow the same file to be uploaded to multiple locations with same LFN
    dtr.set_force_registration(replication);
    // set sub-share for download or upload
    dtr.set_sub_share((job.get_state() == JOB_STATE_PREPARING) ? "download" : "upload");
    // set priority as given in job description
    if (job.get_local())
      dtr.set_priority(job.get_local()->priority);
    DataStaging::CacheParameters cache_parameters;
    cache_parameters.cache_dirs = jobuser->CacheParams().getCacheDirs();
    cache_parameters.remote_cache_dirs = jobuser->CacheParams().getRemoteCacheDirs();
    dtr.set_cache_parameters(cache_parameters);
    dtr.registerCallback(this,DataStaging::GENERATOR);
    dtr.registerCallback(&scheduler,DataStaging::SCHEDULER);
    // callbacks for info
    dtr.registerCallback(&info, DataStaging::SCHEDULER);
    lock.lock();
    active_dtrs.insert(std::pair<std::string, std::string>(jobid, dtr.get_id()));
    lock.unlock();

    // Disconnect from root logger while submitting to Scheduler
    const std::list<Arc::LogDestination*> log_dests = Arc::Logger::getRootLogger().getDestinations();
    Arc::Logger::getRootLogger().removeDestinations();
    dtr.push(DataStaging::SCHEDULER);
    Arc::Logger::getRootLogger().addDestinations(log_dests);

    // update .local with transfer share
    JobLocalDescription *job_desc = new JobLocalDescription;
    if (!job_local_read_file(jobid, *jobuser, *job_desc)) {
      logger.msg(Arc::ERROR, "%s: Failed reading local information", jobid);
      delete job_desc;
      continue;
    }
    job_desc->transfershare = dtr.get_transfer_share();
    if (!job_local_write_file(job, *jobuser, *job_desc)) {
      logger.msg(Arc::ERROR, "%s: Failed writing local information", jobid);
    }
    delete job_desc;
  }
  return true;
}

bool DTRGenerator::processCancelledJob(const std::string& jobid) {

  // cancel DTRs in Scheduler
  logger.msg(Arc::INFO, "%s: Cancelling active DTRs", jobid);
  scheduler.cancelDTRs(jobid);
  return true;
}

int DTRGenerator::checkUploadedFiles(JobDescription& job) {

  JobId jobid(job.get_id());

  // JobUser object
  std::map<uid_t, const JobUser*>::iterator it = jobusers.find(job.get_uid());
  if (it == jobusers.end()) {
    it = jobusers.find(0);
    if (it == jobusers.end()) {
      job.AddFailure("Internal configuration error in data staging");
      logger.msg(Arc::ERROR, "%s: No configured user found for uid %i", jobid, job.get_uid());
      return 1;
    }
  }
  const JobUser* jobuser(it->second);

  std::string session_dir(jobuser->SessionRoot(jobid) + '/' + jobid);
  // get input files list
  std::list<FileData> input_files;
  std::list<FileData> input_files_ = input_files;
  if (!job_input_read_file(jobid, *jobuser, input_files)) {
    job.AddFailure("Error reading list of input files");
    logger.msg(Arc::ERROR, "%s: Can't read list of input files", jobid);
    return 1;
  }
  int res = 0;

  // loop through each file and check
  for (FileData::iterator i = input_files.begin(); i != input_files.end();) {
    // all remote files should have been downloaded by this point
    if (i->lfn.find(":") != std::string::npos) {
      ++i;
      continue;
    }
    logger.msg(Arc::VERBOSE, "%s: Checking user uploadable file: %s", jobid, i->pfn);
    std::string error;
    int err = user_file_exists(*i, session_dir, error);

    if (err == 0) { // file is uploaded
      logger.msg(Arc::VERBOSE, "%s: User has uploaded file %s", jobid, i->pfn);
      // remove from input list
      i = input_files.erase(i);
      input_files_.clear();
      for (FileData::iterator it = input_files.begin(); it != input_files.end(); ++it)
        input_files_.push_back(*it);
      if (!job_input_write_file(job, *jobuser, input_files_)) {
        logger.msg(Arc::WARNING, "%s: Failed writing changed input file.", jobid);
      }
    }
    else if (err == 1) { // critical failure
      logger.msg(Arc::ERROR, "%s: Critical error for uploadable file %s", jobid, i->pfn);
      job.AddFailure("User file: "+i->pfn+" - "+error);
      res = 1;
      break;
    }
    else { // still waiting
      res = 2;
      ++i;
    }
  }
  // check for timeout
  if (res == 2 && ((time(NULL) - job.GetStartTime()) > 600)) { // hard-coded timeout
    for (FileData::iterator i = input_files.begin(); i != input_files.end(); ++i) {
      if (i->lfn.find(":") == std::string::npos) {
        job.AddFailure("User file: "+i->pfn+" - Timeout waiting");
      }
    }
    logger.msg(Arc::ERROR, "%s: Uploadable files timed out", jobid);
    res = 1;
  }
  // clean unfinished files here
  delete_all_files(session_dir, input_files, false, true, false);
  return res;
}

int DTRGenerator::user_file_exists(FileData &dt,
                                   const std::string& session_dir,
                                   std::string& error) {
  struct stat st;
  std::string file_info(dt.lfn);
  if (file_info == "*.*") return 0; // do not wait for this file

  std::string fname = session_dir + '/' + dt.pfn;
  // check if file exists at all
  if (lstat(fname.c_str(), &st) != 0) return 2;

  // if no size/checksum was supplied, return success
  if (file_info.empty()) return 0;

  if (S_ISDIR(st.st_mode)) {
    error = "Expected file. Directory found.";
    return 1;
  }
  if (!S_ISREG(st.st_mode)) {
    error = "Expected ordinary file. Special object found.";
    return 1;
  }

  long long int fsize;
  long long int fsum;
  bool have_size = false;
  bool have_checksum = false;

  // parse format [size][.checksum]
  if (file_info[0] == '.') { // checksum only
    if (!Arc::stringto(file_info.substr(1), fsum)) {
      logger.msg(Arc::ERROR, "Can't convert checksum %s to int for %s", file_info.substr(1), dt.pfn);
      error = "Invalid checksum information";
      return 1;
    }
    have_checksum = true;
  } else if (file_info.find('.') == std::string::npos) { // size only
    if (!Arc::stringto(file_info, fsize)) {
      logger.msg(Arc::ERROR, "Can't convert filesize %s to int for %s", file_info, dt.pfn);
      error = "Invalid file size information";
      return 1;
    }
    have_size = true;
  } else { // size and checksum
    std::vector<std::string> file_attrs;
    Arc::tokenize(dt.lfn, file_attrs, ".");
    if (file_attrs.size() != 2) {
      logger.msg(Arc::ERROR, "Invalid size/checksum information (%s) for %s", file_info, dt.pfn);
      error = "Invalid size/checksum information";
      return 1;
    }
    if (!Arc::stringto(file_attrs[0], fsize)) {
      logger.msg(Arc::ERROR, "Can't convert filesize %s to int for %s", file_attrs[0], dt.pfn);
      error = "Invalid file size information";
      return 1;
    }
    if (!Arc::stringto(file_attrs[1], fsum)) {
      logger.msg(Arc::ERROR, "Can't convert checksum %s to int for %s", file_attrs[1], dt.pfn);
      error = "Invalid checksum information";
      return 1;
    }
    have_size = true;
    have_checksum = true;
  }

  // now check if proper size
  if (have_size) {
    if (st.st_size < fsize) return 2;
    if (st.st_size > fsize) {
      logger.msg(Arc::ERROR, "Invalid file: %s is too big.", dt.pfn);
      error = "Delivered file is bigger than specified.";
      return 1;
    }
  }

  if (have_checksum) { // calculate checksum
    int h = ::open(fname.c_str(), O_RDONLY);
    if (h == -1) { // if we can't read that file job won't too
      logger.msg(Arc::ERROR, "Error accessing file %s", dt.pfn);
      error = "Delivered file is unreadable.";
      return 1;
    }
    Arc::CRC32Sum crc;
    char buffer[1024];
    ssize_t l;
    for(;;) {
      if((l=read(h,buffer,1024)) == -1) {
        logger.msg(Arc::ERROR, "Error reading file %s", dt.pfn);
        error = "Could not read file to compute checksum.";
        return 1;
      }
      if(l==0) break;
      crc.add(buffer,l);
    }
    close(h);
    crc.end();
    if (fsum != crc.crc()) {
      if (have_size) { // size was checked and is ok
        logger.msg(Arc::ERROR, "File %s has wrong CRC.", dt.pfn);
        error = "Delivered file has wrong checksum.";
        return 1;
      }
      return 2; // not uploaded yet
    }
  }
  return 0; // all checks passed - file is ok
}

