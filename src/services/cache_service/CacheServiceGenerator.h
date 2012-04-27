#ifndef CACHESERVICEGENERATOR_H_
#define CACHESERVICEGENERATOR_H_

#include <arc/data-staging/DTR.h>
#include <arc/data-staging/Scheduler.h>

#include "../a-rex/grid-manager/conf/conf_staging.h"
#include "../a-rex/grid-manager/jobs/users.h"

namespace Cache {

  /// DTR Generator for the cache service.
  class CacheServiceGenerator : public DataStaging::DTRCallback {

  private:
    /// Scheduler object to process DTRs
    DataStaging::Scheduler scheduler;
    /// Generator state
    DataStaging::ProcessState generator_state;
    /// Whether to use the host certificate when communicating with remote delivery
    bool use_host_cert;
    /// Scratch directory used by job
    std::string scratch_dir;

    /// Map of job id to DTRs
    std::multimap<std::string, DataStaging::DTR_ptr> processing_dtrs;
    /// Lock for DTR map
    Arc::SimpleCondition processing_lock;
    /// Map of job id to error message, if any
    std::map<std::string, std::string> finished_jobs;
    /// Lock for finished job map
    Arc::SimpleCondition finished_lock;

    /// Logger
    static Arc::Logger logger;

  public:
    /// Read in staging configuration and start data staging threads
    CacheServiceGenerator(const JobUsers& users);

    /// Callback method to receive completed DTRs
    void receiveDTR(DataStaging::DTR_ptr dtr);

    /// Add a new request.
    /**
     * @param jobuser JobUser for this transfer
     * @param source Source file
     * @param destination Destination file
     * @param usercfg UserConfig with proxy information
     * @param jobid Job identifier
     * @param uid uid under which to access session dir
     * @param priority DTR priority
     */
    bool addNewRequest(const JobUser& jobuser,
                       const std::string& source,
                       const std::string& destination,
                       const Arc::UserConfig& usercfg,
                       const std::string& jobid,
                       uid_t uid,
                       int priority);

    /// Query requests for given job id.
    /**
     * @param jobid Job ID to query
     * @param error If any DTR finished with an error, the description is put
     * in error.
     * @return True if all requests for the job have finished, false otherwise
     */
    bool queryRequestsFinished(const std::string& jobid, std::string& error);
  };

} // namespace Cache

#endif /* CACHESERVICEGENERATOR_H_ */
