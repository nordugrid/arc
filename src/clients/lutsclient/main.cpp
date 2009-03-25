#include "arexlutsclient.h"

//TODO cross-platform
#include <signal.h>
#include <errno.h>
#include <unistd.h>

#include <string>
#include <sstream>
#include <map>
#include <list>
#include <iostream>
#include <fstream>

#include <arc/Logger.h>
#include <arc/ArcConfig.h>

#ifdef WIN32
#include <arc/win32.h>
#endif

#include "UsagePublisher.h"


int main(int argc, char **argv)
{

  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);

  //Initialize logger
  Arc::Logger logger(Arc::Logger::rootLogger, "Lutsclient");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::rootLogger.addDestination(logcerr);
  logger.msg(Arc::INFO, 
    "Accounting agent (Lutsclient) started.");

  //Load configuration

  // First command line argument, required, is the config file name
  if (argc<2)
    {
      logger.msg(Arc::FATAL, 
		 "Configuration file path must be given in"
		 " first command line argument!");
      return -4;
    }
  std::string config_file=argv[1];
  logger.msg(Arc::VERBOSE, "Reading configuration file: %s",
	     config_file.c_str());
  Arc::Config config(config_file.c_str());

  //Get config values:

  // Default interval:
  int reporting_interval = AREXLUTSCLIENT_DEFAULT_REPORTING_INTERVAL;

  // Log to file if given in config
  Arc::XMLNode logger_node=config["LutsClient"]["Logger"];

  //  try logging to same file as server if no separate log file given
  if (!logger_node) logger_node=config["Server"]["Logger"];
      
  if (logger_node)
    {
      //  (stolen from hed daemon's init_logger function)
      Arc::LogStream* sd = NULL; 
      std::string log_file = (std::string)logger_node;
      std::string levelstr = (std::string)logger_node.Attribute("level");
      if(!levelstr.empty()) {
	Arc::LogLevel level = Arc::string_to_level(levelstr);
	Arc::Logger::rootLogger.setThreshold(level); 
      }
      if(!log_file.empty()) {
	std::fstream *dest = new std::fstream(log_file.c_str(), 
					      std::fstream::out | 
					      std::fstream::app
					      );
	if(!(*dest)) {
	  logger.msg(Arc::FATAL,"Failed to open log file: %s",log_file);
	  return -5;
	}
	sd = new Arc::LogStream(*dest); 
      }
      logger.msg(Arc::VERBOSE,"Adding log destination: %s",log_file);
      if(sd) Arc::Logger::rootLogger.addDestination(*sd);  
    }

  // Interval from config if present:
  if (Arc::XMLNode reporting_interval_node=config["LutsClient"]["ReportingInterval"])
    {
      std::istringstream stream((std::string)reporting_interval_node);
      if (!(stream>>reporting_interval))
	{
	  logger.msg(Arc::FATAL, 
		     "Could not parse integer value \"ReportingInterval\" "
		     "in config file %s",
		     config_file.c_str()
		     );
	  return -3;
	}
    }

  logger.msg(Arc::VERBOSE, "Reporting interval is: %d s",
	     reporting_interval);

  // The essence:
  Arc::UsagePublisher publisher(config, logger);
  publisher.publish();
  // End of the essence
  
  logger.msg(Arc::INFO, 
	     "Finished logging Usage Records,"
	     " sleeping for %d seconds before exit.",
	     reporting_interval);
  sleep(reporting_interval);
  logger.msg(Arc::INFO, "Exiting.");
  
  return 0;
}
