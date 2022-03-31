#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Thread.h>
#include <arc/StringConv.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataStatus.h>
#include <arc/data/FileCache.h>
#include <arc/credential/Credential.h>

#include "DTRStatus.h"
#include "Processor.h"

namespace DataStaging {

  std::string Processor::hostname;

  /** Set up logging. Should be called at the start of each thread method. */
  void setUpLogger(DTR_ptr request) {
    // Move DTR destinations from DTR logger to Root logger to catch all messages.
    // disconnect this thread's root logger
    Arc::Logger::getRootLogger().setThreadContext();
    request->get_logger()->setThreadContext();
    Arc::Logger::getRootLogger().removeDestinations();
    Arc::Logger::getRootLogger().addDestinations(request->get_logger()->getDestinations());
    request->get_logger()->removeDestinations();
  }

  Processor::Processor() {
    // Get hostname, needed to exclude ACIX replicas on localhost
    char hostn[256];
    if (gethostname(hostn, sizeof(hostn)) == 0){
      hostname = hostn;
    }
  }

  /* Thread methods for each state of the DTR */

  void Processor::DTRCheckCache(void* arg) {
    ThreadArgument* targ = (ThreadArgument*)arg;
    DTR_ptr request = targ->dtr;
    delete targ;

    setUpLogger(request);

    // IMPORTANT: This method creates a lock on the cached file for
    // this DTR. It must be released at some point using ProcessCache

    // Create cache using configuration
    Arc::FileCache cache(request->get_cache_parameters().cache_dirs,
                         request->get_cache_parameters().drain_cache_dirs,
                         request->get_cache_parameters().readonly_cache_dirs,
                         request->get_parent_job_id(),
                         request->get_local_user().get_uid(),
                         request->get_local_user().get_gid());

    if (!cache) {
      request->get_logger()->msg(Arc::ERROR, "Error creating cache");
      request->set_cache_state(CACHE_SKIP);
      request->set_error_status(DTRErrorStatus::CACHE_ERROR,
                            DTRErrorStatus::ERROR_DESTINATION,
                            "Failed to create cache");
      request->set_status(DTRStatus::CACHE_CHECKED);
      DTR::push(request, SCHEDULER);
      return;
    }
    // DN is used for checking cache permissions
    std::string dn = request->get_credential_info().getDN();
    Arc::Time exp_time = request->get_credential_info().getExpiryTime();

    std::string canonic_url(request->get_source()->GetURL().plainstr());
    std::string cacheoption(request->get_source()->GetURL().Option("cache"));
    // add guid if present
    // TODO handle guids better in URL class so we don't need to care here
    if (!request->get_source()->GetURL().MetaDataOption("guid").empty())
      canonic_url += ":guid=" + request->get_source()->GetURL().MetaDataOption("guid");

    bool is_in_cache = false;
    bool is_locked = false;
    // check for forced re-download option
    bool renew = (cacheoption == "renew");
    if (renew) request->get_logger()->msg(Arc::VERBOSE, "Forcing re-download of file %s", canonic_url);

    for (;;) {
      if (!cache.Start(canonic_url, is_in_cache, is_locked, renew)) {
        if (is_locked) {
          request->get_logger()->msg(Arc::WARNING, "Cached file is locked - should retry");
          request->set_cache_state(CACHE_LOCKED);
          request->set_status(DTRStatus::CACHE_WAIT);

          // set a flat wait time with some randomness, fine-grained to minimise lock clashes
          // this may change in future eg be taken from configuration or increase over time
          time_t cache_wait_time = 10;
          time_t randomness = (rand() % cache_wait_time) - (cache_wait_time/2);
          cache_wait_time += randomness;
          // add random number of milliseconds
          uint32_t nano_randomness = (rand() % 1000) * 1000000;
          Arc::Period cache_wait_period(cache_wait_time, nano_randomness);
          request->get_logger()->msg(Arc::INFO, "Will wait around %is", cache_wait_time);
          request->set_process_time(cache_wait_period);

          DTR::push(request, SCHEDULER);
          return;
        }
        request->get_logger()->msg(Arc::ERROR, "Failed to initiate cache");
        request->set_cache_state(CACHE_SKIP);
        request->set_error_status(DTRErrorStatus::CACHE_ERROR,
                              DTRErrorStatus::ERROR_DESTINATION,
                              "Failed to initiate cache");
        break;
      }
      request->set_cache_file(cache.File(canonic_url));
      if (is_in_cache) {
        // Whether cache file is outdated
        bool outdated = (cacheoption != "invariant");
        // Check source if requested
        if (cacheoption == "check") {
          request->get_logger()->msg(Arc::INFO, "Force-checking source of cache file %s", cache.File(canonic_url));
          Arc::DataStatus cres = request->get_source()->Check(true);
          if (!cres.Passed()) {
            request->get_logger()->msg(Arc::ERROR, "Source check requested but failed: %s", std::string(cres));
            // Try again skipping cache, maybe this is not worth it
            request->set_cache_state(CACHE_SKIP);
            request->set_error_status(DTRErrorStatus::CACHE_ERROR,
                                      DTRErrorStatus::ERROR_DESTINATION,
                                      "Failed to check source for " + canonic_url + ": " + std::string(cres));
            break;
          }
        }
        else {
          // just need to check permissions
          request->get_logger()->msg(Arc::INFO, "File %s is cached (%s) - checking permissions",
                                     canonic_url, cache.File(canonic_url));
          // check the list of cached DNs
          if (cache.CheckDN(canonic_url, dn)) {
            outdated = false; // If DN is cached then don't check creation date
          }
          else {
            Arc::DataStatus cres = request->get_source()->Check(cacheoption != "invariant");
            if (!cres.Passed()) {
              request->get_logger()->msg(Arc::ERROR, "Permission checking failed, will try downloading without using cache");
              request->set_cache_state(CACHE_SKIP);
              request->set_error_status(DTRErrorStatus::CACHE_ERROR,
                                        DTRErrorStatus::ERROR_DESTINATION,
                                        "Failed to check cache permissions for " + canonic_url + ": " + std::string(cres));
              break;
            }
            cache.AddDN(canonic_url, dn, exp_time);
          }
        }
        request->get_logger()->msg(Arc::INFO, "Permission checking passed");
        // check if file is fresh enough
        if (request->get_source()->CheckModified() && cache.CheckCreated(canonic_url)) {
          Arc::Time sourcetime = request->get_source()->GetModified();
          Arc::Time cachetime = cache.GetCreated(canonic_url);
          request->get_logger()->msg(Arc::VERBOSE, "Source modification date: %s", sourcetime.str());
          request->get_logger()->msg(Arc::VERBOSE, "Cache creation date: %s", cachetime.str());
          if (sourcetime <= cachetime)
            outdated = false;
        }
        if (outdated) {
          request->get_logger()->msg(Arc::INFO, "Cached file is outdated, will re-download");
          renew = true;
          continue;
        }
        // cached file is present and valid
        request->get_logger()->msg(Arc::VERBOSE, "Cached copy is still valid");
        request->set_cache_state(CACHE_ALREADY_PRESENT);
      }
      else { // file is not there but we are ready to download it
        request->get_logger()->msg(Arc::VERBOSE, "Will download to cache file %s", request->get_cache_file());
        request->set_cache_state(CACHEABLE);
      }
      break;
    }
    request->set_status(DTRStatus::CACHE_CHECKED);
    DTR::push(request, SCHEDULER);
  }

