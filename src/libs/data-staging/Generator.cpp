#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <signal.h>

#include <arc/GUID.h>

#include "Scheduler.h"

#include "Generator.h"

namespace DataStaging {

  Arc::Logger Generator::logger(Arc::Logger::getRootLogger(), "DataStaging.Generator");
  //Generator* Generator::instance = NULL;
  //Arc::SimpleCondition Generator::cond;

  //Generator* Generator::getInstance() {
  //  if (!instance)
  //    instance = new Generator();
  //  return instance;
  //}

  void Generator::shutdown(int sig) {
    logger.msg(Arc::INFO, "Cancelling all DTRs");
//    cond.signal();
  }

  void Generator::receiveDTR(DTR& dtr) {
    logger.msg(Arc::INFO, "Received DTR %s back from scheduler", dtr.get_id());
    // DTR logger can be destroyed when DTR has finished
    //dtr.get_logger()->deleteDestinations();
    delete dtr.get_logger();
    cond.signal();
  }

  void Generator::run(const std::string& source, const std::string& destination) {

    logger.msg(Arc::INFO, "Generator started");
    signal(SIGINT, shutdown);
    // set up job and session dir
    std::string job_id = Arc::UUID();
    std::string session_dir = "/var/tmp/arc/session";
    Arc::UserConfig cfg;

    // check credentials
    if (!Arc::Credential::IsCredentialsValid(cfg)) {
      logger.msg(Arc::ERROR, "No valid credentials found, exiting");
      return;
    }

    cfg.UtilsDirPath(Arc::UserConfig::ARCUSERDIRECTORY);

    // Scheduler instance
    Scheduler scheduler;
    // Starting scheduler with default configuration
    scheduler.start();

    {
      Arc::Logger * log = new Arc::Logger(Arc::Logger::getRootLogger(), "DataStaging");
      Arc::LogDestination * dest = new Arc::LogStream(std::cerr);
      log->addDestination(*dest);
      //log->detach();

      // Free DTR immediately after passing to scheduler
      DTR dtr(source, destination, cfg, job_id, getuid(), log);
      if (!dtr) {
        logger.msg(Arc::ERROR, "Problem creating dtr (source %s, destination %s)", source, destination);
        return;
      }
      // register callback with DTR
      dtr.registerCallback(this,GENERATOR);
      dtr.registerCallback(&scheduler,SCHEDULER);
      dtr.push(SCHEDULER);
    }
    //Scheduler::getInstance()->cancel_dtrs(job_desc);
    
    cond.wait();
    logger.msg(Arc::INFO, "Generator finished, shutting down scheduler");
    scheduler.stop();
    logger.msg(Arc::INFO, "Scheduler stopped, exiting");
  }

} // namespace DataStaging
