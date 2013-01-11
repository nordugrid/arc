#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <signal.h>

#include <arc/GUID.h>

#include "Scheduler.h"

#include "Generator.h"

namespace DataStaging {

  Arc::Logger Generator::logger(Arc::Logger::getRootLogger(), "DataStaging.Generator");
  Arc::SimpleCondition Generator::cond;

  Generator::Generator() {
    // Set up logging
    root_destinations = Arc::Logger::getRootLogger().getDestinations();
    DTR::LOG_LEVEL = Arc::Logger::getRootLogger().getThreshold();
  }

  Generator::~Generator() {
    logger.msg(Arc::INFO, "Shutting down scheduler");
    scheduler.stop();
    logger.msg(Arc::INFO, "Scheduler stopped, exiting");
  }

  void Generator::receiveDTR(DTR_ptr dtr) {
    // root logger is disabled in Scheduler thread so need to add it here
    Arc::Logger::getRootLogger().addDestinations(root_destinations);
    logger.msg(Arc::INFO, "Received DTR %s back from scheduler in state %s", dtr->get_id(), dtr->get_status().str());
    Arc::Logger::getRootLogger().removeDestinations();
    // DTR logger destinations can be destroyed when DTR has finished
    dtr->get_logger()->deleteDestinations();
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

    cfg.UtilsDirPath(Arc::UserConfig::ARCUSERDIRECTORY);

    DTRLogger log(new Arc::Logger(Arc::Logger::getRootLogger(), "DataStaging"));
    Arc::LogDestination * dest = new Arc::LogStream(std::cerr);
    log->addDestination(*dest);

    // Free DTR immediately after passing to scheduler
    DTR_ptr dtr(new DTR(source, destination, cfg, job_id,  Arc::User().get_uid(), log));
    if (!(*dtr)) {
      logger.msg(Arc::ERROR, "Problem creating dtr (source %s, destination %s)", source, destination);
      return;
    }
    // register callback with DTR
    dtr->registerCallback(this,GENERATOR);
    dtr->registerCallback(&scheduler,SCHEDULER);
    dtr->set_tries_left(5);
    DTR::push(dtr, SCHEDULER);
    counter.inc();
  }

} // namespace DataStaging