  void Processor::DTRResolve(void* arg) {
    // call request->source.Resolve() to get replicas
    // call request->destination.Resolve() to check supplied replicas
    // call request->destination.PreRegister() to lock destination LFN
    ThreadArgument* targ = (ThreadArgument*)arg;
    DTR_ptr request = targ->dtr;
    delete targ;

    setUpLogger(request);

    // check for source replicas
    if (request->get_source()->IsIndex()) {
      request->get_logger()->msg(Arc::VERBOSE, "Looking up source replicas");
      Arc::DataStatus res = request->get_source()->Resolve(true);
      if (!res.Passed() || !request->get_source()->HaveLocations() || !request->get_source()->LocationValid()) {
        request->get_logger()->msg(Arc::ERROR, std::string(res));
        request->set_error_status(res.Retryable() ? DTRErrorStatus::TEMPORARY_REMOTE_ERROR : DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                  DTRErrorStatus::ERROR_SOURCE,
                                  "Could not resolve any source replicas for " + request->get_source()->str() + ": " + std::string(res));
        request->set_status(DTRStatus::RESOLVED);
        DTR::push(request, SCHEDULER);
        return;
      }
    }
    // If using ACIX, remove sources on our own host
    if (request->get_use_acix()) {
      int tries = request->get_source()->GetTries();
      while (request->get_source()->LocationValid()) {
        if (request->get_source()->CurrentLocation().Host() == Processor::hostname) {
          request->get_logger()->msg(Arc::INFO, "Skipping replica on local host %s", request->get_source()->CurrentLocation().str());
          request->get_source()->RemoveLocation();
        } else {
          request->get_source()->NextLocation();
        }
      }
      // Check that there are still replicas to use
      if (!request->get_source()->HaveLocations()) {
        request->get_logger()->msg(Arc::ERROR, "No locations left for %s", request->get_source()->str());
        request->set_error_status(DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                  DTRErrorStatus::ERROR_SOURCE,
                                  "Could not resolve any source replicas for " + request->get_source()->str());
        request->set_status(DTRStatus::RESOLVED);
        DTR::push(request, SCHEDULER);
        return;
      }
      // reset retries
      request->get_source()->SetTries(tries);
    }
    // If overwrite is requested, the resolving and pre-registering of the
    // destination will be done in the pre-clean stage after deleting.
    if (!request->is_replication() && request->get_destination()->GetURL().Option("overwrite") == "yes") {
      request->set_status(DTRStatus::RESOLVED);
      DTR::push(request, SCHEDULER);
      return;
    }

    // Check replicas supplied for destination
    if (request->get_destination()->IsIndex()) {
      request->get_logger()->msg(Arc::VERBOSE, "Resolving destination replicas");
      Arc::DataStatus res = request->get_destination()->Resolve(false);
      if (!res.Passed() || !request->get_destination()->HaveLocations() || !request->get_destination()->LocationValid()) {
        request->get_logger()->msg(Arc::ERROR, std::string(res));
        request->set_error_status(res.Retryable() ? DTRErrorStatus::TEMPORARY_REMOTE_ERROR : DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                  DTRErrorStatus::ERROR_DESTINATION,
                                  "Could not resolve any destination replicas for " + request->get_destination()->str() + ": " + std::string(res));
        request->set_status(DTRStatus::RESOLVED);
        DTR::push(request, SCHEDULER);
        return;
      }
    }
    // check that replication is possible
    if (request->is_replication()) {
      // we do not want to replicate to same physical file
      request->get_destination()->RemoveLocations(*(request->get_source()));
      if (!request->get_destination()->HaveLocations()) {
        request->get_logger()->msg(Arc::ERROR, "No locations for destination different from source found");
        request->set_error_status(DTRErrorStatus::SELF_REPLICATION_ERROR,
                                  DTRErrorStatus::NO_ERROR_LOCATION,
                                  "No locations for destination different from source found for " + request->get_destination()->str());
        request->set_status(DTRStatus::RESOLVED);
        DTR::push(request, SCHEDULER);
        return;
      }
    }
    // pre-register destination
    if (request->get_destination()->IsIndex()) {
      request->get_logger()->msg(Arc::VERBOSE, "Pre-registering destination in index service");
      Arc::DataStatus res = request->get_destination()->PreRegister(request->is_replication(), request->is_force_registration());
      if (!res.Passed()) {
        request->get_logger()->msg(Arc::ERROR, std::string(res));
        request->set_error_status(res.Retryable() ? DTRErrorStatus::TEMPORARY_REMOTE_ERROR : DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                  DTRErrorStatus::ERROR_DESTINATION,
                                  "Could not pre-register destination " + request->get_destination()->str() + ": " + std::string(res));
      }
    }
    // finished with resolving - send back to scheduler
    request->set_status(DTRStatus::RESOLVED);
    DTR::push(request, SCHEDULER);
  }

