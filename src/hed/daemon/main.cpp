#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <fstream>
#include <signal.h>
#include <arc/ArcConfig.h>
#include <arc/loader/Loader.h>
#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include "daemon.h"
#include "options.h"

#ifdef WIN32
#define NOGDI
#include <windows.h>
#endif

Arc::Daemon *main_daemon;
Arc::Config config;
Arc::Loader *loader;
Arc::Logger& logger=Arc::Logger::rootLogger;

static void shutdown(int)
{
    logger.msg(Arc::DEBUG, "shutdown");
    delete loader;
    delete main_daemon;
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
#ifdef HAVE_GLIBMM_OPTIONS
    Glib::OptionContext opt_ctx;
    opt_ctx.set_help_enabled();
    Arc::ServerOptions options;
    opt_ctx.set_main_group(options);
#else
    Arc::ServerOptions options;
    Arc::ServerOptions& opt_ctx = options;
#endif

    try {
        int status = opt_ctx.parse(argc, argv);
        if (status == 1) {
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
            
            // demonize if the foreground options was not set
            if (!(bool)config["Server"]["Foreground"]) {
                main_daemon = new Arc::Daemon(pid_file, root_log_file);
            } else {
                signal(SIGINT, shutdown);
            }

            // bootstrap
            loader = new Arc::Loader(&config);
            // Arc::Loader loader(&config);
            logger.msg(Arc::INFO, "Service side MCCs are loaded");
            // sleep forever
            for (;;) {
#ifndef WIN32
                sleep(INT_MAX);
#else
		Sleep(INT_MAX);
#endif
            }
        }
    } catch (const Glib::Error& error) {
      logger.msg(Arc::ERROR, error.what());
    }

    return 0;
}
