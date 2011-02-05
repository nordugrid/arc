#include <sys/stat.h>
#include <fcntl.h>

#include <arc/FileUtils.h>

#include <arc/data-staging/Scheduler.h>

#include "../files/info_files.h"
#include "../files/delete.h"
#include "../conf/conf_map.h"

// need to import FileCache after Scheduler since define of round()
// conflicts with math.h. TODO: fix this!
#include <arc/data/FileCache.h>

#include "dtr_generator.h"

std::multimap<std::string, std::string> DTRGenerator::active_dtrs;
std::map<std::string, std::string> DTRGenerator::finished_jobs;
Arc::SimpleCondition DTRGenerator::lock;

std::list<DataStaging::DTR> DTRGenerator::dtrs_received;
std::list<JobDescription> DTRGenerator::jobs_received;
std::list<std::string> DTRGenerator::jobs_cancelled;
Arc::SimpleCondition DTRGenerator::event_lock;

Arc::SimpleCondition DTRGenerator::run_condition;
DataStaging::ProcessState DTRGenerator::generator_state = DataStaging::INITIATED;
std::map<uid_t, const JobUser*> DTRGenerator::jobusers;
Arc::Logger DTRGenerator::logger(Arc::Logger::getRootLogger(), "Generator");

DataStaging::Scheduler DTRGenerator::scheduler;
DTRGeneratorCallback DTRGenerator::receive_dtr;

void DTRGenerator::main_thread(void* arg) {

  // set up logging - to avoid logging DTR logs to the main A-REX log
  // we disconnect the root logger
  Arc::Logger::getRootLogger().setThreadContext();
  logger.addDestinations(Arc::Logger::getRootLogger().getDestinations());
  Arc::Logger::getRootLogger().removeDestinations();

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
      it_dtrs = dtrs_received.erase(it_dtrs);
    }

    // finally new jobs
    std::list<JobDescription>::iterator it_jobs = jobs_received.begin();
    while (it_jobs != jobs_received.end()) {
      event_lock.unlock();
      processReceivedJob(*it_jobs);
      event_lock.lock();
      it_jobs = jobs_received.erase(it_jobs);
    }
    event_lock.unlock();
    usleep(50000);
  }
  // stop scheduler - cancels all DTRs and waits for them to complete
  scheduler.stop();
  run_condition.signal();
}

bool DTRGenerator::start(const JobUsers& users) {
  if (generator_state == DataStaging::RUNNING || generator_state == DataStaging::TO_STOP)
    return false;
  generator_state = DataStaging::RUNNING;
  for (JobUsers::const_iterator i = users.begin(); i != users.end(); ++i) {
    jobusers[i->get_uid()] = &(*i);
  }
//  DataStaging::DTR::registerCallback(&DTRGenerator::receiveDTR);

  JobsListConfig& jcfg = users.Env().jobs_cfg();
  // Converting old configuration values to something
  // useful for new framework

  // processing limits
  int max_processing;
  int max_processing_emergency;
  int max_downloads;
  jcfg.GetMaxJobsLoad(max_processing,max_processing_emergency,max_downloads);
  if(max_processing <= 0) max_processing = 1;
  if(max_processing_emergency <= 0) max_processing_emergency = 0;
  if(max_downloads <= 0) max_downloads = 1;
  max_processing *= max_downloads;
  max_processing_emergency *= max_downloads;
  scheduler.SetSlots(max_processing*2,max_processing*2,max_processing,max_processing_emergency);

  // transfer shares
  DataStaging::TransferShares shares;
  shares.set_reference_shares(jcfg.GetLimitedShares());
  shares.set_share_type(jcfg.GetShareType());
  scheduler.SetTransferShares(shares);

  // URL mappings
  UrlMapConfig url_map(users.Env());
  scheduler.SetURLMapping(url_map);

  // preferred pattern
  scheduler.SetPreferredPattern(jcfg.GetPreferredPattern());

  scheduler.start();

  Arc::CreateThreadFunction(&main_thread, NULL);
  return true;
}