  void Processor::DTRBulkResolve(void* arg) {
    // call request->source.BulkResolve() to get replicas
    // NOTE only source resolution can be done in bulk
    BulkThreadArgument* targ = (BulkThreadArgument*)arg;
    std::list<DTR_ptr> requests = targ->dtrs;
    delete targ;

    if (requests.empty()) return;

    std::list<Arc::DataPoint*> sources;
    for (std::list<DTR_ptr>::iterator i = requests.begin(); i != requests.end(); ++i) {
      setUpLogger(*i);
      (*i)->get_logger()->msg(Arc::VERBOSE, "Resolving source replicas in bulk");
      sources.push_back(&(*((*i)->get_source()))); // nasty...
    }

    // check for source replicas
    Arc::DataStatus res = requests.front()->get_source()->Resolve(true, sources);
    for (std::list<DTR_ptr>::iterator i = requests.begin(); i != requests.end(); ++i) {
      DTR_ptr request = *i;
      if (!res.Passed()) {
        request->get_logger()->msg(Arc::ERROR, std::string(res));
        request->set_error_status(res.Retryable() ? DTRErrorStatus::TEMPORARY_REMOTE_ERROR : DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                  DTRErrorStatus::ERROR_SOURCE,
                                  "Could not resolve any source replicas for " + request->get_source()->str() + ": " + std::string(res));
      } else if (!request->get_source()->HaveLocations() || !request->get_source()->LocationValid()) {
        request->get_logger()->msg(Arc::ERROR, "No replicas found for %s", request->get_source()->str());
        request->set_error_status(DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                  DTRErrorStatus::ERROR_SOURCE,
                                  "No replicas found for " + request->get_source()->str());
      }
      // If using ACIX, remove sources on our own host
      if (request->get_use_acix()) {
        int tries = request->get_source()->GetTries();
        while (request->get_source()->LocationValid()) {
          if (request->get_source()->CurrentLocation().Host() == Processor::hostname) {
            request->get_logger()->msg(Arc::INFO, "Skipping replica on local host %s", request->get_source()->CurrentLocation().str());
            request->get_source()->RemoveLocation();
          } else {
            request->get_source()->NextLocation();
          }
        }
        // Check that there are still replicas to use
        if (!request->get_source()->HaveLocations()) {
          request->get_logger()->msg(Arc::ERROR, "No locations left for %s", request->get_source()->str());
          request->set_error_status(DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                    DTRErrorStatus::ERROR_SOURCE,
                                    "Could not resolve any source replicas for " + request->get_source()->str());
        }
        // reset retries
        request->get_source()->SetTries(tries);
      }

      request->set_status(DTRStatus::RESOLVED);
      DTR::push(request, SCHEDULER);
    }
  }

