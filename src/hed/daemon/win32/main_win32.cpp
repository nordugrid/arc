#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <signal.h>
#include <arc/ArcConfig.h>
#include <arc/message/MCCLoader.h>
#include <arc/XMLNode.h>
#include <arc/StringConv.h>
#include <arc/Logger.h>
#include "../options.h"

Arc::Config config;
Arc::MCCLoader *loader = NULL;
Arc::Logger& logger=Arc::Logger::rootLogger;
static bool req_shutdown = false;

static void sig_shutdown(int)
{
    if(req_shutdown) _exit(0);
    req_shutdown = true;
}

static void do_shutdown(void)
{
    logger.msg(Arc::VERBOSE, "shutdown");
    if(loader) delete loader;
    _exit(0);
}

static void merge_options_and_config(Arc::Config& cfg, Arc::ServerOptions& opt)
{   
    Arc::XMLNode srv = cfg["Server"];
    if (!(bool)srv) {
      logger.msg(Arc::ERROR, "No server config part of config file");
      return;
    }
    if (opt.pid_file != "") {
        if (!(bool)srv["PidFile"]) {
           srv.NewChild("PidFile")=opt.pid_file;
        } else {
            srv["PidFile"] = opt.pid_file;
        }
    }
    if (opt.foreground == true) {
        if (!(bool)srv["Foreground"]) {
            srv.NewChild("Foreground");
        }
    }
}

static std::string init_logger(Arc::Config& cfg)
{   
    /* setup root logger */
    Arc::LogFile* sd = NULL;
    Arc::XMLNode log = cfg["Server"]["Logger"];
    Arc::XMLNode xlevel = log["Level"];

    Arc::Logger::rootLogger.setThreshold(Arc::WARNING);
    for(;(bool)xlevel;++xlevel) {
      std::string domain = xlevel.Attribute("Domain");
      Arc::LogLevel level = Arc::WARNING;
      if(!string_to_level((std::string)xlevel, level)) {
        logger.msg(Arc::WARNING, "Unknown log level %s", (std::string)xlevel);
      } else {
        Arc::Logger::setThresholdForDomain(level,domain);
      }
    }

    std::string log_file = (log["File"] ? (std::string)log["File"] :  Glib::build_filename(Glib::get_tmp_dir(), "arched.log"));
    sd = new Arc::LogFile(log_file);
    if((!sd) || (!(*sd))) {
      logger.msg(Arc::ERROR, "Failed to open log file: %s", (std::string)log["File"]);
      _exit(1);
    }   
    if(log["Backups"]) {
      int backups;
      if(Arc::stringto((std::string)log["Backups"], backups)) {
	sd->setBackups(backups);
      }   
    }   
    
    if(log["Maxsize"]) {
      int maxsize;
      if(Arc::stringto((std::string)log["Maxsize"], maxsize)) {
	sd->setMaxSize(maxsize);
      }   
    }    
    if(log["Reopen"]) {
      std::string reopen = (std::string)(log["Reopen"]);
      bool reopen_b = false;
      if((reopen == "true") || (reopen == "1")) reopen_b = true;
      sd->setReopen(reopen_b);
    }

    Arc::Logger::rootLogger.removeDestinations();
    if(sd) Arc::Logger::rootLogger.addDestination(*sd);
    if ((bool)cfg["Server"]["Foreground"]) {
      logger.msg(Arc::INFO, "Start foreground");
      Arc::LogStream *err = new Arc::LogStream(std::cerr);
      Arc::Logger::rootLogger.addDestination(*err);
    }
    return log_file;
}

int main(int argc, char **argv)
{
    signal(SIGTTOU, SIG_IGN);
    // Temporary stderr destination for error messages
    Arc::LogStream logcerr(std::cerr);
    Arc::Logger::getRootLogger().addDestination(logcerr);
    /* Create options parser */
    Arc::ServerOptions options;
      
    try {
        std::list<std::string> params = options.Parse(argc, argv);
        if (params.empty()) {
            if (options.version) {
                std::cout << Arc::IString("%s version %s", "arched", VERSION) << std::endl;
                exit(0);
            }

            /* Load and parse config file */
            if(!config.parse(options.xml_config_file.c_str())) {
                logger.msg(Arc::ERROR, "Failed to load service configuration from file %s",options.xml_config_file);
                exit(1);
            };

            if(!MatchXMLName(config,"ArcConfig")) {
              logger.msg(Arc::ERROR, "Configuration root element is not <ArcConfig>");
              exit(1);
            }

            /* overwrite config variables by cmdline options */
            merge_options_and_config(config, options);
            std::string pid_file = (std::string)config["Server"]["PidFile"];
            /* initalize logger infrastucture */
            std::string root_log_file = init_logger(config);
            
            // set signal handlers 
            signal(SIGTERM, sig_shutdown);
            signal(SIGINT, sig_shutdown);

            // bootstrap
            loader = new Arc::MCCLoader(config);
            if(!*loader) {
                logger.msg(Arc::ERROR, "Failed to load service side MCCs");
            } else {
                logger.msg(Arc::INFO, "Service side MCCs are loaded");
                // sleep forever
                for (;!req_shutdown;) {
                    sleep(1);
                }
            }
        } else {
            logger.msg(Arc::ERROR, "Unexpected arguments supplied");
        }
    } catch (const Glib::Error& error) {
      logger.msg(Arc::ERROR, error.what());
    }
    
    do_shutdown();
    return 0;
}
