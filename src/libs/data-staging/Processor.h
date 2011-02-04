#ifndef PROCESSOR_H_
#define PROCESSOR_H_

#include <arc/Logger.h>

#include "DTR.h"

namespace DataStaging {

  /**
   * The Processor takes care of everything that should happen before
   * and after a transfer takes place. It is a singleton class and
   * calling processDTR() spawns a thread to perform the required
   * operation depending on the DTR state.
   */
  class Processor {

   private:

//    static Processor* instance;
    Processor(const Processor&);
    Processor& operator=(const Processor&);

    class ThreadArgument {
     public:
      Processor* proc;
      DTR* dtr;
      ThreadArgument(Processor* proc_, DTR* dtr_):proc(proc_),dtr(dtr_) { };
    };

    Arc::SimpleCounter thread_count;

    /** Thread methods which deal with each state */
    static void DTRCheckCache(void* arg);
    static void DTRResolve(void* arg);
    static void DTRQueryReplica(void* arg);
    static void DTRPreClean(void *arg);
    static void DTRStagePrepare(void* arg);
    static void DTRReleaseRequest(void* arg);
    static void DTRRegisterReplica(void* arg);
    static void DTRProcessCache(void* arg);

   public:
    Processor() {};
    ~Processor() { stop(); };
    /**
     * This method is doing nothing. It is here only to make all classes
     * of data staging to look alike. But it is better to call it before
     * starting to use object because it may do something in a future.
     */
    void start(void);
    /**
     * This method send signal to abort all DTR processing. Then it waits
     * for all started threads to end and exits.
     */
    void stop(void);
    /**
     * Get the single instance of the Processor. Note that the creation
     * of the object is not thread-safe so it must be created in some
     * single-threaded initialisation stage.
     * @return The single Processor instance
     */
    //static Processor* getInstance();
    /**
     * The DTR is sent to the Processor through this method when some
     * long-latency processing is to be performed, eg contacting a
     * remote service. The Processor spawns a thread to do the processing,
     * and then returns. The thread notifies the scheduler when
     * it is finished.
     */
    void processDTR(DTR* dtr);
  };


} // namespace DataStaging


#endif /* PROCESSOR_H_ */
