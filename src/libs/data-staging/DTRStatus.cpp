#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "DTRStatus.h"

namespace DataStaging {

  static const std::string status_string[DTRStatus::NULL_STATE + 1] = {
    "NEW",
    "CHECK_CACHE",
    "RESOLVE",
    "QUERY_REPLICA",
    "PRE_CLEAN",
    "STAGE_PREPARE",
    "TRANSFER_WAIT",
    "TRANSFER",
    "RELEASE_REQUEST",
    "REGISTER_REPLICA",
    "PROCESS_CACHE",
    "DONE",
    "CANCELLED",
    "CANCELLED_FINISHED",
    "ERROR",
    "CHECKING_CACHE",
    "CACHE_WAIT",
    "CACHE_CHECKED",
    "RESOLVING",
    "RESOLVED",
    "QUERYING_REPLICA",
    "REPLICA_QUERIED",
    "PRE_CLEANING",
    "PRE_CLEANED",
    "STAGING_PREPARING",
    "STAGING_PREPARING_WAIT",
    "STAGED_PREPARED",
    "TRANSFERRING",
    "TRANSFERRING_CANCEL",
    "TRANSFERRED",
    "RELEASING_REQUEST",
    "REQUEST_RELEASED",
    "REGISTERING_REPLICA",
    "REPLICA_REGISTERED",
    "PROCESSING_CACHE",
    "CACHE_PROCESSED",
    "NULL_STATE"
  };

  std::string DTRStatus::str() const {
    return status_string[status];
  }

} // namespace DataStaging
