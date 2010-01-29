#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef WIN32
#include <arc/win32.h>
#endif

#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>

#include <arc/ArcConfig.h>
#include <arc/IniConfig.h>
#include <arc/ArcLocation.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/StringConv.h>
#include <arc/message/MCCLoader.h>

#include "daemon.h"
#include "../options.h"

Arc::Daemon *main_daemon = NULL;
Arc::Config config;
Arc::MCCLoader *loader = NULL;
Arc::Logger& logger = Arc::Logger::rootLogger;
int exit_code = 0;

static void shutdown(int)
{
    logger.msg(Arc::VERBOSE, "shutdown");
    if(loader) delete loader;
    if(main_daemon) delete main_daemon;
    logger.msg(Arc::DEBUG, "exit");
    _exit(exit_code);
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

    if (opt.user != "") {
        if (!(bool)srv["User"]) {
            srv.NewChild("User") = opt.user;
        } else {
            srv["User"] = opt.user;
        }
    }

    if (opt.group != "") {
        if (!(bool)srv["Group"]) {
            srv.NewChild("Group") = opt.group;
        } else {
            srv["Group"] = opt.group;
        }
    }
}

static std::string init_logger(Arc::XMLNode log, bool foreground)
{
    /* setup root logger */
    Arc::LogFile* sd = NULL;
    Arc::LogLevel level = Arc::WARNING;
    if((bool)log["Level"] && !string_to_level((std::string)log["Level"], level)) {
      logger.msg(Arc::WARNING, "Unknown log level %s", (std::string)log["Level"]);
    }
    Arc::Logger::rootLogger.setThreshold(level);

    std::string log_file = (log["File"] ? (std::string)log["File"] : "/var/log/arched.log");
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
    Arc::Logger::rootLogger.removeDestinations();
    Arc::Logger::rootLogger.addDestination(*sd);
    if (foreground) {
      logger.msg(Arc::INFO, "Start foreground");
      Arc::LogStream *err = new Arc::LogStream(std::cerr);
      Arc::Logger::rootLogger.addDestination(*err);
    }
    return log_file;
}

static uid_t get_uid(const std::string &name)
{
    struct passwd *ent;
    if (name[0] == '#') {
        return (atoi(&(name.c_str()[1])));
    }
    if (!(ent = getpwnam(name.c_str()))) {
        std::cerr << "Bad user name" << std::endl;
        exit(1);
    }
    return (ent->pw_uid);
}

static gid_t get_gid(const std::string &name)
{
    struct group *ent;
    if (name[0] == '#') {
        return (atoi(&(name.c_str()[1])));
    }
    if (!(ent = getgrnam(name.c_str()))) {
        std::cerr << "Bad user name" << std::endl;
        exit(1);
    }
    return (ent->gr_gid);
}

static void init_config(const Arc::ServerOptions &options)
{
    if (!options.xml_config_file.empty()) {
        if (Glib::file_test(options.xml_config_file,
            Glib::FILE_TEST_EXISTS) == false) {
            logger.msg(Arc::ERROR, "XML config file %s does not exist", options.xml_config_file);
            exit(1);
        }
        config.parse(options.xml_config_file.c_str());
        if (!config) {
            logger.msg(Arc::ERROR, "Failed to load service configuration from file %s", options.xml_config_file);
            exit(1);
        }
    } else if (!options.ini_config_file.empty()) {
        if (Glib::file_test(options.ini_config_file,
            Glib::FILE_TEST_EXISTS) == false) {
            logger.msg(Arc::ERROR, "INI config file %s does not exist", options.xml_config_file);
            exit(1);
        }
        Arc::IniConfig ini_parser(options.ini_config_file);
        if (ini_parser.Evaluate(config) == false) {
            logger.msg(Arc::ERROR, "Error evaluating profile");
            exit(1);
        }
        if (!config) {
            logger.msg(Arc::ERROR, "Failed to load service configuration from file %s", options.ini_config_file);
            exit(1);
        }
    } else {
        std::string ini_config_file = "/etc/arc/service.ini";
        if (Glib::file_test(ini_config_file,
            Glib::FILE_TEST_EXISTS) == false) {
                std::string xml_config_file = "/etc/arc/service.xml";
                if (Glib::file_test(xml_config_file,
                    Glib::FILE_TEST_EXISTS) == false) {
                }
                config.parse(xml_config_file.c_str());
        } else {
            Arc::IniConfig ini_parser(ini_config_file);
            ini_parser.Evaluate(config);
            if (ini_parser.Evaluate(config) == false) {
                logger.msg(Arc::ERROR, "Error evaulating profile");
                exit(1);
            }
        }
        if (!config) {
            logger.msg(Arc::ERROR, "Failed to load service configuration from any default config file");
            exit(1);
        }
    }
}

