#ifndef DTR_GENERATOR_H_
#define DTR_GENERATOR_H_

#include <arc/data-staging/DTR.h>
#include <arc/data-staging/Scheduler.h>

#include "job.h"


/**
 * DTRInfo passes state information from data staging to A-REX
 * via the defined callback, called when the DTR passes to the
 * certain processes. It could for example write to files in the
 * control directory, and this information can be picked up and
 * published by the info system.
 */
class DTRInfo: public DataStaging::DTRCallback {
 private:
  /** Job users. Map of UID to JobUser pointer, used to map a DTR or job to a JobUser. */
  std::map<uid_t, const JobUser*> jobusers;
  static Arc::Logger logger;
 public:
  /** JobUsers is needed to find the correct control dir */
  DTRInfo(const JobUsers& users);
  DTRInfo() {};
  virtual void receiveDTR(DataStaging::DTR& dtr);
};


/**
 * A-REX implementation of DTR Generator. 
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
  std::list<DataStaging::DTR> dtrs_received;
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
  /** Job users. Map of UID to JobUser pointer, used to map a DTR or job to a JobUser. */
  std::map<uid_t, const JobUser*> jobusers;
  /** logger to a-rex log */
  static Arc::Logger logger;
  /** Associated scheduler */
  DataStaging::Scheduler scheduler;

  /** Info object for passing DTR info back to A-REX */
  DTRInfo info;

  //static DTRGeneratorCallback receive_dtr;
  
  /** Function and arguments for callback when all DTRs for a job have finished */
  void (*kicker_func)(void*);
  void* kicker_arg;

  /** Private constructors */
  DTRGenerator(const DTRGenerator& generator) {};

  /** run main thread */
  static void main_thread(void* arg);
  void thread(void);

  /** Process a received DTR */
  bool processReceivedDTR(DataStaging::DTR& dtr);
  /** Process a received job */
  bool processReceivedJob(const JobDescription& job);
  /** Process a cancelled job */
  bool processCancelledJob(const std::string& jobid);

  /** Check that user-uploadable file exists.
   * Returns 0 - if file exists
   *         1 - it is not proper file or other error
   *         2 - not there yet
   * @param dt Filename and size/checksum information
   * @param session_dir Directory in which to find uploaded file
   * @param error Errors are reported in this string
   */
  static int user_file_exists(FileData &dt,
                              const std::string& session_dir,
                              std::string& error);

 public:
  /**
   * Start up Generator.
   * @param user JobUsers for this Generator.
   * @param kicker_func Function to call on completion of all DTRs for a job
   * @param kicker_arg Argument to kicker function
   */
  DTRGenerator(const JobUsers& users,
               void (*kicker_func)(void*) = NULL,
               void* kicker_arg = NULL);
  /**
   * Stop Generator
   */
  ~DTRGenerator();

  operator bool(void) { return true; };
  bool operator!(void) { return false; };

  /**
   * Callback called when DTR is finished. This DTR is marked done in the
   * DTR list and if all DTRs for the job have completed, the job is marked
   * as done.
   * @param dtr DTR object sent back from the Scheduler
   */
  virtual void receiveDTR(DataStaging::DTR& dtr);

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
