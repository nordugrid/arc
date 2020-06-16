#include <arc/credential/Credential.h>
#include <arc/GUID.h>
#include "Generator.h"

Arc::Logger Generator::logger(Arc::Logger::getRootLogger(), "Generator");
Arc::SimpleCondition Generator::cond;

Generator::Generator() {
  // Set up logging
  root_destinations = Arc::Logger::getRootLogger().getDestinations();
  DataStaging::DTR::LOG_LEVEL = Arc::Logger::getRootLogger().getThreshold();
}

Generator::~Generator() {
  logger.msg(Arc::INFO, "Shutting down scheduler");
  scheduler.stop();
  logger.msg(Arc::INFO, "Scheduler stopped, exiting");
}

void Generator::receiveDTR(DataStaging::DTR_ptr dtr) {
  // root logger is disabled in Scheduler thread so need to add it here
  Arc::Logger::getRootLogger().addDestinations(root_destinations);
  logger.msg(Arc::INFO, "Received DTR %s back from scheduler in state %s", dtr->get_id(), dtr->get_status().str());
  Arc::Logger::getRootLogger().removeDestinations();
  counter.dec();
}

void Generator::start() {
  // Starting scheduler with default configuration
  logger.msg(Arc::INFO, "Generator started");
  logger.msg(Arc::INFO, "Starting DTR threads");
  scheduler.SetDumpLocation("/tmp/dtr.log");
  scheduler.start();
}

void Generator::run(const std::string& source, const std::string& destination) {

  std::string job_id = Arc::UUID();
  Arc::initializeCredentialsType cred_type(Arc::initializeCredentialsType::TryCredentials);
  Arc::UserConfig cfg(cred_type);

  // check credentials
  if (!Arc::Credential::IsCredentialsValid(cfg)) {
    logger.msg(Arc::ERROR, "No valid credentials found, exiting");
    return;
  }

  cfg.UtilsDirPath(Arc::UserConfig::ARCUSERDIRECTORY());

  std::list<DataStaging::DTRLogDestination> logs;
  logs.push_back(new Arc::LogStream(std::cout));

  DataStaging::DTR_ptr dtr(new DataStaging::DTR(source, destination, cfg, job_id,  Arc::User().get_uid(), logs, "DataStaging"));
  if (!(*dtr)) {
    logger.msg(Arc::ERROR, "Problem creating dtr (source %s, destination %s)", source, destination);
    return;
  }
  // register callback with DTR
  dtr->registerCallback(this,DataStaging::GENERATOR);
  dtr->registerCallback(&scheduler,DataStaging::SCHEDULER);
  dtr->set_tries_left(5);
  DataStaging::DTR::push(dtr, DataStaging::SCHEDULER);
  counter.inc();
}
