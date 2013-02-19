#ifndef GENERATOR_H_
#define GENERATOR_H_

#include <arc/Thread.h>
#include <arc/Logger.h>

#include <arc/data-staging/Scheduler.h>


// This Generator basic implementation shows how a Generator can
// be written. It has one method, run(), which creates a single DTR
// and submits it to the Scheduler.
class Generator: public DataStaging::DTRCallback {

 private:

  // Condition to wait on until DTR has finished
  static Arc::SimpleCondition cond;

  // DTR Scheduler
  DataStaging::Scheduler scheduler;

  // Logger object
  static Arc::Logger logger;
  // Root LogDestinations to be used in receiveDTR
  std::list<Arc::LogDestination*> root_destinations;

 public:

  // Counter for main to know how many DTRs are in the system
  Arc::SimpleCounter counter;

  // Create a new Generator. start() must be called to start DTR threads.
  Generator();
  // Stop Generator and DTR threads
  ~Generator();

  // Implementation of callback from DTRCallback - the callback method used
  // when DTR processing is complete to pass the DTR back to the generator.
  // It decrements counter.
  virtual void receiveDTR(DataStaging::DTR_ptr dtr);

  // Start Generator and DTR threads
  void start();

  // Submit a DTR with given source and destination. Increments counter.
  void run(const std::string& source, const std::string& destination);
};

#endif /* GENERATOR_H_ */
