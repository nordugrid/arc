#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "DTRStatus.h"

namespace DataStaging {

  // to do states
  static const DTRStatus::DTRStatusType to_process_states[] = {
      DTRStatus::CHECK_CACHE,
      DTRStatus::RESOLVE,
      DTRStatus::QUERY_REPLICA,
      DTRStatus::PRE_CLEAN,
      DTRStatus::STAGE_PREPARE,
      DTRStatus::TRANSFER,
      DTRStatus::RELEASE_REQUEST,
      DTRStatus::FINALISE_REPLICA,
      DTRStatus::REGISTER_REPLICA,
      DTRStatus::PROCESS_CACHE
  };

  // doing states
  static const DTRStatus::DTRStatusType processing_states[] = {
      DTRStatus::CHECKING_CACHE,
      DTRStatus::RESOLVING,
      DTRStatus::QUERYING_REPLICA,
      DTRStatus::PRE_CLEANING,
      DTRStatus::STAGING_PREPARING,
      DTRStatus::TRANSFERRING,
      DTRStatus::RELEASING_REQUEST,
      DTRStatus::FINALISING_REPLICA,
      DTRStatus::REGISTERING_REPLICA,
      DTRStatus::PROCESSING_CACHE
  };

  static const DTRStatus::DTRStatusType staged_states[] = {
      DTRStatus::STAGING_PREPARING,
      DTRStatus::STAGING_PREPARING_WAIT,
      DTRStatus::STAGED_PREPARED,
      DTRStatus::TRANSFER,
      DTRStatus::TRANSFERRING,
      DTRStatus::TRANSFERRING_CANCEL,
  };


  const std::vector<DTRStatus::DTRStatusType> DTRStatus::ToProcessStates(to_process_states,
      to_process_states + sizeof to_process_states / sizeof to_process_states[0]);

  const std::vector<DTRStatus::DTRStatusType> DTRStatus::ProcessingStates(processing_states,
      processing_states + sizeof processing_states / sizeof processing_states[0]);

  const std::vector<DTRStatus::DTRStatusType> DTRStatus::StagedStates(staged_states,
      staged_states + sizeof staged_states / sizeof staged_states[0]);

  static const std::string status_string[DTRStatus::NULL_STATE + 1] = {
    "NEW",
    "CHECK_CACHE",
    "CHECKING_CACHE",
    "CACHE_WAIT",
    "CACHE_CHECKED",
    "RESOLVE",
    "RESOLVING",
    "RESOLVED",
    "QUERY_REPLICA",
    "QUERYING_REPLICA",
    "REPLICA_QUERIED",
    "PRE_CLEAN",
    "PRE_CLEANING",
    "PRE_CLEANED",
    "STAGE_PREPARE",
    "STAGING_PREPARING",
    "STAGING_PREPARING_WAIT",
    "STAGED_PREPARED",
    "TRANSFER",
    "TRANSFERRING",
    "TRANSFERRING_CANCEL",
    "TRANSFERRED",
    "RELEASE_REQUEST",
    "RELEASING_REQUEST",
    "REQUEST_RELEASED",
    "FINALISE_REPLICA",
    "FINALISING_REPLICA",
    "REPLICA_FINALISED",
    "REGISTER_REPLICA",
    "REGISTERING_REPLICA",
    "REPLICA_REGISTERED",
    "PROCESS_CACHE",
    "PROCESSING_CACHE",
    "CACHE_PROCESSED",
    "DONE",
    "CANCELLED",
    "CANCELLED_FINISHED",
    "ERROR",
    "NULL_STATE"
  };

  std::string DTRStatus::str() const {
    return status_string[status];
  }

} // namespace DataStaging