  void Processor::DTRQueryReplica(void* arg) {
    // check source is ok and obtain metadata
    ThreadArgument* targ = (ThreadArgument*)arg;
    DTR_ptr request = targ->dtr;
    delete targ;

    setUpLogger(request);

    Arc::DataStatus res;
    request->get_logger()->msg(Arc::INFO, "Checking %s", request->get_source()->CurrentLocation().str());
    if (request->get_source()->IsIndex()) {
      res = request->get_source()->CompareLocationMetadata();
    } else {
      Arc::FileInfo file;
      res = request->get_source()->Stat(file, Arc::DataPoint::INFO_TYPE_CONTENT);
    }

    if (res == Arc::DataStatus::InconsistentMetadataError) {
      request->get_logger()->msg(Arc::ERROR, "Metadata of replica and index service differ");
      request->set_error_status(DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                DTRErrorStatus::ERROR_SOURCE,
                                "Metadata of replica and index service differ for " +
                                  request->get_source()->CurrentLocation().str() +
                                  " and " + request->get_source()->str());
    }
    else if (!res.Passed()) {
      request->get_logger()->msg(Arc::ERROR, "Failed checking source replica %s: %s",
                                 request->get_source()->CurrentLocation().str(),
                                 std::string(res) );
      request->set_error_status(res.Retryable() ? DTRErrorStatus::TEMPORARY_REMOTE_ERROR : DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                DTRErrorStatus::ERROR_SOURCE,
                                "Failed checking source replica " + request->get_source()->CurrentLocation().str() + ": " + std::string(res));
    }
    else {
      // assign metadata to destination
      request->get_destination()->SetMeta(*request->get_source());
    }
    // finished querying - send back to scheduler
    request->set_status(DTRStatus::REPLICA_QUERIED);
    DTR::push(request, SCHEDULER);
  }

  void Processor::DTRBulkQueryReplica(void* arg) {
    BulkThreadArgument* targ = (BulkThreadArgument*)arg;
    std::list<DTR_ptr> requests = targ->dtrs;
    delete targ;

    if (requests.empty()) return;

    std::list<Arc::DataPoint*> sources;
    for (std::list<DTR_ptr>::iterator i = requests.begin(); i != requests.end(); ++i) {
      setUpLogger(*i);
      (*i)->get_logger()->msg(Arc::VERBOSE, "Querying source replicas in bulk");
      sources.push_back((*i)->get_source()->CurrentLocationHandle());
    }

    // Query source
    std::list<Arc::FileInfo> files;
    Arc::DataStatus res = sources.front()->Stat(files, sources, Arc::DataPoint::INFO_TYPE_CONTENT);

    std::list<Arc::FileInfo>::const_iterator file = files.begin();
    for (std::list<DTR_ptr>::iterator i = requests.begin(); i != requests.end(); ++i, ++file) {
      DTR_ptr request = *i;
      if (!res.Passed() || files.size() != requests.size()) {
        request->get_logger()->msg(Arc::ERROR, "Failed checking source replica: %s", std::string(res));
        request->set_error_status(res.Retryable() ? DTRErrorStatus::TEMPORARY_REMOTE_ERROR : DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                  DTRErrorStatus::ERROR_SOURCE,
                                  "Failed checking source replica " + request->get_source()->CurrentLocation().str() + ": " + std::string(res));
      }
      else if (!*file) {
        request->get_logger()->msg(Arc::ERROR, "Failed checking source replica");
        request->set_error_status(DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                  DTRErrorStatus::ERROR_SOURCE,
                                  "Failed checking source replica " + request->get_source()->CurrentLocation().str());
      }
      else if (request->get_source()->IsIndex() && !request->get_source()->CompareMeta(*(request->get_source()->CurrentLocationHandle()))) {
        request->get_logger()->msg(Arc::ERROR, "Metadata of replica and index service differ");
        request->set_error_status(DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                  DTRErrorStatus::ERROR_SOURCE,
                                  "Metadata of replica and index service differ for " +
                                    request->get_source()->CurrentLocation().str() +
                                    " and " + request->get_source()->str());
      }
      else {
        // assign metadata to destination
        request->get_destination()->SetMeta(*request->get_source());
      }
      request->set_status(DTRStatus::REPLICA_QUERIED);
      DTR::push(request, SCHEDULER);
    }
  }

