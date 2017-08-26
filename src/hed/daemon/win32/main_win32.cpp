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

static void glib_exception_handler()
{
  std::cerr << "Glib exception thrown" << std::endl;
  try {
    throw;
  }
  catch(const Glib::Error& err) {
    std::cerr << "Glib exception caught: " << err.what() << std::endl;
  }
  catch(const std::exception &err) {
    std::cerr << "Exception caught: " << err.what() << std::endl;
  }
  catch(...) {
    std::cerr << "Unknown exception caught" << std::endl;
  }
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
            srv.NewChild("Foreground") = "true";
        } else {
            srv["Foreground"] = "true";
        }
    }
}

static bool is_true(Arc::XMLNode val) {
  std::string v = (std::string)val;
  if(v == "true") return true;
  if(v == "1") return true;
  return false;
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
      if (!istring_to_level((std::string)xlevel, level)) {
        unsigned int ilevel;
        if (Arc::stringto((std::string)xlevel, ilevel)) {
          level = Arc::old_level_to_level(ilevel);
        } else {
          logger.msg(Arc::WARNING, "Unknown log level %s", (std::string)xlevel);
        }
      }
      Arc::Logger::setThresholdForDomain(level,domain);
    }

    std::string log_file = (log["File"] ? (std::string)log["File"] :  Glib::build_filename(Glib::get_tmp_dir(), "arched.log"));
    sd = new Arc::LogFile(log_file);
    if((!sd) || (!(*sd))) {
      logger.msg(Arc::ERROR, "Failed to open log file: %s", log_file);
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
    if (is_true(cfg["Server"]["Foreground"])) {
      logger.msg(Arc::INFO, "Start foreground");
      Arc::LogStream *err = new Arc::LogStream(std::cerr);
      Arc::Logger::rootLogger.addDestination(*err);
    }
    return log_file;
}

int main(int argc, char **argv)
{
    signal(SIGTTOU, SIG_IGN);
    // Set up Glib exception handler
    Glib::add_exception_handler(sigc::ptr_fun(&glib_exception_handler));
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
