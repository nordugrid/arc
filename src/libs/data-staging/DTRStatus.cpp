#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "DTRStatus.h"

namespace DataStaging {

  static const DTRStatus::DTRStatusType states[] = {
      DTRStatus::CHECK_CACHE,
      DTRStatus::RESOLVE,
      DTRStatus::QUERY_REPLICA,
      DTRStatus::PRE_CLEAN,
      DTRStatus::STAGE_PREPARE,
      DTRStatus::TRANSFER,
      DTRStatus::RELEASE_REQUEST,
      DTRStatus::REGISTER_REPLICA,
      DTRStatus::PROCESS_CACHE
  };

  const std::vector<DTRStatus::DTRStatusType> DTRStatus::ToProcessStates(states, states + sizeof states / sizeof states[0]);

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
