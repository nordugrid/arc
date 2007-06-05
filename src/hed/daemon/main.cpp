#ifdef HAVE_CONFIG
#include <config.h>
#endif

#include "daemon.h"
#include "options.h"
#include <iostream>
#include <glibmm.h>
#include <signal.h>
#include "../../libs/common/ArcConfig.h"
#include "../../hed/libs/loader/Loader.h"

Arc::Daemon *main_daemon;
Arc::Config config;
// Arc::Loader loader;

static void shutdown(int sigint)
{
    delete main_daemon;
}

int main(int argc, char **argv)
{
    // Init thead support
    // Glib::thread_init();
    
    /* Create options parser */
    Glib::OptionContext opt_ctx;
    opt_ctx.set_help_enabled();
    Arc::ArcServerOptions options;
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
            /* XXX: overwrite config variables by cmdline options */

            // set signal handlers 
            signal(SIGTERM, shutdown);

            // XXX: use config instead options
            if (options.foreground == false) {
                main_daemon = new Arc::Daemon(options.pid_file);
            }

            // bootstrap
            // loader = Arc::Loader(&config);
            Arc::Loader loader(&config);
            std::cout << "Service side MCCs are loaded" << std::endl;
            // XXX: How should wait for threads?
            sleep(INT_MAX);
        }
    } catch (const Glib::Error& error) {
        std::cout << error.what() << std::endl;
    }

    return 0;
}
