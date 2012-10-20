#ifndef DTR_GENERATOR_H_
#define DTR_GENERATOR_H_

#include <arc/data-staging/DTR.h>
#include <arc/data-staging/Scheduler.h>

#include "../conf/conf_staging.h"

class GMConfig;
class FileData;
class JobDescription;

/**
 * DTRInfo passes state information from data staging to A-REX
 * via the defined callback, called when the DTR passes to the
 * certain processes. It could for example write to files in the
 * control directory, and this information can be picked up and
 * published by the info system.
 */
class DTRInfo: public DataStaging::DTRCallback {
 private:
  const GMConfig& config;
  static Arc::Logger logger;
 public:
  /** JobUsers is needed to find the correct control dir */
  DTRInfo(const GMConfig& config);
  virtual void receiveDTR(DataStaging::DTR_ptr dtr);
};


/**
 * A-REX implementation of DTR Generator. Note that neither Janitor nor job
 * migration functionality present in the down/uploaders has been implemented
 * here.
 */
class DTRGenerator: public DataStaging::DTRCallback {
 private:
  /** Active DTRs. Map of job id to DTR id. */
  std::multimap<std::string, std::string> active_dtrs;
  /** Jobs where all DTRs are finished. Map of job id to failure reason (empty if success) */
  std::map<std::string, std::string> finished_jobs;
  /** Lock for lists */
  Arc::SimpleCondition lock;

  // Event lists
  /** DTRs received */
  std::list<DataStaging::DTR_ptr> dtrs_received;
  /** Jobs received */
  std::list<JobDescription> jobs_received;
  /** Jobs cancelled. List of Job IDs. */
  std::list<std::string> jobs_cancelled;
  /** Lock for events */
  Arc::SimpleCondition event_lock;

  /** Condition to wait on when stopping Generator */
  Arc::SimpleCondition run_condition;
  /** State of Generator */
  DataStaging::ProcessState generator_state;
  /** Grid manager configuration */
  const GMConfig& config;
  /** A list of files left mid-transfer from a previous process */
  std::list<std::string> recovered_files;
  /** logger to a-rex log */
  static Arc::Logger logger;
  /** Associated scheduler */
  DataStaging::Scheduler* scheduler;

  /** Staging configuration */
  StagingConfig staging_conf;

  /** Info object for passing DTR info back to A-REX */
  DTRInfo info;

  //static DTRGeneratorCallback receive_dtr;
  
  /** Function and arguments for callback when all DTRs for a job have finished */
  void (*kicker_func)(void*);
  void* kicker_arg;

  /** Private constructors */
  DTRGenerator(const DTRGenerator& generator);

  /** run main thread */
  static void main_thread(void* arg);
  void thread(void);

  /** Process a received DTR */
  bool processReceivedDTR(DataStaging::DTR_ptr dtr);
  /** Process a received job */
  bool processReceivedJob(const JobDescription& job);
  /** Process a cancelled job */
  bool processCancelledJob(const std::string& jobid);

  /** Read in state left from previous process and fill recovered_files */
  void readDTRState(const std::string& dtr_log);

  /** Clean up joblinks dir in caches for given job (called at the end of upload) */
  void CleanCacheJobLinks(const GMConfig& config, const JobDescription& job) const;

  /** Check that user-uploadable file exists.
   * Returns 0 - if file exists
   *         1 - it is not proper file or other error
   *         2 - not there yet
   * @param dt Filename and size/checksum information
   * @param session_dir Directory in which to find uploaded file
   * @param jobid Job ID, used in log messages
   * @param error Errors are reported in this string
   * @param uid uid under which to access session dir
   * @param gid gid under which to access session dir
   */
  static int user_file_exists(FileData &dt,
                              const std::string& session_dir,
                              const std::string& jobid,
                              std::string& error,
                              uid_t uid, gid_t gid);

 public:
  /**
   * Start up Generator.
   * @param user Grid manager configuration.
   * @param kicker_func Function to call on completion of all DTRs for a job
   * @param kicker_arg Argument to kicker function
   */
  DTRGenerator(const GMConfig& config,
               void (*kicker_func)(void*) = NULL,
               void* kicker_arg = NULL);
  /**
   * Stop Generator
   */
  ~DTRGenerator();

  operator bool(void) { return (generator_state == DataStaging::RUNNING); };
  bool operator!(void) { return (generator_state != DataStaging::RUNNING); };

  /**
   * Callback called when DTR is finished. This DTR is marked done in the
   * DTR list and if all DTRs for the job have completed, the job is marked
   * as done.
   * @param dtr DTR object sent back from the Scheduler
   */
  virtual void receiveDTR(DataStaging::DTR_ptr dtr);

  /**
   * A-REX sends data transfer requests to the data staging system through
   * this method. It reads the job.id.input/output files, forms DTRs and
   * sends them to the Scheduler.
   * @param job Job description object.
   */
  void receiveJob(const JobDescription& job);

  /**
   * This method is used by A-REX to cancel on-going DTRs. A cancel request
   * is made for each DTR in the job and the method returns. The Scheduler
   * asychronously deals with cancelling the DTRs.
   * @param job The job which is being cancelled
   */
  void cancelJob(const JobDescription& job);

  /**
   * Query status of DTRs in job. If all DTRs are finished, returns true,
   * otherwise returns false. If true is returned, the JobDescription should
   * be checked for whether the staging was successful or not by checking
   * GetFailure().
   * @param job Description of job to query. Can be modified to add a failure
   * reason.
   * @return True if all DTRs in the job are finished, false otherwise.
   */
  bool queryJobFinished(JobDescription& job);

  /**
   * Query whether the Generator has a record of this job.
   * @param job Job to query.
   * @return True if the job is active or finished.
   */
  bool hasJob(const JobDescription& job);

  /**
   * Remove the job from the Generator. Only finished jobs will be removed,
   * and a warning will be logged if the job still has active DTRs. This
   * method should be called after A-REX has finished PREPARING or FINISHING.
   * @param job The job to remove.
   */
  void removeJob(const JobDescription& job);

  /**
   * Utility method to check that all files the user was supposed to
   * upload with the job are ready.
   * @param job Job description, failures will be reported directly in
   * this object.
   * @return 0 if file exists, 1 if it is not a proper file or other error,
   * 2 if the file not there yet
   */
  int checkUploadedFiles(JobDescription& job);
};

#endif /* DTR_GENERATOR_H_ */
