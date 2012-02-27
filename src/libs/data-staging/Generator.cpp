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

  void Generator::shutdown(int sig) {
    logger.msg(Arc::INFO, "Cancelling all DTRs");
    cond.signal();
  }

  void Generator::receiveDTR(DTR_ptr dtr) {
    logger.msg(Arc::INFO, "Received DTR %s back from scheduler", dtr->get_id());
    // DTR logger destinations can be destroyed when DTR has finished
    dtr->get_logger()->deleteDestinations();
    dtrs.push_back(dtr);
    cond.signal();
  }

  void Generator::run(const std::string& source, const std::string& destination) {

    logger.msg(Arc::INFO, "Generator started");
    signal(SIGINT, shutdown);

    std::string job_id = Arc::UUID();
    Arc::initializeCredentialsType cred_type(Arc::initializeCredentialsType::TryCredentials);
    Arc::UserConfig cfg(cred_type);

    // check credentials
    if (!Arc::Credential::IsCredentialsValid(cfg)) {
      logger.msg(Arc::ERROR, "No valid credentials found, exiting");
      return;
    }

    cfg.UtilsDirPath(Arc::UserConfig::ARCUSERDIRECTORY);

    // Scheduler instance
    Scheduler scheduler;
    //std::vector<Arc::URL> endpoints;
    //endpoints.push_back(Arc::URL("https://localhost:60002/datadeliveryservice"));
    //scheduler.SetDeliveryServices(endpoints);
    // Starting scheduler with default configuration
    scheduler.start();

    {
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
    }
    
    cond.wait();
    logger.msg(Arc::INFO, "Received back DTR %s", dtrs.front()->get_id());
    logger.msg(Arc::INFO, "Generator finished, shutting down scheduler");
    scheduler.stop();
    logger.msg(Arc::INFO, "Scheduler stopped, exiting");
  }

} // namespace DataStaging