  void Processor::DTRPreClean(void *arg) {
    // If overwrite is requested, for physical files call Remove()
    // for index services delete entry and all existing replicas
    // only if the entry already exists. Otherwise check if a remote
    // destination exists and fail if it does
    ThreadArgument* targ = (ThreadArgument*)arg;
    DTR_ptr request = targ->dtr;
    delete targ;
    setUpLogger(request);

    Arc::DataStatus res = Arc::DataStatus::Success;

    if (!request->is_replication() &&
        (request->get_destination()->GetURL().Option("overwrite") == "yes" ||
         request->get_destination()->CurrentLocation().Option("overwrite") == "yes")) {
      request->get_logger()->msg(Arc::VERBOSE, "Overwrite requested - will pre-clean destination");

      if (!request->get_destination()->IsIndex()) {
        request->get_logger()->msg(Arc::INFO, "Removing %s", request->get_destination()->CurrentLocation().str());
        res = request->get_destination()->Remove();
      }
      else {
        // get existing locations
        Arc::DataHandle dest(request->get_destination()->GetURL(), request->get_destination()->GetUserConfig());
        request->get_logger()->msg(Arc::VERBOSE, "Finding existing destination replicas");
        res = dest->Resolve(true);
        if (!res.Passed()) {
          request->get_logger()->msg(Arc::ERROR, std::string(res));
        }
        else {
          if (dest->HaveLocations()) {
            while (dest->LocationValid()) {
              request->get_logger()->msg(Arc::INFO, "Removing %s", dest->CurrentLocation().str());
              res = dest->Remove();
              if (!res.Passed()) {
                // if we fail to delete one replica then bail out
                request->get_logger()->msg(Arc::ERROR, "Failed to delete replica %s: %s",
                                          dest->CurrentLocation().str(),
                                          std::string(res));
                break;
              }
              // unregister this replica from the index
              // not critical if this fails as will be removed in the next step
              dest->Unregister(false);
              // next replica
              dest->RemoveLocation();
            }
          }
          if (!dest->HaveLocations()) {
            // all replicas were deleted successfully, now unregister the LFN
            request->get_logger()->msg(Arc::INFO, "Unregistering %s", dest->str());
            res = dest->Unregister(true);
          }
        }
        // if deletion was successful resolve destination and pre-register
        if (!dest->HaveLocations()) {
          request->get_logger()->msg(Arc::VERBOSE, "Resolving destination replicas");
          res = request->get_destination()->Resolve(false);
          if (!res.Passed()) {
            request->get_logger()->msg(Arc::ERROR, std::string(res));
          } else {
            request->get_logger()->msg(Arc::VERBOSE, "Pre-registering destination");
            res = request->get_destination()->PreRegister(false, request->is_force_registration());
          }
        }
      }
      if (!res.Passed()) {
        request->get_logger()->msg(Arc::ERROR, "Failed to pre-clean destination: %s", std::string(res));
        request->set_error_status(DTRErrorStatus::TEMPORARY_REMOTE_ERROR,
                                  DTRErrorStatus::ERROR_DESTINATION,
                                  "Failed to pre-clean destination " + request->get_destination()->str() + ": " + std::string(res));
      }
    } else if (!request->get_destination()->Local() && !request->get_destination()->IsIndex()) {
      Arc::FileInfo file;
      res = request->get_destination()->Stat(file, Arc::DataPoint::INFO_TYPE_MINIMAL);
      if (res.Passed()) {
        request->get_logger()->msg(Arc::ERROR, "Destination already exists");
        request->set_error_status(DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                  DTRErrorStatus::ERROR_DESTINATION,
                                  "Destination " + request->get_destination()->str() + " already exists");
      } // We should check the error was no such file but just report all errors as ok
    }
    request->set_status(DTRStatus::PRE_CLEANED);
    DTR::push(request, SCHEDULER);
  }

