#ifndef PROCESSOR_H_
#define PROCESSOR_H_

#include <arc/Logger.h>

#include "DTR.h"

namespace DataStaging {

  /// The Processor performs pre- and post-transfer operations.
  /**
   * The Processor takes care of everything that should happen before
   * and after a transfer takes place. Calling receiveDTR() spawns a
   * thread to perform the required operation depending on the DTR state.
   * \ingroup datastaging
   * \headerfile Processor.h arc/data-staging/Processor.h
   */
  class Processor: public DTRCallback {

   private:

    /// Private copy constructor because Processor should not be copied
    Processor(const Processor&);
    /// Private assignment operator because Processor should not be copied
    Processor& operator=(const Processor&);

    /// Class used to pass information to spawned thread
    class ThreadArgument {
     public:
      Processor* proc;
      DTR_ptr dtr;
      ThreadArgument(Processor* proc_, DTR_ptr dtr_):proc(proc_),dtr(dtr_) { };
    };

    /// Class used to pass information to spawned thread (for bulk operations)
    class BulkThreadArgument {
     public:
      Processor* proc;
      std::list<DTR_ptr> dtrs;
      BulkThreadArgument(Processor* proc_, const std::list<DTR_ptr>& dtrs_):proc(proc_),dtrs(dtrs_) { };
    };

    /// Counter of active threads
    Arc::SimpleCounter thread_count;

    /// List of DTRs to be processed in bulk. Filled between receiveDTR
    /// receiving a DTR with bulk_start on and receiving one with bulk_end on.
    /// It is up to the caller to make sure that all the requests are suitable
    /// for bulk handling. The list is cleared after the DTR with bulk_end set.
    std::list<DTR_ptr> bulk_list;

    /* Thread methods which deal with each state */
    /// Check the cache to see if the file already exists
    static void DTRCheckCache(void* arg);
    /// Resolve replicas of source and destination
    static void DTRResolve(void* arg);
    /// Bulk resolve replicas of source and destination
    static void DTRBulkResolve(void* arg);
    /// Check if source exists
    static void DTRQueryReplica(void* arg);
    /// Bulk check if source exists
    static void DTRBulkQueryReplica(void* arg);
    /// Remove destination file before creating a new version
    static void DTRPreClean(void *arg);
    /// Call external services to prepare physical files for reading/writing
    static void DTRStagePrepare(void* arg);
    /// Release requests made during DTRStagePrepare
    static void DTRReleaseRequest(void* arg);
    /// Finalise replica
    static void DTRFinaliseReplica(void* arg);
    /// Register destination file in catalog
    static void DTRRegisterReplica(void* arg);
    /// Link cached file to final destination
    static void DTRProcessCache(void* arg);

   public:

    /// Constructor
    Processor() {};
    /// Destructor waits for all active threads to stop.
    ~Processor() { stop(); };

    /// Start Processor.
    /**
     * This method actually does nothing. It is here only to make all classes
     * of data staging to look alike. But it is better to call it before
     * starting to use object because it may do something in the future.
     */
    void start(void);

    /// Stop Processor.
    /**
     * This method sends waits for all started threads to end and exits. Since
     * threads are short-lived it is better to wait rather than interrupt them.
     */
    void stop(void);

    /// Send a DTR to the Processor.
    /**
     * The DTR is sent to the Processor through this method when some
     * long-latency processing is to be performed, eg contacting a
     * remote service. The Processor spawns a thread to do the processing,
     * and then returns. The thread pushes the DTR back to the scheduler when
     * it is finished.
     */
    virtual void receiveDTR(DTR_ptr dtr);
  };


} // namespace DataStaging


#endif /* PROCESSOR_H_ */