int main(int argc, char **argv)
{
    // Ignore some signals
    signal(SIGTTOU,SIG_IGN);
    signal(SIGPIPE,SIG_IGN);
    // Temporary stderr destination for error messages
    Arc::LogStream logcerr(std::cerr);
    Arc::Logger::getRootLogger().addDestination(logcerr);
    /* Create options parser */
    Arc::ServerOptions options;

    if((argc>0) && (argv[0])) Arc::ArcLocation::Init(argv[0]);

    try {
        std::list<std::string> params = options.Parse(argc, argv);
        if (params.size() == 0) {
            /* Load and parse config file */
            init_config(options);

            // schema validation
            if (!options.schema_file.empty()) {
                std::string err_msg;
                bool ret = config.Validate(options.schema_file, err_msg);
                if (ret == false) {
                    logger.msg(Arc::ERROR, "Schema validation error");
                    logger.msg(Arc::ERROR, err_msg);
                    exit(1);
                }
            }

            // dump config if it was requested
            if (options.config_dump == true) {
                std::string str;
                config.GetXML(str, true);
                std::cout << Arc::strip(str) << std::endl;
                exit(0);
            }

            if(!MatchXMLName(config,"ArcConfig")) {
                logger.msg(Arc::ERROR, "Configuration root element is not <ArcConfig>");
                exit(1);
            }

            /* overwrite config variables by cmdline options */
            merge_options_and_config(config, options);
            std::string pid_file = (config["Server"]["PidFile"] ? (std::string)config["Server"]["PidFile"] : "/var/run/arched.pid");
            /* initalize logger infrastucture */
            std::string root_log_file = init_logger(config["Server"]["Logger"], config["Server"]["Foreground"]);
            std::string user = (std::string)config["Server"]["User"];
            std::string group = (std::string)config["Server"]["Group"];
            // set signal handlers
            signal(SIGTERM, shutdown);
            signal(SIGINT, shutdown);

            // switch user
            if (getuid() == 0) { // are we root?
                /* switch group it is specified */
                if (!group.empty()) {
                    gid_t g = get_gid(group);
                    if (setgid(g) != 0) {
                        logger.msg(Arc::ERROR, "Cannot switch to group (%s)", group);
                        exit(1);
                    }
                }
                /* switch user if it is specied */
                if (!user.empty()) {
                    uid_t u = get_uid(user);
                    if (setuid(u) != 0) {
                        logger.msg(Arc::ERROR, "Cannot switch to user (%s)", user);
                        exit(1);
                    }
                }
            }
            // demonize if the foreground options was not set
            if (!(bool)(config)["Server"]["Foreground"]) {
                main_daemon = new Arc::Daemon(pid_file, root_log_file);
            }

            // bootstrap
            loader = new Arc::MCCLoader(config);
            logger.msg(Arc::INFO, "Service side MCCs are loaded");
            // sleep forever
            for (;;) {
                sleep(INT_MAX);
            }
        }
    } catch (const Glib::Error& error) {
      logger.msg(Arc::ERROR, error.what());
    }

    exit_code = -1;
    shutdown(0);
    return exit_code;
}