  void Processor::DTRStagePrepare(void* arg) {
    // Only valid for stageable (SRM-like) protocols.
    // Call request->source.PrepareReading() to get TURL for reading or query status of request
    // and/or request->destination.PrepareWriting() to get TURL for writing or query status of request
    ThreadArgument* targ = (ThreadArgument*)arg;
    DTR_ptr request = targ->dtr;
    delete targ;

    setUpLogger(request);

    // first source - if stageable and not already staged yet
    if (request->get_source()->IsStageable() && request->get_source()->TransferLocations().empty()) {
      // give default wait time for cases where no wait time is given by the remote service
      unsigned int source_wait_time = 10;
      request->get_logger()->msg(Arc::VERBOSE, "Preparing to stage source");
      Arc::DataStatus res = request->get_source()->PrepareReading(0, source_wait_time);
      if (!res.Passed()) {
        request->get_logger()->msg(Arc::ERROR, std::string(res));
        request->set_error_status(res.Retryable() ? DTRErrorStatus::TEMPORARY_REMOTE_ERROR : DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                  DTRErrorStatus::ERROR_SOURCE,
                                  "Failed to prepare source " + request->get_source()->CurrentLocation().str() + ": " + std::string(res));
      }
      else if (res == Arc::DataStatus::ReadPrepareWait) {
        // if timeout then don't wait - scheduler will deal with it immediately
        if (Arc::Time() < request->get_timeout()) {
          if (source_wait_time > 60) source_wait_time = 60;
          request->set_process_time(source_wait_time);
          request->get_logger()->msg(Arc::VERBOSE, "Source is not ready, will wait %u seconds", source_wait_time);
        }
        request->set_status(DTRStatus::STAGING_PREPARING_WAIT);
      }
      else {
        if (request->get_source()->TransferLocations().empty()) {
          request->get_logger()->msg(Arc::ERROR, "No physical files found for source");
          request->set_error_status(DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                    DTRErrorStatus::ERROR_SOURCE,
                                    "No physical files found for source " + request->get_source()->CurrentLocation().str());
        } else {
          // TODO order physical files according to eg preferred pattern
        }
      }
    }
    if (request->error()) {
      request->set_status(DTRStatus::STAGED_PREPARED);
      DTR::push(request, SCHEDULER);
      return;
    }
    // now destination - if stageable and not already staged yet
    if (request->get_destination()->IsStageable() && request->get_destination()->TransferLocations().empty()) {
      // give default wait time for cases where no wait time is given by the remote service
      unsigned int dest_wait_time = 10;
      request->get_logger()->msg(Arc::VERBOSE, "Preparing to stage destination");
      Arc::DataStatus res = request->get_destination()->PrepareWriting(0, dest_wait_time);
      if (!res.Passed()) {
        request->get_logger()->msg(Arc::ERROR, std::string(res));
        request->set_error_status(res.Retryable() ? DTRErrorStatus::TEMPORARY_REMOTE_ERROR : DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                  DTRErrorStatus::ERROR_DESTINATION,
                                  "Failed to prepare destination " + request->get_destination()->CurrentLocation().str() + ": " + std::string(res));
      }
      else if (res == Arc::DataStatus::WritePrepareWait) {
        // if timeout then don't wait - scheduler will deal with it immediately
        if (Arc::Time() < request->get_timeout()) {
          if (dest_wait_time > 60) dest_wait_time = 60;
          request->set_process_time(dest_wait_time);
          request->get_logger()->msg(Arc::VERBOSE, "Destination is not ready, will wait %u seconds", dest_wait_time);
        }
        request->set_status(DTRStatus::STAGING_PREPARING_WAIT);
      }
      else {
        if (request->get_destination()->TransferLocations().empty()) {
          request->get_logger()->msg(Arc::ERROR, "No physical files found for destination");
          request->set_error_status(DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                    DTRErrorStatus::ERROR_DESTINATION,
                                    "No physical files found for destination " + request->get_destination()->CurrentLocation().str());
        } else {
          // TODO choose best physical file
        }
      }
    }
    // set to staged prepared if we don't have to wait for source or destination
    if (request->get_status() != DTRStatus::STAGING_PREPARING_WAIT)
      request->set_status(DTRStatus::STAGED_PREPARED);
    DTR::push(request, SCHEDULER);
  }

  void Processor::DTRReleaseRequest(void* arg) {
    // only valid for stageable (SRM-like) protocols. call request->source.FinishReading() and/or
    // request->destination.FinishWriting() to release or abort requests
    ThreadArgument* targ = (ThreadArgument*)arg;
    DTR_ptr request = targ->dtr;
    delete targ;
    setUpLogger(request);

    Arc::DataStatus res;

    if (request->get_source()->IsStageable()) {
      request->get_logger()->msg(Arc::VERBOSE, "Releasing source");
      res = request->get_source()->FinishReading(request->error() || request->cancel_requested());
      if (!res.Passed()) {
        // an error here is not critical to the transfer
        request->get_logger()->msg(Arc::WARNING, "There was a problem during post-transfer source handling: %s",
                                   std::string(res));
      }
    }
    if (request->get_destination()->IsStageable()) {
      request->get_logger()->msg(Arc::VERBOSE, "Releasing destination");
      res = request->get_destination()->FinishWriting(request->error() || request->cancel_requested());
      if (!res.Passed()) {
        if (request->error()) {
          request->get_logger()->msg(Arc::WARNING, "There was a problem during post-transfer destination handling after error: %s",
                                     std::string(res));
        }
        else {
          request->get_logger()->msg(Arc::ERROR, "Error with post-transfer destination handling: %s",
                                     std::string(res));
          request->set_error_status(res.Retryable() ? DTRErrorStatus::TEMPORARY_REMOTE_ERROR :
                                                      DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                    DTRErrorStatus::ERROR_DESTINATION,
                                    "Error with post-transfer destination handling of " +
                                       request->get_destination()->CurrentLocation().str() + ": " + std::string(res));
        }
      }
    }
    request->set_status(DTRStatus::REQUEST_RELEASED);
    DTR::push(request, SCHEDULER);
  }

