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
   */
  class Generator: public DTRCallback {

   private:

    /// Condition to wait on until DTR has finished
    static Arc::SimpleCondition cond;

    /// DTR Scheduler
    Scheduler scheduler;
    std::list<DTR_ptr> dtrs;

    /// Interrupt signal handler
    static void shutdown(int sig);

    /// Logger object
    static Arc::Logger logger;
    /// Root LogDestinations to be used in receiveDTR
    std::list<Arc::LogDestination*> root_destinations;

   public:

    /// Counter for main to know how many DTRs are in the system
    Arc::SimpleCounter counter;

    /// Create a new Generator
    Generator();
    /// Stop Generator and DTR threads
    ~Generator();

    /// Implementation of callback from DTRCallback
    /**
     * Callback method used when DTR processing is complete to
     * pass back to the generator. The DTR is passed by value so
     * that the scheduler can delete its copy of the object after
     * calling this method.
     */
    virtual void receiveDTR(DTR_ptr dtr);

    /// Start Generator and DTR threads
    void start();

    /// Submit a DTR with given source and destination
    void run(const std::string& source, const std::string& destination);
  };

} // namespace DataStaging

#endif /* GENERATOR_H_ */
