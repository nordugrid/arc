#ifdef HAVE_CONFIG
#include <config.h>
#endif

#include "daemon.h"
#include "options.h"
#include <fstream>
#include <glibmm.h>
#include <signal.h>
#include "../../hed/libs/loader/Loader.h"
#include "../../libs/common/ArcConfig.h"
#include "../../libs/common/XMLNode.h"
#include "../../libs/common/Logger.h"

Arc::Daemon *main_daemon;
Arc::Config config;
Arc::Loader *loader;
Arc::Logger& logger=Arc::Logger::rootLogger;

static void shutdown(int sigint)
{
    logger.msg(Arc::DEBUG, "shutdown");
    delete loader;
    delete main_daemon;
    _exit(0);
}

static void merge_options_and_config(Arc::Config& cfg, Arc::ServerOptions& opt)
{   
    Arc::XMLNode srv = cfg["ArcConfig"]["Server"];
    if (!(bool)srv) {
      logger.msg(Arc::ERROR, "No server config part of config file");
      return;
    }
    if (opt.pid_file != "") {
        if (!(bool)srv["PidFile"]) {
           srv.NewChild("Pidfile")=opt.pid_file;
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
    Arc::XMLNode log = cfg["ArcConfig"]["Server"]["Logger"];
    std::string log_file = (std::string)log;
    std::string level = (std::string)log.Attribute("level");
    Arc::Logger::rootLogger.setThreshold(Arc::string_to_level(level)); 
    std::fstream *dest = new std::fstream(log_file.c_str(), std::fstream::app);
    Arc::LogStream* sd = new Arc::LogStream(*dest); 
    Arc::Logger::rootLogger.addDestination(*sd);
    if ((bool)cfg["ArcConfig"]["Server"]["Foreground"]) {
      logger.msg(Arc::INFO, "Start foreground");
      Arc::LogStream *err = new Arc::LogStream(std::cerr);
      Arc::Logger::rootLogger.addDestination(*err);
    }
    return log_file;
}

int main(int argc, char **argv)
{
    /* Create options parser */
    Glib::OptionContext opt_ctx;
    opt_ctx.set_help_enabled();
    Arc::ServerOptions options;
    opt_ctx.set_main_group(options);

    try {
        int status = opt_ctx.parse(argc, argv);
        if (status == 1) {
            /* Load and parse config file */
            config.parse(options.config_file.c_str());
            if(!config) {
	      logger.msg(Arc::ERROR, "Failed to load service configuration");
	      exit(1);
            };

            /* overwrite config variables by cmdline options */
            merge_options_and_config(config, options);
            std::string pid_file = (std::string)config["ArcServer"]["Server"]["PidFile"];
            /* initalize logger infrastucture */
            std::string root_log_file = init_logger(config);

            // set signal handlers 
            signal(SIGTERM, shutdown);
            
            // demonize if the foreground options was not set
            if (!(bool)config["ArcConfig"]["Server"]["Foreground"]) {
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
                sleep(INT_MAX);
            }
        }
    } catch (const Glib::Error& error) {
      logger.msg(Arc::ERROR, error.what().c_str());
    }

    return 0;
}
