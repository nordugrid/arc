#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <list>

#include <unistd.h>

#include <arc/ArcLocation.h>
#include <arc/FileUtils.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/credential/Credential.h>
#include <arc/data/DataHandle.h>
#include <arc/loader/FinderLoader.h>
#include <arc/OptionParser.h>

#include "utils.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(), "arcrename");

bool arcrename(const Arc::URL& old_url,
               const Arc::URL& new_url,
               Arc::UserConfig& usercfg,
               int timeout) {

  if (!old_url) {
    logger.msg(Arc::ERROR, "Invalid URL: %s", old_url.str());
    return false;
  }
  if (!new_url) {
    logger.msg(Arc::ERROR, "Invalid URL: %s", new_url.str());
    return false;
  }

  // Check URLs
  if (old_url.Protocol() != new_url.Protocol() ||
      old_url.Host() != new_url.Host() ||
      old_url.Port() != new_url.Port()) {
    logger.msg(Arc::ERROR, "Both URLs must have the same protocol, host and port");
    return false;
  }
  std::string old_path(old_url.Path());
  std::string new_path(new_url.Path());
  Arc::CanonicalDir(old_path, true);
  Arc::CanonicalDir(new_path, true);
  // LFC URLs can be specified by guid metadata option
  if ((old_path.find_first_not_of('/') == std::string::npos && old_url.MetaDataOptions().empty()) ||
      new_path.find_first_not_of('/') == std::string::npos) {
    logger.msg(Arc::ERROR, "Cannot rename to or from root directory");
    return false;
  }
  if (old_path == new_path && old_url.FullPath() == new_url.FullPath()) {
    logger.msg(Arc::ERROR, "Cannot rename to the same URL");
    return false;
  }

  Arc::DataHandle url(old_url, usercfg);
  if (!url) {
    logger.msg(Arc::ERROR, "Unsupported URL given");
    return false;
  }
  if (url->RequiresCredentials()) {
    if (!usercfg.InitializeCredentials(Arc::initializeCredentialsType::RequireCredentials)) {
      logger.msg(Arc::ERROR, "Unable to rename %s", old_url.str());
      logger.msg(Arc::ERROR, "Invalid credentials, please check proxy and/or CA certificates");
      return false;
    }
    Arc::Credential holder(usercfg);
    if (!holder.IsValid()) {
      if (holder.GetEndTime() < Arc::Time()) {
        logger.msg(Arc::ERROR, "Proxy expired");
      }
      logger.msg(Arc::ERROR, "Unable to rename %s", old_url.str());
      logger.msg(Arc::ERROR, "Invalid credentials, please check proxy and/or CA certificates");
      return false;
    }
  }

  // Insecure by default
  url->SetSecure(false);

  // Do the renaming
  Arc::DataStatus res = url->Rename(new_url);
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

  Arc::OptionParser options(istring("old_url new_url"),
                            istring("The arcrename command renames files on "
                                    "grid storage elements."));

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

  bool force_default_ca = false;
  options.AddOption('\0', "defaultca",
              istring("force using CA certificates configuration provided by OpenSSL"),
              force_default_ca);

  bool force_grid_ca = false;
  options.AddOption('\0', "gridca",
              istring("force using CA certificates configuration for Grid services (typically IGTF)"),
              force_grid_ca);
    
  std::string debug;
  options.AddOption('d', "debug",
                    istring("FATAL, ERROR, WARNING, INFO, VERBOSE or DEBUG"),
                    istring("debuglevel"), debug);

  bool version = false;
  options.AddOption('v', "version", istring("print version information"),
                    version);

  std::list<std::string> params = options.Parse(argc, argv);

  if (version) {
    std::cout << Arc::IString("%s version %s", "arcrename", VERSION) << std::endl;
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
  if (force_default_ca) usercfg.CAUseDefault(true);
  if (force_grid_ca) usercfg.CAUseDefault(false);

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

  if (params.size() != 2) {
    logger.msg(Arc::ERROR, "Wrong number of parameters specified");
    return 1;
  }

  std::string oldurl(params.front());
  std::string newurl(params.back());
  if (!arcrename(oldurl, newurl, usercfg, timeout))
    return 1;

  return 0;
}

int main(int argc, char **argv) {
  int xr = runmain(argc,argv);
  _exit(xr);
  return 0;
}

