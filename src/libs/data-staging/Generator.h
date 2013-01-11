#ifndef GENERATOR_H_
#define GENERATOR_H_

#include <arc/Thread.h>
#include <arc/Logger.h>

#include "Scheduler.h"

namespace DataStaging {

  /// Simple Generator implementation
  /**
   * This Generator implementation is included in the data staging library for
   * for basic direct testing of the library and to show how a Generator can
   * be written. It has one method, run(), which creates a single DTR
   * and submits it to the Scheduler.
   * \headerfile Generator.h arc/data-staging/Generator.h
   */
  class Generator: public DTRCallback {

   private:

    /// Condition to wait on until DTR has finished
    static Arc::SimpleCondition cond;

    /// DTR Scheduler
    Scheduler scheduler;

    /// Interrupt signal handler
    static void shutdown(int sig);

    /// Logger object
    static Arc::Logger logger;
    /// Root LogDestinations to be used in receiveDTR
    std::list<Arc::LogDestination*> root_destinations;

   public:

    /// Counter for main to know how many DTRs are in the system
    Arc::SimpleCounter counter;

    /// Create a new Generator. start() must be called to start DTR threads.
    Generator();
    /// Stop Generator and DTR threads
    ~Generator();

    /// Implementation of callback from DTRCallback
    /**
     * Callback method used when DTR processing is complete to pass the DTR
     * back to the generator. It decrements counter.
     */
    virtual void receiveDTR(DTR_ptr dtr);

    /// Start Generator and DTR threads
    void start();

    /// Submit a DTR with given source and destination. Increments counter.
    void run(const std::string& source, const std::string& destination);
  };

} // namespace DataStaging

#endif /* GENERATOR_H_ */
