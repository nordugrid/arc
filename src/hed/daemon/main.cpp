#ifdef HAVE_CONFIG
#include <config.h>
#endif

#include "daemon.h"
#include "options.h"
#include <iostream>
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
Arc::Logger &l = Arc::Logger::rootLogger;

static void shutdown(int sigint)
{
    l.msg(Arc::DEBUG, "shutdown");
    delete loader;
    delete main_daemon;
    _exit(0);
}

static void merge_options_and_config(Arc::Config& cfg, Arc::ServerOptions& opt)
{   
    Arc::XMLNode srv = cfg["ArcConfig"]["Server"];
    if (!(bool)srv) {
        std::cout << "No server config part of config file" << std::endl;
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
        std::cout << "Start foreground" << std::endl;
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
                std::cerr << "Failed to load service configuration" << std::endl;
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
            l.msg(Arc::INFO, "Service side MCCs are loaded");
            // sleep forever
            for (;;) {
                sleep(INT_MAX);
            }
        }
    } catch (const Glib::Error& error) {
        std::cout << error.what() << std::endl;
    }

    return 0;
}
