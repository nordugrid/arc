#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>
#include <errno.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>

#include <arc/ArcConfig.h>
#include <arc/IniConfig.h>
#include <arc/ArcLocation.h>
#include <arc/DateTime.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/StringConv.h>
#include <arc/message/MCCLoader.h>

#include "daemon.h"
#include "../options.h"

static Arc::Daemon *main_daemon = NULL;
static Arc::Config config;
static Arc::MCCLoader *loader = NULL;
static Arc::Logger& logger = Arc::Logger::rootLogger;
static int exit_code = 0;
static bool req_shutdown = false;

static void sig_shutdown(int)
{
    if(req_shutdown) _exit(exit_code);
    req_shutdown = true;
}

static void do_shutdown(void)
{
    if(main_daemon) main_daemon->shutdown();
    logger.msg(Arc::VERBOSE, "shutdown");
    if(loader) delete loader;
    if(main_daemon) delete main_daemon;
    logger.msg(Arc::DEBUG, "exit");
    _exit(exit_code);
}

static void sighup_handler(int) {
    int old_errno = errno;
    if(main_daemon) main_daemon->logreopen();
    Arc::LogFile::ReopenAll();
    errno = old_errno;
}

static void daemon_kick(Arc::Daemon*)
{
  // Reopen log file(s) which may have been archived by now.
  sighup_handler(SIGHUP);
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
            srv.NewChild("Foreground")="true";
        } else {
            srv["Foreground"]="true";
        }
    }

    if (opt.watchdog == true) {
        if (!(bool)srv["Watchdog"]) {
            srv.NewChild("Watchdog")="true";
        } else {
            srv["Watchdog"]="true";
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

    if (!opt.log_file.empty()) {
        if (!(bool)srv["Logger"]["File"]) {
            srv.NewChild("Logger").NewChild("File") = opt.log_file;
        } else {
            srv["Logger"]["File"] = opt.log_file;
        }
    }
}

static bool is_true(Arc::XMLNode val) {
  std::string v = (std::string)val;
  if(v == "true") return true;
  if(v == "1") return true;
  return false;
}

static std::string init_logger(Arc::XMLNode log, bool foreground)
{
    // set up root logger(s)
    std::list<Arc::LogFile*> dests;
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

    std::string default_log;
    for (Arc::XMLNode logfile = log["File"]; logfile; ++logfile) {
      Arc::LogFile* l = new Arc::LogFile((std::string)logfile);
      if((!l) || (!(*l))) {
        logger.msg(Arc::ERROR, "Failed to open log file: %s", (std::string)logfile);
        _exit(1);
      }
      dests.push_back(l);
      if (default_log.empty()) default_log = (std::string)logfile;
    }
    if (dests.empty()) {
      Arc::LogFile* l = new Arc::LogFile("/var/log/arc/arched.log");
      dests.push_back(l);
      default_log = "/var/log/arc/arched.log";
    }

    int backups = -1;
    if (log["Backups"]) Arc::stringto((std::string)log["Backups"], backups);

    int maxsize = -1;
    if (log["Maxsize"]) Arc::stringto((std::string)log["Maxsize"], maxsize);

    bool reopen_b = false;
    if (log["Reopen"]) {
      std::string reopen = (std::string)(log["Reopen"]);
      if((reopen == "true") || (reopen == "1")) reopen_b = true;
    }

    Arc::Logger::rootLogger.removeDestinations();
    for (std::list<Arc::LogFile*>::iterator i = dests.begin(); i != dests.end(); ++i) {
      (*i)->setBackups(backups);
      (*i)->setMaxSize(maxsize);
      (*i)->setReopen(reopen_b);
      Arc::Logger::rootLogger.addDestination(**i);
    }
    if (foreground) {
      logger.msg(Arc::INFO, "Start foreground");
      Arc::LogStream *err = new Arc::LogStream(std::cerr);
      Arc::Logger::rootLogger.addDestination(*err);
    }
    if (reopen_b) return "";
    return default_log;
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

static gid_t get_gid(uid_t uid)
{
    struct passwd *ent;
    if (!(ent = getpwuid(uid))) {
        std::cerr << "Bad user id" << std::endl;
        exit(1);
    }
    return (ent->pw_gid);
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
        if(!config.parse(options.xml_config_file.c_str())) {
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
                if(!config.parse(xml_config_file.c_str())) {
                    logger.msg(Arc::ERROR, "Error loading generated configuration");
                    exit(1);
                }
        } else {
            Arc::IniConfig ini_parser(ini_config_file);
            if (ini_parser.Evaluate(config) == false) {
                logger.msg(Arc::ERROR, "Error evaluating profile");
                exit(1);
            }
        }
        if (config.Size() == 0) {
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
    signal(SIGHUP,&sighup_handler);
    // Set up Glib exception handler
    Glib::add_exception_handler(sigc::ptr_fun(&glib_exception_handler));
    // Debug code for setting different logging formats
    char const * log_time_format = ::getenv("ARC_LOGGER_TIME_FORMAT");
    if(log_time_format) {
        if(strcmp(log_time_format,"USER") == 0) {
            Arc::Time::SetFormat(Arc::UserTime);
        } else if(strcmp(log_time_format,"USEREXT") == 0) {
            Arc::Time::SetFormat(Arc::UserExtTime);
        } else if(strcmp(log_time_format,"ELASTIC") == 0) {
            Arc::Time::SetFormat(Arc::ElasticTime);
        } else if(strcmp(log_time_format,"MDS") == 0) {
            Arc::Time::SetFormat(Arc::MDSTime);
        } else if(strcmp(log_time_format,"ASC") == 0) {
            Arc::Time::SetFormat(Arc::ASCTime);
        } else if(strcmp(log_time_format,"ISO") == 0) {
            Arc::Time::SetFormat(Arc::ISOTime);
        } else if(strcmp(log_time_format,"UTC") == 0) {
            Arc::Time::SetFormat(Arc::UTCTime);
        } else if(strcmp(log_time_format,"RFC1123") == 0) {
            Arc::Time::SetFormat(Arc::RFC1123Time);
        } else if(strcmp(log_time_format,"EPOCH") == 0) {
            Arc::Time::SetFormat(Arc::EpochTime);
        };
    };
    // Temporary stderr destination for error messages
    Arc::LogStream logcerr(std::cerr);
    Arc::Logger::getRootLogger().addDestination(logcerr);
    /* Create options parser */
    Arc::ServerOptions options;

    if((argc>0) && (argv[0])) Arc::ArcLocation::Init(argv[0]);

    try {
        std::list<std::string> params = options.Parse(argc, argv);
        if (params.empty()) {
            if (options.version) {
                std::cout << Arc::IString("%s version %s", "arched", VERSION) << std::endl;
                exit(0);
            }

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
            std::string pid_file = (config["Server"]["PidFile"] ? (std::string)config["Server"]["PidFile"] : "/run/arched.pid");
            std::string user = (std::string)config["Server"]["User"];
            std::string group = (std::string)config["Server"]["Group"];

            // switch user
            if (getuid() == 0) { // are we root?
                /* switch group if it is specified */
                if (!group.empty()) {
                    gid_t g = get_gid(group);
                    if (setgid(g) != 0) {
                        logger.msg(Arc::ERROR, "Cannot switch to group (%s)", group);
                        exit(1);
                    }
                }
                /* switch user if it is specified */
                if (!user.empty()) {
                    uid_t u = get_uid(user);
                    if (group.empty()) {
                        gid_t g = get_gid(u);
                        if (setgid(g) != 0) {
                            logger.msg(Arc::ERROR, "Cannot switch to primary group for user (%s)", user);
                            exit(1);
                        }
                    }
                    if (setuid(u) != 0) {
                        logger.msg(Arc::ERROR, "Cannot switch to user (%s)", user);
                        exit(1);
                    }
                }
            }
            /* initalize logger infrastucture */
            std::string root_log_file = init_logger(config["Server"]["Logger"], is_true(config["Server"]["Foreground"]));
            // demonize if the foreground option was not set
            if (!is_true((config)["Server"]["Foreground"])) {
                main_daemon = new Arc::Daemon(pid_file, root_log_file, is_true((config)["Server"]["Watchdog"]), &daemon_kick);
            }
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

    if (!req_shutdown) exit_code = -1;
    do_shutdown();
    return exit_code;
}
