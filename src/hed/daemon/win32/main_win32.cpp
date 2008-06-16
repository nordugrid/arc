#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef WIN32
#include <arc/win32.h>
#endif

#include <fstream>
#include <signal.h>
#include <arc/ArcConfig.h>
#include <arc/loader/Loader.h>
#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include "../options.h"

Arc::Config config;
Arc::Loader *loader;
Arc::Logger& logger=Arc::Logger::rootLogger;

static void shutdown(int)
{
    logger.msg(Arc::DEBUG, "shutdown");
    delete loader;
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
    Arc::XMLNode log = cfg["Server"]["Logger"];
    std::string log_file = (std::string)log;
    std::string str = (std::string)log.Attribute("level");
    Arc::LogLevel level = Arc::string_to_level(str);
    Arc::Logger::rootLogger.setThreshold(level); 
    std::fstream *dest = new std::fstream(log_file.c_str(), std::fstream::out | std::fstream::app);
    if(!(*dest)) {
      std::cerr<<"Failed to open log file: "<<log_file<<std::endl;
      _exit(1);
    }
    Arc::LogStream* sd = new Arc::LogStream(*dest); 
    Arc::Logger::rootLogger.addDestination(*sd);
    if ((bool)cfg["Server"]["Foreground"]) {
      logger.msg(Arc::INFO, "Start foreground");
      Arc::LogStream *err = new Arc::LogStream(std::cerr);
      Arc::Logger::rootLogger.addDestination(*err);
    }
    return log_file;
}

int main(int argc, char **argv)
{
    signal(SIGTTOU,SIG_IGN);
    /* Create options parser */
    Arc::ServerOptions options;
      
    try {
        std::list<std::string> params = options.Parse(argc, argv);
        if (params.size() == 0) {
            /* Load and parse config file */
            config.parse(options.config_file.c_str());
            if(!config) {
                logger.msg(Arc::ERROR, "Failed to load service configuration");
                exit(1);
            };
            if(!MatchXMLName(config,"ArcConfig")) {
              logger.msg(Arc::ERROR, "Configuration root element is not ArcConfig");
              exit(1);
            }

            /* overwrite config variables by cmdline options */
            merge_options_and_config(config, options);
            std::string pid_file = (std::string)config["Server"]["PidFile"];
            /* initalize logger infrastucture */
            std::string root_log_file = init_logger(config);

            // set signal handlers 
            signal(SIGTERM, shutdown);
            signal(SIGINT, shutdown);

            // bootstrap
            loader = new Arc::Loader(&config);
            // Arc::Loader loader(&config);
            logger.msg(Arc::INFO, "Service side MCCs are loaded");
            // sleep forever
            for (;;) {
                sleep(INT_MAX/1000);
            }
        }
    } catch (const Glib::Error& error) {
      logger.msg(Arc::ERROR, error.what());
    }
    
    return 0;
}