bool DTRGenerator::stop() {
  if (generator_state != DataStaging::RUNNING)
    return false;
  generator_state = DataStaging::TO_STOP;
  run_condition.wait();
  generator_state = DataStaging::STOPPED;
  return true;
}

void DTRGenerator::receiveDTR(DataStaging::DTR dtr) {

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
    logger.msg(Arc::ERROR, "DTRGenerator is not running!");
    return;
  }

  event_lock.lock();
  jobs_received.push_back(job);
  event_lock.unlock();
}

void DTRGenerator::cancelJob(const JobDescription& job) {

  if (generator_state != DataStaging::RUNNING) {
    logger.msg(Arc::ERROR, "DTRGenerator is not running!");
    return;
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

  // delete DTR Logger and LogDestinations
  const std::list<Arc::LogDestination*> log_dests = dtr.get_logger()->getDestinations();
  for (std::list<Arc::LogDestination*>::const_iterator i = log_dests.begin(); i != log_dests.end(); ++i)
    delete *i;
  delete dtr.get_logger();

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

    if (!dtr.get_source()->Local() && finished_jobs.find(jobid) == finished_jobs.end()) { // download
      // cancel other DTRs and erase from our list unless error was already reported
      logger.msg(Arc::INFO, "%s: Cancelling other DTRs", jobid);
      scheduler.cancelDTRs(jobid);

      // add error to finished jobs
      lock.lock();
      finished_jobs[jobid] = std::string("Failed in data staging: " + dtr.get_error_status().GetDesc());
      lock.unlock();
    }
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
    if (!job_output_read_file(jobid, *jobuser, files))
      logger.msg(Arc::WARNING, "%s: Failed to read list of output files, can't clean up session dir", jobid);
    else if (delete_all_files(job.SessionDir(), files, true) == 2)
      logger.msg(Arc::WARNING, "%s: Failed to clean up session dir", jobid);
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
  lock.unlock();

  logger.msg(Arc::INFO, "%s: Data staging finished", jobid);
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
    return false;
  }

  if (job.get_state() == JOB_STATE_PREPARING) {
    if (!job_input_read_file(jobid, *jobuser, files)) {
      logger.msg(Arc::ERROR, "%s: Failed to read list of input files", jobid);
      lock.lock();
      finished_jobs[jobid] = std::string("Failed to read list of input files");
      lock.unlock();
      return false;
    }
    // check for duplicates (see bug 1285)
    for (std::list<FileData>::iterator i = files.begin(); i != files.end(); i++) {
      for (std::list<FileData>::iterator j = files.begin(); j != files.end(); j++) {
        if (i != j && j->pfn == i->pfn) {
          logger.msg(Arc::ERROR, "Duplicate file in list of input files: %s", i->pfn);
          lock.lock();
          finished_jobs[jobid] = std::string("Duplicate file in list of input files: " + i->pfn);
          lock.unlock();
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
        logger.msg(Arc::INFO, "Reading output files from user generated list in %s", outputfilelist);
        if (!job_Xput_read_file(outputfilelist, files)) {
          logger.msg(Arc::ERROR, "Error reading user generated output file list in %s", outputfilelist);
          lock.lock();
          finished_jobs[jobid] = std::string("Error reading user generated output file list");
          lock.unlock();
          return false;
        }
      }
    }
    // remove dynamic output file lists from the files to upload
    it = files.begin();
    while (it != files.end()) {
      if (it->pfn.find("@") == 1) it = files.erase(it);
      else it++;
    }
    // check if any files share the same LFN, if so allow overwriting existing LFN
    for (it = files.begin(); it != files.end(); it++) {
      bool done = false;
      for (std::list<FileData>::iterator it2 = files.begin(); it2 != files.end(); it2++) {
        if (it != it2 && !it->lfn.empty() && !it2->lfn.empty()) {
          // error if lfns (including locations) are identical
          if (it->lfn == it2->lfn) {
            logger.msg(Arc::ERROR, "Two identical output destinations: %s", it->lfn);
            lock.lock();
            finished_jobs[jobid] = std::string("Two identical output destinations: " + it->lfn);
            lock.unlock();
            return false;
          }
          Arc::URL u_it(it->lfn);
          Arc::URL u_it2(it2->lfn);
          if (u_it == u_it2) {
            // error if pfns are different
            if (it->pfn != it2->pfn) {
              logger.msg(Arc::ERROR, "Cannot upload two different files %s and %s to same LFN: %s", it->pfn, it2->pfn, it->lfn);
              lock.lock();
              finished_jobs[jobid] = std::string("Cannot upload two different files to same LFN: " + it->lfn);
              lock.unlock();
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
      return false;
    }
  }
  else {
    // bad state
    logger.msg(Arc::ERROR, "Received job %s in a bad state: %s", jobid, job.get_state_name());
    lock.lock();
    finished_jobs[jobid] = std::string("Logic error: DTR Generator received job in a bad state");
    lock.unlock();
    return false;
  }
  Arc::UserConfig usercfg(Arc::initializeCredentialsType(Arc::initializeCredentialsType::SkipCredentials));
  usercfg.ProxyPath(job_proxy_filename(jobid, *jobuser));
  usercfg.UtilsDirPath(jobuser->ControlDir());
  usercfg.CACertificatesDirectory(env.cert_dir_loc());
  // TODO: chelonia bartenders

  // create job.id.errors file with correct permissions to add to Logger
  job_errors_mark_put(job, *jobuser);

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
    dtr.registerCallback(&receive_dtr,DataStaging::GENERATOR);
    lock.lock();
    active_dtrs.insert(std::pair<std::string, std::string>(jobid, dtr.get_id()));
    lock.unlock();
    scheduler.processDTR(dtr);
  }
  return true;
}

bool DTRGenerator::processCancelledJob(const std::string& jobid) {

  // remove DTRs and job from lists
  lock.lock();
  active_dtrs.erase(jobid);
  lock.unlock();

  // cancel DTRs in Scheduler
  logger.msg(Arc::INFO, "%s: Cancelling active DTRs", jobid);
  scheduler.cancelDTRs(jobid);
  return true;
}

/* taken straight from old downloader - could be re-written better */
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
    logger.msg(Arc::ERROR, "Can't read list of input files");
    return 1;
  }
  int res = 0;
  // read input_status files
  std::list<std::string> uploaded_files;
  std::list<std::string>* uploaded_files_ = NULL;
  if (job_input_status_read_file(jobid, *jobuser, uploaded_files))
    uploaded_files_ = &uploaded_files;
  // loop through each file and check
  for (FileData::iterator i = input_files.begin(); i != input_files.end();) {
    if (i->lfn.find(":") == std::string::npos) { // not remote file
      logger.msg(Arc::INFO, "Check user uploadable file: %s", i->pfn);
      std::string error;
      int err = user_file_exists(*i, session_dir, uploaded_files_, &error);
      if (err == 0) { /* file is uploaded */
        logger.msg(Arc::INFO, "User has uploaded file %s", i->pfn);
        // remove from input list
        i = input_files.erase(i);
        input_files_.clear();
        for (FileData::iterator it = input_files.begin(); it != input_files.end(); ++it)
          input_files_.push_back(*it);
        if (!job_input_write_file(job, *jobuser, input_files_)) {
          logger.msg(Arc::WARNING, "Failed writing changed input file.");
        }
      }
      else if (err == 1) { /* critical failure */
        logger.msg(Arc::ERROR, "Critical error for uploadable file %s", i->pfn);
        job.AddFailure("User file: "+i->pfn+" - "+error);
        res = 1;
        goto exit;
      }
      else {
        res = 2;
        ++i;
      }
    }
    else {
      ++i;
    }
  }
  // check for timeout
  if (res == 2 && ((time(NULL) - job.GetStartTime()) > 600)) { // hard-coded timeout
    for (FileData::iterator i = input_files.begin(); i != input_files.end(); ++i) {
      if (i->lfn.find(":") == std::string::npos) { /* is it lfn ? */
        job.AddFailure("User file: "+i->pfn+" - Timeout waiting");
      };
    };
    logger.msg(Arc::ERROR, "Uploadable files timed out");
    res = 1;
  };
exit:
  // clean unfinished files here
  input_files_.clear();
  for (FileData::iterator i = input_files.begin(); i != input_files.end(); ++i)
    input_files_.push_back(*i);
  delete_all_files(session_dir, input_files_, false, true, false);
  return res;
}

/*
   Check for existence of user uploadable file
   returns 0 if file exists
           1 - it is not proper file or other error
           2 - not here yet
   TODO: Copied straight from old downloader - should be cleaned up
*/
int DTRGenerator::user_file_exists(FileData &dt,const std::string& session_dir,std::list<std::string>* have_files,std::string* error) {
  struct stat st;
  const char *str = dt.lfn.c_str();
  if(strcmp(str,"*.*") == 0) return 0; /* do not wait for this file */
  std::string fname=session_dir + '/' + dt.pfn;
  /* check if file does exist at all */
  if(lstat(fname.c_str(),&st) != 0) return 2;
  /* check for misconfiguration */
  /* parse files information */
  char *str_;
  unsigned long long int fsize;
  unsigned long long int fsum = (unsigned long long int)(-1);
  bool have_size = false;
  bool have_checksum = false;
  errno = 0;
  fsize = strtoull(str,&str_,10);
  if((*str_) == '.') {
    if(str_ != str) have_size=true;
    str=str_+1;
    fsum = strtoull(str,&str_,10);
    if((*str_) != 0) {
      logger.msg(Arc::ERROR, "Invalid checksum in %s for %s", dt.lfn, dt.pfn);
      if(error) (*error)="Bad information about file: checksum can't be parsed.";
      return 1;
    };
    have_checksum=true;
  }
  else {
    if(str_ != str) have_size=true;
    if((*str_) != 0) {
      logger.msg(Arc::ERROR, "Invalid file size in %s for %s ", dt.lfn, dt.pfn);
      if(error) (*error)="Bad information about file: size can't be parsed.";
      return 1;
    };
  };
  if(S_ISDIR(st.st_mode)) {
    if(have_size || have_checksum) {
      if(error) (*error)="Expected file. Directory found.";
      return 1;
    };
    return 0;
  };
  if(!S_ISREG(st.st_mode)) {
    if(error) (*error)="Expected ordinary file. Special object found.";
    return 1;
  };
  /* now check if proper size */
  if(have_size) {
    if(st.st_size < fsize) return 2;
    if(st.st_size > fsize) {
      logger.msg(Arc::ERROR, "Invalid file: %s is too big.", dt.pfn);
      if(error) (*error)="Delivered file is bigger than specified.";
      return 1; /* too big file */
    };
  };
  if(have_files) {
    std::list<std::string>::iterator f = have_files->begin();
    for(;f!=have_files->end();++f) {
      if(dt.pfn == *f) break;
    };
    if(f == have_files->end()) return 2;
  } else if(have_checksum) {
    int h=Arc::FileOpen(fname,O_RDONLY);
    if(h==-1) { /* if we can't read that file job won't too */
      logger.msg(Arc::ERROR, "Error accessing file %s", dt.pfn);
      if(error) (*error)="Delivered file is unreadable.";
      return 1;
    };
    Arc::CRC32Sum crc;
    char buffer[1024];
    ssize_t l;
    size_t ll = 0;
    for(;;) {
      if((l=read(h,buffer,1024)) == -1) {
        logger.msg(Arc::ERROR, "Error reading file %s", dt.pfn);
        if(error) (*error)="Could not read file to compute checksum.";
        return 1;
      };
      if(l==0) break; ll+=l;
      crc.add(buffer,l);
    };
    close(h);
    crc.end();
    if(fsum != crc.crc()) {
      if(have_size) { /* size was checked - it is an error to have wrong crc */
        logger.msg(Arc::ERROR, "File %s has wrong CRC.", dt.pfn);
        if(error) (*error)="Delivered file has wrong checksum.";
        return 1;
      };
      return 2; /* just not uploaded yet */
    };
  };
  return 0; /* all checks passed - file is ok */
}

void DTRGeneratorCallback::receive_dtr(DataStaging::DTR dtr) {
  DTRGenerator::receiveDTR(dtr);
}

