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

  /** Set up logging. Should be called at the start of each thread method. */
  void setUpLogger(DTR* request) {
    // disconnect this thread's root logger
    Arc::Logger::getRootLogger().setThreadContext();
    Arc::Logger::getRootLogger().removeDestinations();
    Arc::Logger::getRootLogger().addDestinations(request->get_logger()->getDestinations());
    // now disconnect the DTR logger - the root logger is enabled and we
    // don't want duplicate messages. IMPORTANT: the DTR logger must be
    // re-enabled at the end of the thread
    request->disconnect_logger();
  }


  /* Thread methods for each state of the DTR */

  void Processor::DTRCheckCache(void* arg) {
    ThreadArgument* targ = (ThreadArgument*)arg;
    DTR* request = targ->dtr;

    setUpLogger(request);

    // IMPORTANT: This method creates a lock on the cached file for
    // this DTR. It must be released at some point using ProcessCache

    // Create cache using configuration
    Arc::FileCache cache(request->get_cache_parameters().cache_dirs,
                         request->get_cache_parameters().remote_cache_dirs,
                         request->get_cache_parameters().drain_cache_dirs,
                         request->get_parent_job_id(),
                         request->get_local_user().get_uid(),
                         request->get_local_user().get_gid());

    if (!cache) {
      request->get_logger()->msg(Arc::ERROR, "DTR %s: Error creating cache", request->get_short_id());
      request->set_cache_state(CACHE_SKIP);
      request->set_error_status(DTRErrorStatus::CACHE_ERROR,
                            DTRErrorStatus::ERROR_DESTINATION,
                            "Failed to create cache");
      request->set_status(DTRStatus::CACHE_CHECKED);
      request->connect_logger();
      request->push(SCHEDULER);
      return;
    }
    // DN is used for checking cache permissions
    Arc::Credential cred(request->get_usercfg().ProxyPath(),
                         request->get_usercfg().ProxyPath(),
                         request->get_usercfg().CACertificatesDirectory(), "");
    std::string dn = cred.GetIdentityName();
    Arc::Time exp_time = cred.GetEndTime();

    std::string canonic_url(request->get_source()->GetURL().plainstr());
    // add guid if present
    // TODO handle guids better in URL class so we don't need to care here
    if (!request->get_source()->GetURL().MetaDataOption("guid").empty())
      canonic_url += ":guid=" + request->get_source()->GetURL().MetaDataOption("guid");

    bool is_in_cache = false;
    bool is_locked = false;
    bool use_remote = true;
    for (;;) {
      if (!cache.Start(canonic_url, is_in_cache, is_locked, use_remote)) {
        if (is_locked) {
          request->get_logger()->msg(Arc::WARNING, "DTR %s: Cached file is locked - should retry", request->get_short_id());
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
          request->get_logger()->msg(Arc::INFO, "DTR %s: Will wait around %is", request->get_short_id(), cache_wait_time);
          request->set_process_time(cache_wait_period);

          request->connect_logger();
          request->push(SCHEDULER);
          return;
        }
        request->get_logger()->msg(Arc::ERROR, "DTR %s: Failed to initiate cache", request->get_short_id());
        request->set_cache_state(CACHE_SKIP);
        request->set_error_status(DTRErrorStatus::CACHE_ERROR,
                              DTRErrorStatus::ERROR_DESTINATION,
                              "Failed to initiate cache");
        break;
      }
      request->set_cache_file(cache.File(canonic_url));
      if (is_in_cache) {
        // check for forced re-download option
        std::string cache_option = request->get_source()->GetURL().Option("cache");
        if (cache_option == "renew") {
          request->get_logger()->msg(Arc::VERBOSE, "DTR %s: Forcing re-download of file %s", request->get_short_id(), canonic_url);
          cache.StopAndDelete(canonic_url);
          use_remote = false;
          continue;
        }
        // just need to check permissions
        request->get_logger()->msg(Arc::INFO, "DTR %s: File %s is cached (%s) - checking permissions",
                   request->get_short_id(), canonic_url, cache.File(canonic_url));
        // check the list of cached DNs
        bool have_permission = false;
        if (cache.CheckDN(canonic_url, dn))
          have_permission = true;
        else {
          Arc::DataStatus cres = request->get_source()->Check();
          if (!cres.Passed()) {
            request->get_logger()->msg(Arc::ERROR, "DTR %s: Permission checking failed", request->get_short_id());
            cache.Stop(canonic_url);
            request->set_cache_state(CACHE_SKIP);
            request->set_error_status(DTRErrorStatus::CACHE_ERROR,
                                  DTRErrorStatus::ERROR_DESTINATION,
                                  "Failed to check cache permissions for " + canonic_url);
            break;
          }
          cache.AddDN(canonic_url, dn, exp_time);
        }
        request->get_logger()->msg(Arc::INFO, "DTR %s: Permission checking passed", request->get_short_id());
        // check if file is fresh enough
        bool outdated = true;
        if (have_permission)
          outdated = false; // cached DN means don't check creation date
        if (request->get_source()->CheckCreated() && cache.CheckCreated(canonic_url)) {
          Arc::Time sourcetime = request->get_source()->GetCreated();
          Arc::Time cachetime = cache.GetCreated(canonic_url);
          request->get_logger()->msg(Arc::VERBOSE, "DTR %s: Source creation date: %s", request->get_short_id(), sourcetime.str());
          request->get_logger()->msg(Arc::VERBOSE, "DTR %s: Cache creation date: %s", request->get_short_id(), cachetime.str());
          if (sourcetime <= cachetime)
            outdated = false;
        }
        if (cache.CheckValid(canonic_url)) {
          Arc::Time validtime = cache.GetValid(canonic_url);
          request->get_logger()->msg(Arc::VERBOSE, "DTR %s: Cache file valid until: %s", validtime.str(), request->get_short_id());
          if (validtime > Arc::Time())
            outdated = false;
          else
            outdated = true;
        }
        if (outdated) {
          cache.StopAndDelete(canonic_url);
          request->get_logger()->msg(Arc::INFO, "DTR %s: Cached file is outdated, will re-download", request->get_short_id());
          use_remote = false;
          continue;
        }
        // cached file is present and valid
        request->get_logger()->msg(Arc::VERBOSE, "DTR %s: Cached copy is still valid", request->get_short_id());
        request->set_cache_state(CACHE_ALREADY_PRESENT);
      }
      else { // file is not there but we are ready to download it
        request->get_logger()->msg(Arc::VERBOSE, "DTR %s: Will download to cache file %s", request->get_short_id(), request->get_cache_file());
        request->set_cache_state(CACHEABLE);
      }
      break;
    }
    request->set_status(DTRStatus::CACHE_CHECKED);
    request->connect_logger();
    request->push(SCHEDULER);
  }

  void Processor::DTRResolve(void* arg) {
    // call request.source.Resolve() to get replicas
    // call request.destination.Resolve() to check supplied replicas
    // call request.destination.PreRegister() to lock destination LFN
    ThreadArgument* targ = (ThreadArgument*)arg;
    DTR* request = targ->dtr;

    setUpLogger(request);

    // check for source replicas
    if (request->get_source()->IsIndex()) {
      request->get_logger()->msg(Arc::VERBOSE, "DTR %s: Looking up source replicas", request->get_short_id());
      Arc::DataStatus res = request->get_source()->Resolve(true);
      if (!res.Passed() || !request->get_source()->HaveLocations() || !request->get_source()->LocationValid()) {
        request->get_logger()->msg(Arc::ERROR, "DTR %s: Failed to resolve any source replicas", request->get_short_id());
        request->set_error_status(res.Retryable() ? DTRErrorStatus::TEMPORARY_REMOTE_ERROR : DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                  DTRErrorStatus::ERROR_SOURCE,
                                  "Could not resolve any source replicas for " + request->get_source()->str());
        request->set_status(DTRStatus::RESOLVED);
        request->connect_logger();
        request->push(SCHEDULER);
        return;
      }
    }
    // check replicas supplied for destination
    if (request->get_destination()->IsIndex()) {
      request->get_logger()->msg(Arc::VERBOSE, "DTR %s: Resolving destination replicas", request->get_short_id());
      Arc::DataStatus res = request->get_destination()->Resolve(false);
      if (!res.Passed() || !request->get_destination()->HaveLocations() || !request->get_destination()->LocationValid()) {
        request->get_logger()->msg(Arc::ERROR, "DTR %s: Failed to resolve destination replicas", request->get_short_id());
        request->set_error_status(res.Retryable() ? DTRErrorStatus::TEMPORARY_REMOTE_ERROR : DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                  DTRErrorStatus::ERROR_DESTINATION,
                                  "Could not resolve any destination replicas for " + request->get_destination()->str());
        request->set_status(DTRStatus::RESOLVED);
        request->connect_logger();
        request->push(SCHEDULER);
        return;
      }
    }
    // check that replication is possible
    if (request->is_replication()) {
      // we do not want to replicate to same physical file
      request->get_destination()->RemoveLocations(*(request->get_source()));
      if (!request->get_destination()->HaveLocations()) {
        request->get_logger()->msg(Arc::ERROR, "DTR %s: No locations for destination different from source found");
        request->set_error_status(DTRErrorStatus::SELF_REPLICATION_ERROR,
                                  DTRErrorStatus::NO_ERROR_LOCATION,
                                  "No locations for destination different from source found for " + request->get_destination()->str());
        request->set_status(DTRStatus::RESOLVED);
        request->connect_logger();
        request->push(SCHEDULER);
        return;
      }
    }
    // pre-register destination
    if (request->get_destination()->IsIndex()) {
      request->get_logger()->msg(Arc::VERBOSE, "DTR %s: Pre-registering destination in index service", request->get_short_id());
      Arc::DataStatus res = request->get_destination()->PreRegister(request->is_replication(), request->is_force_registration());
      if (!res.Passed()) {
        request->get_logger()->msg(Arc::ERROR, "DTR %s: Failed to pre-register destination", request->get_short_id());
        request->set_error_status(res.Retryable() ? DTRErrorStatus::TEMPORARY_REMOTE_ERROR : DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                  DTRErrorStatus::ERROR_DESTINATION,
                                  "Could not pre-register destination " + request->get_destination()->str());
      }
    }
    // finished with resolving - send back to scheduler
    request->set_status(DTRStatus::RESOLVED);
    request->connect_logger();
    request->push(SCHEDULER);
  }

  void Processor::DTRQueryReplica(void* arg) {
    // check source is ok and obtain metadata
    ThreadArgument* targ = (ThreadArgument*)arg;
    DTR* request = targ->dtr;

    setUpLogger(request);

    Arc::DataStatus res;
    request->get_logger()->msg(Arc::INFO, "DTR %s: Checking %s", request->get_short_id(), request->get_source()->CurrentLocation().str());
    if (request->get_source()->IsIndex()) {
      res = request->get_source()->CompareLocationMetadata();
    } else {
      Arc::FileInfo file;
      res = request->get_source()->Stat(file, Arc::DataPoint::INFO_TYPE_CONTENT);
    }

    if (res == Arc::DataStatus::InconsistentMetadataError) {
      request->get_logger()->msg(Arc::ERROR, "DTR %s: Metadata of replica and index service differ", request->get_short_id());
      request->set_error_status(DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                DTRErrorStatus::ERROR_SOURCE,
                                "Metadata of replica and index service differ for " +
                                  request->get_source()->CurrentLocation().str() +
                                  request->get_source()->str());
    }
    else if (!res.Passed()) {
      request->get_logger()->msg(Arc::ERROR, "DTR %s: Failed checking source replica %s", request->get_short_id(), request->get_source()->CurrentLocation().str());
      request->set_error_status(res.Retryable() ? DTRErrorStatus::TEMPORARY_REMOTE_ERROR : DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                DTRErrorStatus::ERROR_SOURCE,
                                "Failed checking source replica " + request->get_source()->CurrentLocation().str());
    }
    else {
      // assign metadata to destination
      request->get_destination()->SetMeta(*request->get_source());
    }
    // finished querying - send back to scheduler
    request->set_status(DTRStatus::REPLICA_QUERIED);
    request->connect_logger();
    request->push(SCHEDULER);
  }

  void Processor::DTRPreClean(void *arg) {
    // for physical files call Remove()
    // for index services delete entry and all existing replicas
    // only if the entry already exists
    ThreadArgument* targ = (ThreadArgument*)arg;
    DTR* request = targ->dtr;
    setUpLogger(request);

    Arc::DataStatus res = Arc::DataStatus::Success;

    if (!request->get_destination()->IsIndex()) {
      request->get_logger()->msg(Arc::INFO, "DTR %s: Removing %s", request->get_short_id(), request->get_destination()->CurrentLocation().str());
      res = request->get_destination()->Remove();
    }
    else if (request->get_destination()->Registered() && !request->is_replication()) {
      // get existing locations
      Arc::DataHandle dest(request->get_destination()->GetURL(), request->get_destination()->GetUserConfig());
      request->get_logger()->msg(Arc::VERBOSE, "DTR %s: Finding existing destination replicas", request->get_short_id());
      res = dest->Resolve(true);
      if (!res.Passed()) {
        request->get_logger()->msg(Arc::ERROR, "DTR %s: Error resolving destination replicas", request->get_short_id());
      }
      else {
        if (dest->HaveLocations()) {
          while (dest->LocationValid()) {
            request->get_logger()->msg(Arc::INFO, "DTR %s: Removing %s", request->get_short_id(), dest->CurrentLocation().str());
            res = dest->Remove();
            if (!res.Passed()) {
              // if we fail to delete one replica then bail out
              request->get_logger()->msg(Arc::ERROR, "DTR %s: Failed to delete replica %s", request->get_short_id(), dest->CurrentLocation().str());
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
          request->get_logger()->msg(Arc::INFO, "DTR %s: Unregistering %s", request->get_short_id(), dest->str());
          res = dest->Unregister(true);
          // re-resolve destination and pre-register
          // remove current destination - it will be re-resolved
          request->get_destination()->RemoveLocation();
          request->get_logger()->msg(Arc::VERBOSE, "DTR %s: Re-resolving destination replicas", request->get_short_id());
          res = request->get_destination()->Resolve(false);
          if (!res.Passed()) {
            request->get_logger()->msg(Arc::ERROR, "DTR %s: Failed to resolve destination", request->get_short_id());
          } else {
            request->get_logger()->msg(Arc::VERBOSE, "DTR %s: Pre-registering destination", request->get_short_id());
            res = request->get_destination()->PreRegister(false, request->is_force_registration());
          }
        }
      }
    }
    else {
      // replication or LFN not registered
      request->get_logger()->msg(Arc::INFO, "DTR %s: There is nothing to pre-clean", request->get_short_id());
    }
    if (!res.Passed()) {
      request->get_logger()->msg(Arc::ERROR, "DTR %s: Failed to pre-clean destination", request->get_short_id());
      request->set_error_status(res.Retryable() ? DTRErrorStatus::TEMPORARY_REMOTE_ERROR : DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                DTRErrorStatus::ERROR_DESTINATION,
                                "Failed to pre-clean destination " + request->get_destination()->str());
    }
    request->set_status(DTRStatus::PRE_CLEANED);
    request->connect_logger();
    request->push(SCHEDULER);
  }

  void Processor::DTRStagePrepare(void* arg) {
    // Only valid for stageable (SRM-like) protocols.
    // Call request.source.PrepareReading() to get TURL for reading or query status of request
    // and/or request.destination.PrepareWriting() to get TURL for writing or query status of request
    ThreadArgument* targ = (ThreadArgument*)arg;
    DTR* request = targ->dtr;

    setUpLogger(request);

    // first source - if stageable and not already staged yet
    if (request->get_source()->IsStageable() && request->get_source()->TransferLocations().empty()) {
      // give default wait time for cases where no wait time is given by the remote service
      unsigned int source_wait_time = 10;
      std::list<std::string> transport_protocols; // TODO fill from URL options
      request->get_logger()->msg(Arc::VERBOSE, "DTR %s: Preparing to stage source", request->get_short_id());
      Arc::DataStatus res = request->get_source()->PrepareReading(0, source_wait_time, transport_protocols);
      if (!res.Passed()) {
        request->get_logger()->msg(Arc::ERROR, "DTR %s: Failed to prepare source", request->get_short_id());
        request->set_error_status(res.Retryable() ? DTRErrorStatus::TEMPORARY_REMOTE_ERROR : DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                  DTRErrorStatus::ERROR_SOURCE,
                                  "Failed to prepare source " + request->get_source()->CurrentLocation().str());
      }
      else if (res == Arc::DataStatus::ReadPrepareWait) {
        // if timeout then don't wait - scheduler will deal with it immediately
        if (Arc::Time() < request->get_timeout()) {
          if (source_wait_time > 60) source_wait_time = 60;
          request->set_process_time(source_wait_time);
          request->get_logger()->msg(Arc::VERBOSE, "DTR %s: Source is not ready, will wait %u seconds", request->get_short_id(), source_wait_time);
        }
        request->set_status(DTRStatus::STAGING_PREPARING_WAIT);
      }
      else {
        if (request->get_source()->TransferLocations().empty()) {
          request->get_logger()->msg(Arc::ERROR, "DTR %s: No physical files found for source", request->get_short_id());
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
      request->connect_logger();
      request->push(SCHEDULER);
      return;
    }
    // now destination - if stageable and not already staged yet
    if (request->get_destination()->IsStageable() && request->get_destination()->TransferLocations().empty()) {
      // give default wait time for cases where no wait time is given by the remote service
      unsigned int dest_wait_time = 10;
      std::list<std::string> transport_protocols; // TODO fill from URL options
      request->get_logger()->msg(Arc::VERBOSE, "DTR %s: Preparing to stage destination", request->get_short_id());
      Arc::DataStatus res = request->get_destination()->PrepareWriting(0, dest_wait_time, transport_protocols);
      if (!res.Passed()) {
        request->get_logger()->msg(Arc::ERROR, "DTR %s: Failed to prepare destination", request->get_short_id());
        request->set_error_status(res.Retryable() ? DTRErrorStatus::TEMPORARY_REMOTE_ERROR : DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                  DTRErrorStatus::ERROR_DESTINATION,
                                  "Failed to prepare destination " + request->get_destination()->CurrentLocation().str());
      }
      else if (res == Arc::DataStatus::WritePrepareWait) {
        // if timeout then don't wait - scheduler will deal with it immediately
        if (Arc::Time() < request->get_timeout()) {
          if (dest_wait_time > 60) dest_wait_time = 60;
          request->set_process_time(dest_wait_time);
          request->get_logger()->msg(Arc::VERBOSE, "DTR %s: Destination is not ready, will wait %u seconds", request->get_short_id(), dest_wait_time);
        }
        request->set_status(DTRStatus::STAGING_PREPARING_WAIT);
      }
      else {
        if (request->get_destination()->TransferLocations().empty()) {
          request->get_logger()->msg(Arc::ERROR, "DTR %s: No physical files found for destination", request->get_short_id());
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
    request->connect_logger();
    request->push(SCHEDULER);
  }

  void Processor::DTRReleaseRequest(void* arg) {
    // only valid for stageable (SRM-like) protocols. call request.source.FinishReading() and/or
    // request.destination.FinishWriting() to release or abort requests
    ThreadArgument* targ = (ThreadArgument*)arg;
    DTR* request = targ->dtr;
    setUpLogger(request);

    Arc::DataStatus res;

    if (request->get_source()->IsStageable()) {
      request->get_logger()->msg(Arc::VERBOSE, "DTR %s: Releasing source", request->get_short_id());
      res = request->get_source()->FinishReading(request->error() || request->cancel_requested());
      if (!res.Passed()) {
        // an error here is not critical to the transfer
        request->get_logger()->msg(Arc::WARNING, "DTR %s: There was a problem during post-transfer source handling", request->get_short_id());
      }
    }
    if (request->get_destination()->IsStageable()) {
      request->get_logger()->msg(Arc::VERBOSE, "DTR %s: Releasing destination", request->get_short_id());
      res = request->get_destination()->FinishWriting(request->error() || request->cancel_requested());
      if (!res.Passed()) {
        if (request->error()) {
          request->get_logger()->msg(Arc::WARNING, "DTR %s: There was a problem during post-transfer destination handling after error",
                     request->get_short_id());
        }
        else {
          request->get_logger()->msg(Arc::ERROR, "DTR %s: Error with post-transfer destination handling", request->get_short_id());
          request->set_error_status(res.Retryable() ? DTRErrorStatus::TEMPORARY_REMOTE_ERROR :
                                                      DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                    DTRErrorStatus::ERROR_DESTINATION,
                                    "Error with post-transfer destination handling of " +
                                       request->get_destination()->CurrentLocation().str());
        }
      }
    }
    request->set_status(DTRStatus::REQUEST_RELEASED);
    request->connect_logger();
    request->push(SCHEDULER);
  }

  void Processor::DTRRegisterReplica(void* arg) {
    // call request.destination.Register() to add new replica and metadata for normal workflow
    // call request.destination.PreUnregister() to delete LFN placed during
    // RESOLVE stage for error workflow
    ThreadArgument* targ = (ThreadArgument*)arg;
    DTR* request = targ->dtr;
    setUpLogger(request);

    // TODO: If the copy completed before request was cancelled, unregistering
    // here will lead to dark data. Need to check for successful copy
    if (request->error() || request->cancel_requested()) {
      request->get_logger()->msg(Arc::VERBOSE, "DTR %s: Removing pre-registered destination in index service", request->get_short_id());
      if (!request->get_destination()->PreUnregister(request->is_replication()).Passed())
        request->get_logger()->msg(Arc::ERROR, "DTR %s: Failed to unregister pre-registered destination. You may need to unregister it manually: %s",
                   request->get_short_id(), request->get_destination()->str());
    }
    else {
      request->get_logger()->msg(Arc::VERBOSE, "DTR %s: Registering destination replica", request->get_short_id());
      Arc::DataStatus res = request->get_destination()->PostRegister(request->is_replication());
      if (!res.Passed()) {
        request->get_logger()->msg(Arc::ERROR, "DTR %s: Failed to register destination replica", request->get_short_id());
        if (!request->get_destination()->PreUnregister(request->is_replication()).Passed()) {
          request->get_logger()->msg(Arc::ERROR, "DTR %s: Failed to unregister pre-registered destination. You may need to unregister it manually: %s",
                     request->get_short_id(), request->get_destination()->str());
        }
        request->set_error_status(res.Retryable() ? DTRErrorStatus::TEMPORARY_REMOTE_ERROR : DTRErrorStatus::PERMANENT_REMOTE_ERROR,
                                  DTRErrorStatus::ERROR_DESTINATION,
                                  "Could not post-register destination " + request->get_destination()->str());
      }
    }
    // finished with registration - send back to scheduler
    request->set_status(DTRStatus::REPLICA_REGISTERED);
    request->connect_logger();
    request->push(SCHEDULER);
  }

  void Processor::DTRProcessCache(void* arg) {
    // link or copy cached file to session dir, or release locks in case
    // of error or deciding not to use cache (for example because of a mapped link)
    ThreadArgument* targ = (ThreadArgument*)arg;
    DTR* request = targ->dtr;
    setUpLogger(request);

    Arc::FileCache cache(request->get_cache_parameters().cache_dirs,
                         request->get_cache_parameters().remote_cache_dirs,
                         request->get_cache_parameters().drain_cache_dirs,
                         request->get_parent_job_id(),
                         request->get_local_user().get_uid(),
                         request->get_local_user().get_gid());

    if (!cache) {
      request->get_logger()->msg(Arc::ERROR, "DTR %s: Error creating cache. Stale locks may remain.", request->get_short_id());
      request->set_error_status(DTRErrorStatus::CACHE_ERROR,
                            DTRErrorStatus::ERROR_DESTINATION,
                            "Failed to create cache for " + request->get_source()->str());
      request->set_status(DTRStatus::CACHE_PROCESSED);
      request->push(SCHEDULER);
    }

    std::string canonic_url(request->get_source()->GetURL().plainstr());
    // add guid if present
    if (!request->get_source()->GetURL().MetaDataOption("guid").empty())
      canonic_url += ":guid=" + request->get_source()->GetURL().MetaDataOption("guid");

    // check for error, cancellation or cache not being used
    if (request->error() || request->cancel_requested() || request->get_cache_state() == CACHE_NOT_USED) {
      // don't delete cache file if it was already present or successfully downloaded
      if (request->error() || (request->cancel_requested() && request->get_cache_state() == CACHEABLE)) {
        cache.StopAndDelete(canonic_url);
      } else {
        cache.Stop(canonic_url);
      }
      request->set_status(DTRStatus::CACHE_PROCESSED);
      request->connect_logger();
      request->push(SCHEDULER);
      return;
    }

    // check options for whether to copy or link
    bool executable = (request->get_source()->GetURL().Option("exec") == "yes") ? true : false;
    bool cache_copy = (request->get_source()->GetURL().Option("cache") == "copy") ? true : false;

    request->get_logger()->msg(Arc::INFO, "DTR %s: Linking/copying cached file to %s",
                               request->get_short_id(), request->get_destination()->CurrentLocation().Path());

    if (!cache.Link(request->get_destination()->CurrentLocation().Path(), canonic_url, cache_copy, executable)) {
      request->get_logger()->msg(Arc::ERROR, "DTR %s: Error linking cache file to %s.",
                                 request->get_short_id(), request->get_destination()->CurrentLocation().Path());
      request->set_error_status(DTRErrorStatus::CACHE_ERROR,
                                DTRErrorStatus::ERROR_DESTINATION,
                                "Failed to link/copy cache file to session dir");
    }
    cache.Stop(canonic_url);
    request->set_status(DTRStatus::CACHE_PROCESSED);
    request->connect_logger();
    request->push(SCHEDULER);
  }

  /* main process method called from DTR::push() */

  void Processor::receiveDTR(DTR& request) {

    ThreadArgument* arg = new ThreadArgument(this,&request);

    // switch through the expected DTR states
    switch (request.get_status().GetStatus()) {

      // pre-processor states

      case DTRStatus::CHECK_CACHE: {
        request.set_status(DTRStatus::CHECKING_CACHE);
        Arc::CreateThreadFunction(&DTRCheckCache, (void*)arg, &thread_count);
      }; break;

      case DTRStatus::RESOLVE: {
        request.set_status(DTRStatus::RESOLVING);
        Arc::CreateThreadFunction(&DTRResolve, (void*)arg, &thread_count);
      }; break;

      case DTRStatus::QUERY_REPLICA: {
        request.set_status(DTRStatus::QUERYING_REPLICA);
        Arc::CreateThreadFunction(&DTRQueryReplica, (void*)arg, &thread_count);
      }; break;

      case DTRStatus::PRE_CLEAN: {
        request.set_status(DTRStatus::PRE_CLEANING);
        Arc::CreateThreadFunction(&DTRPreClean, (void*)arg, &thread_count);
      }; break;

      case DTRStatus::STAGE_PREPARE: {
        request.set_status(DTRStatus::STAGING_PREPARING);
        Arc::CreateThreadFunction(&DTRStagePrepare, (void*)arg, &thread_count);
      }; break;

      // post-processor states

      case DTRStatus::RELEASE_REQUEST: {
        request.set_status(DTRStatus::RELEASING_REQUEST);
        Arc::CreateThreadFunction(&DTRReleaseRequest, (void*)arg, &thread_count);
      }; break;

      case DTRStatus::REGISTER_REPLICA: {
        request.set_status(DTRStatus::REGISTERING_REPLICA);
        Arc::CreateThreadFunction(&DTRRegisterReplica, (void*)arg, &thread_count);
      }; break;

      case DTRStatus::PROCESS_CACHE: {
        request.set_status(DTRStatus::PROCESSING_CACHE);
        Arc::CreateThreadFunction(&DTRProcessCache, (void*)arg, &thread_count);
      }; break;

      default: {
        // unexpected state - report error
        request.set_error_status(DTRErrorStatus::INTERNAL_LOGIC_ERROR,
                              DTRErrorStatus::ERROR_UNKNOWN,
                              "Received a DTR in an unexpected state ("+request.get_status().str()+") in processor");
        request.push(SCHEDULER);
        delete arg;
      }; break;
    }
  }

  void Processor::start(void) {
  }

  void Processor::stop(void) {
    // threads are short lived so wait for them to complete rather than interrupting
    thread_count.wait();
  }

} // namespace DataStaging
