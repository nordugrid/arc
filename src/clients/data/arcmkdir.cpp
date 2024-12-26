// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <list>

#include <unistd.h>

#include <arc/ArcLocation.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/credential/Credential.h>
#include <arc/data/DataHandle.h>
#include <arc/loader/FinderLoader.h>
#include <arc/OptionParser.h>

#include "utils.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(), "arcmkdir");

bool arcmkdir(const Arc::URL& file_url,
              Arc::UserConfig& usercfg,
              bool with_parents) {
  if (!file_url) {
    logger.msg(Arc::ERROR, "Invalid URL: %s", file_url.str());
    return false;
  }
  if (file_url.Protocol() == "urllist") {
    std::list<Arc::URL> files = Arc::ReadURLList(file_url);
    if (files.empty()) {
      logger.msg(Arc::ERROR, "Can't read list of locations from file %s",
                 file_url.Path());
      return false;
    }
    bool r = true;
    for (std::list<Arc::URL>::iterator file = files.begin();
         file != files.end(); ++file) {
      if (!arcmkdir(*file, usercfg, with_parents))
        r = false;
    }
    return r;
  }

  Arc::DataHandle url(file_url, usercfg);
  if (!url) {
    logger.msg(Arc::ERROR, "Unsupported URL given");
    return false;
  }
  if (url->RequiresCredentials()) {
   if(!initProxy(logger, usercfg, file_url)) return false;
  }
  url->SetSecure(false);
  Arc::DataStatus res = url->CreateDirectory(with_parents);
  if (!res.Passed()) {
    logger.msg(Arc::ERROR, std::string(res));
    if (res.Retryable())
      logger.msg(Arc::ERROR, "This seems like a temporary error, please try again later");
    return false;
  }
  return true;
}

static int runmain(int argc, char **argv) {

  setlocale(LC_ALL, "");

  static Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("url"),
                            istring("The arcmkdir command creates directories "
                                    "on grid storage elements and catalogs."));

  bool with_parents = false;
  options.AddOption('p', "parents",
                    istring("make parent directories as needed"),
                    with_parents);

  bool show_plugins = false;
  options.AddOption('P', "listplugins",
                    istring("list the available plugins (protocols supported)"),
                    show_plugins);

  int timeout = 20;
  options.AddOption('t', "timeout", istring("timeout in seconds (default 20)"),
                    istring("seconds"), timeout);

  std::string conffile;
  options.AddOption('z', "conffile",
                    istring("configuration file (default ~/.arc/client.conf)"),
                    istring("filename"), conffile);

  bool no_authentication = false;
  options.AddOption('\0', "no-authentication",
              istring("do not perform any authentication for opened connections"),
              no_authentication);

  bool x509_authentication = false;
  options.AddOption('\0', "x509-authentication",
              istring("perform X.509 authentication for opened connections"),
              x509_authentication);

  bool token_authentication = false;
  options.AddOption('\0', "token-authentication",
              istring("perform token authentication for opened connections"),
              token_authentication);

  bool force_system_ca = false;
  options.AddOption('\0', "systemca",
              istring("force using CA certificates configuration provided by OpenSSL"),
              force_system_ca);

  bool force_grid_ca = false;
  options.AddOption('\0', "gridca",
              istring("force using CA certificates configuration for Grid services (typically IGTF)"),
              force_grid_ca);
    
  bool allow_insecure_connection = false;
  options.AddOption('\0', "allowinsecureconnection",
              istring("allow TLS connection which failed verification"),
              allow_insecure_connection);

  std::string debug;
  options.AddOption('d', "debug",
                    istring("FATAL, ERROR, WARNING, INFO, VERBOSE or DEBUG"),
                    istring("debuglevel"), debug);

  bool version = false;
  options.AddOption('v', "version", istring("print version information"),
                    version);

  std::list<std::string> params = options.Parse(argc, argv);

  if (version) {
    std::cout << Arc::IString("%s version %s", "arcrm", VERSION) << std::endl;
    return 0;
  }

  // If debug is specified as argument, it should be set before loading the configuration.
  if (!debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(debug));

  logger.msg(Arc::VERBOSE, "Running command: %s", options.GetCommandWithArguments());

  if (show_plugins) {
    std::list<Arc::ModuleDesc> modules;
    Arc::PluginsFactory pf(Arc::BaseConfig().MakeConfig(Arc::Config()).Parent());
    pf.scan(Arc::FinderLoader::GetLibrariesList(), modules);
    Arc::PluginsFactory::FilterByKind("HED:DMC", modules);

    std::cout << Arc::IString("Protocol plugins available:") << std::endl;
    for (std::list<Arc::ModuleDesc>::iterator itMod = modules.begin();
         itMod != modules.end(); ++itMod) {
      for (std::list<Arc::PluginDesc>::iterator itPlug = itMod->plugins.begin();
           itPlug != itMod->plugins.end(); ++itPlug) {
        std::cout << "  " << itPlug->name << " - " << itPlug->description << std::endl;
      }
    }
    return 0;
  }

  // credentials will be initialised later if necessary
  Arc::UserConfig usercfg(conffile, Arc::initializeCredentialsType::TryCredentials);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }
  usercfg.UtilsDirPath(Arc::UserConfig::ARCUSERDIRECTORY());
  usercfg.Timeout(timeout);
  if (force_system_ca) usercfg.CAUseSystem(true);
  if (force_grid_ca) usercfg.CAUseSystem(false);
  if (allow_insecure_connection) usercfg.TLSAllowInsecure(true);
 
  AuthenticationType authentication_type = UndefinedAuthentication;
  switch(authentication_type) {
    case NoAuthentication:
      usercfg.CommunicationAuthType(Arc::UserConfig::AuthTypeNone);
      break;
    case X509Authentication:
      usercfg.CommunicationAuthType(Arc::UserConfig::AuthTypeCert);
      break;
    case TokenAuthentication:
      usercfg.CommunicationAuthType(Arc::UserConfig::AuthTypeToken);
      break;
    case UndefinedAuthentication:
    default:
      usercfg.CommunicationAuthType(Arc::UserConfig::AuthTypeUndefined);
      break;
  }

  if (debug.empty() && !usercfg.Verbosity().empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(usercfg.Verbosity()));

  if (params.size() != 1) {
    logger.msg(Arc::ERROR, "Wrong number of parameters specified");
    return 1;
  }

  // add a slash to the end if not present
  std::string url = params.front();
  if (url[url.length()-1] != '/') url += '/';

  if (!arcmkdir(url, usercfg, with_parents))
    return 1;

  return 0;
}

int main(int argc, char **argv) {
  int xr = runmain(argc,argv);
  _exit(xr);
  return 0;
}