  void Processor::DTRRegisterReplica(void* arg) {
    // call request->destination.Register() to add new replica and metadata for normal workflow
    // call request->destination.PreUnregister() to delete LFN placed during
    // RESOLVE stage for error workflow
    ThreadArgument* targ = (ThreadArgument*)arg;
    DTR_ptr request = targ->dtr;
    delete targ;
    setUpLogger(request);

    // TODO: If the copy completed before request was cancelled, unregistering
    // here will lead to dark data. Need to check for successful copy
    if (request->error() || request->cancel_requested()) {
      request->get_logger()->msg(Arc::VERBOSE, "Removing pre-registered destination in index service");
      Arc::DataStatus res = request->get_destination()->PreUnregister(request->is_replication());
      if (!res.Passed()) {
        request->get_logger()->msg(Arc::ERROR, "Failed to unregister pre-registered destination %s: %s."
                                   " You may need to unregister it manually",
                                   request->get_destination()->str(), std::string(res));
      }
    }
    else {
      request->get_logger()->msg(Arc::VERBOSE, "Registering destination replica");
      Arc::DataStatus res = request->get_destination()->PostRegister(request->is_replication());
      if (!res.Passed()) {
        request->get_logger()->msg(Arc::ERROR, "Failed to register destination replica: %s",
                                   std::string(res));
        if (!request->get_destination()->PreUnregister(request->is_replication()).Passed()) {
          request->get_logger()->msg(Arc::ERROR, "Failed to unregister pre-registered destination %s."
                                     " You may need to unregister it manually",
                                     request->get_destination()->str());
        }
        request->set_error_status(res.Retryable() ? DTRErrorStatus::TEMPORARY_REMOTE_ERROR : DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                  DTRErrorStatus::ERROR_DESTINATION,
                                  "Could not post-register destination " + request->get_destination()->str() + ": " + std::string(res));
      }
    }
    // finished with registration - send back to scheduler
    request->set_status(DTRStatus::REPLICA_REGISTERED);
    DTR::push(request, SCHEDULER);
  }

  void Processor::DTRProcessCache(void* arg) {
    // link or copy cached file to session dir, or release locks in case
    // of error or deciding not to use cache (for example because of a mapped link)
    ThreadArgument* targ = (ThreadArgument*)arg;
    DTR_ptr request = targ->dtr;
    delete targ;
    setUpLogger(request);

    Arc::FileCache cache(request->get_cache_parameters().cache_dirs,
                         request->get_cache_parameters().drain_cache_dirs,
                         request->get_cache_parameters().readonly_cache_dirs,
                         request->get_parent_job_id(),
                         request->get_local_user().get_uid(),
                         request->get_local_user().get_gid());

    if (!cache) {
      request->get_logger()->msg(Arc::ERROR, "Error creating cache. Stale locks may remain.");
      request->set_error_status(DTRErrorStatus::CACHE_ERROR,
                            DTRErrorStatus::ERROR_DESTINATION,
                            "Failed to create cache for " + request->get_source()->str());
      request->set_status(DTRStatus::CACHE_PROCESSED);
      DTR::push(request, SCHEDULER);
      return;
    }

    std::string canonic_url(request->get_source()->GetURL().plainstr());
    // add guid if present
    if (!request->get_source()->GetURL().MetaDataOption("guid").empty())
      canonic_url += ":guid=" + request->get_source()->GetURL().MetaDataOption("guid");

    // don't link if error, cancellation or cache not being used
    if (request->error() || request->cancel_requested() || request->get_cache_state() == CACHE_NOT_USED) {
      // release locks if they were acquired
      if (request->get_cache_state() == CACHEABLE || request->get_cache_state() == CACHE_NOT_USED) {
        if (request->error() || request->cancel_requested()) {
          cache.StopAndDelete(canonic_url);
        } else {
          cache.Stop(canonic_url);
        }
      }
      request->set_status(DTRStatus::CACHE_PROCESSED);
      DTR::push(request, SCHEDULER);
      return;
    }

    // check options for whether to copy or link
    bool executable = (request->get_source()->GetURL().Option("exec") == "yes") ? true : false;
    bool cache_copy = (request->get_source()->GetURL().Option("cache") == "copy") ? true : false;

    request->get_logger()->msg(Arc::INFO, "Linking/copying cached file to %s",
                               request->get_destination()->CurrentLocation().Path());

    bool was_downloaded = (request->get_cache_state() == CACHE_DOWNLOADED) ? true : false;
    if (was_downloaded) {
      // Add DN to cached permissions
      std::string dn = request->get_credential_info().getDN();
      Arc::Time exp_time = request->get_credential_info().getExpiryTime();
      cache.AddDN(canonic_url, dn, exp_time);
    }

    bool try_again = false;
    if (!cache.Link(request->get_destination()->CurrentLocation().Path(),
                    canonic_url,
                    cache_copy,
                    executable,
                    was_downloaded,
                    try_again)) {
      if (try_again) {
        // set cache status to CACHE_LOCKED, so that the Scheduler will try again
        request->set_cache_state(CACHE_LOCKED);
        request->get_logger()->msg(Arc::WARNING, "Failed linking cache file to %s",
                                   request->get_destination()->CurrentLocation().Path());
      }
      else {
        request->get_logger()->msg(Arc::ERROR, "Error linking cache file to %s.",
                                   request->get_destination()->CurrentLocation().Path());
      }
      request->set_error_status(DTRErrorStatus::CACHE_ERROR,
                                DTRErrorStatus::ERROR_DESTINATION,
                                "Failed to link/copy cache file to session dir");
    }
    if (was_downloaded) cache.Stop(canonic_url);
    request->set_status(DTRStatus::CACHE_PROCESSED);
    DTR::push(request, SCHEDULER);
  }

