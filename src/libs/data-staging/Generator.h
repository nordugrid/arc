#ifndef GENERATOR_H_
#define GENERATOR_H_

/**
 * The real generator will most likely be part of A-REX, and the
 * entry point into data staging code will be the scheduler.
 * This code is mainly for independent testing of the data staging
 * framework and will not be in the released version.
 */

#include <arc/Thread.h>
#include <arc/Logger.h>

#include "DTR.h"

namespace DataStaging {

  /**
   * Generator is a singleton class. It generates DTRs and submits
   * them to the data staging system, then waits for them to complete.
   */
  class Generator: public DTRCallback {

   private:

    /** Condition to wait on until DTR has finished */
    static Arc::SimpleCondition cond;

    /** Singleton instance */
    static Generator* instance;

    /** Private constructors and assignment operators */
    Generator() {};
    ~Generator() {};
    Generator(const Generator&);
    Generator& operator=(const Generator&);

    /** Interrupt signal handler */
    static void shutdown(int sig);

    /** Logger object */
    static Arc::Logger logger;

   public:

    /** Get the singleton instance */
    static Generator* getInstance();

    /**
     * Callback method used when DTR processing is complete to
     * pass back to the generator. The DTR is passed by value so
     * that the scheduler can delete its copy of the object after
     * calling this method.
     */
    virtual void receive_dtr(DTR dtr);

    /** Produce and submit some DTRs, with given source and destination */
    void run(const std::string& source, const std::string& destination);
  };

} // namespace DataStaging

#endif /* GENERATOR_H_ */