  /* main process method called from DTR::push() */

  void Processor::receiveDTR(DTR_ptr request) {

    BulkThreadArgument* bulk_arg = NULL;
    ThreadArgument* arg = NULL;

    // first deal with bulk
    if (request->get_bulk_end()) { // end of bulk
      request->get_logger()->msg(Arc::VERBOSE, "Adding to bulk request");
      request->set_bulk_end(false);
      bulk_list.push_back(request);
      bulk_arg = new BulkThreadArgument(this, bulk_list);
      bulk_list.clear();
    }
    else if (request->get_bulk_start() || !bulk_list.empty()) { // filling bulk list
      request->get_logger()->msg(Arc::VERBOSE, "Adding to bulk request");
      bulk_list.push_back(request);
      if (request->get_bulk_start()) request->set_bulk_start(false);
    }
    else { // non-bulk request
      arg = new ThreadArgument(this, request);
    }

    // switch through the expected DTR states
    switch (request->get_status().GetStatus()) {

      // pre-processor states

      case DTRStatus::CHECK_CACHE: {
        request->set_status(DTRStatus::CHECKING_CACHE);
        Arc::CreateThreadFunction(&DTRCheckCache, (void*)arg, &thread_count);
      }; break;

      case DTRStatus::RESOLVE: {
        request->set_status(DTRStatus::RESOLVING);
        if (bulk_arg) Arc::CreateThreadFunction(&DTRBulkResolve, (void*)bulk_arg, &thread_count);
        else if (arg) Arc::CreateThreadFunction(&DTRResolve, (void*)arg, &thread_count);
      }; break;

      case DTRStatus::QUERY_REPLICA: {
        request->set_status(DTRStatus::QUERYING_REPLICA);
        if (bulk_arg) Arc::CreateThreadFunction(&DTRBulkQueryReplica, (void*)bulk_arg, &thread_count);
        else if (arg) Arc::CreateThreadFunction(&DTRQueryReplica, (void*)arg, &thread_count);
      }; break;

      case DTRStatus::PRE_CLEAN: {
        request->set_status(DTRStatus::PRE_CLEANING);
        Arc::CreateThreadFunction(&DTRPreClean, (void*)arg, &thread_count);
      }; break;

      case DTRStatus::STAGE_PREPARE: {
        request->set_status(DTRStatus::STAGING_PREPARING);
        Arc::CreateThreadFunction(&DTRStagePrepare, (void*)arg, &thread_count);
      }; break;

      // post-processor states

      case DTRStatus::RELEASE_REQUEST: {
        request->set_status(DTRStatus::RELEASING_REQUEST);
        Arc::CreateThreadFunction(&DTRReleaseRequest, (void*)arg, &thread_count);
      }; break;

      case DTRStatus::REGISTER_REPLICA: {
        request->set_status(DTRStatus::REGISTERING_REPLICA);
        Arc::CreateThreadFunction(&DTRRegisterReplica, (void*)arg, &thread_count);
      }; break;

      case DTRStatus::PROCESS_CACHE: {
        request->set_status(DTRStatus::PROCESSING_CACHE);
        Arc::CreateThreadFunction(&DTRProcessCache, (void*)arg, &thread_count);
      }; break;

      default: {
        // unexpected state - report error
        request->set_error_status(DTRErrorStatus::INTERNAL_LOGIC_ERROR,
                              DTRErrorStatus::ERROR_UNKNOWN,
                              "Received a DTR in an unexpected state ("+request->get_status().str()+") in processor");
        DTR::push(request, SCHEDULER);
        if (arg) delete arg;
        if (bulk_arg) delete bulk_arg;
      }; break;
    }
  }

  void Processor::start(void) {
  }

  void Processor::stop(void) {
    // threads are short lived so wait for them to complete rather than interrupting
    thread_count.wait(60*1000);
  }

} // namespace DataStaging
