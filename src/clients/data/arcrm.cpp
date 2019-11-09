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
#include <arc/data/DataMover.h>
#include <arc/loader/FinderLoader.h>
#include <arc/OptionParser.h>

static Arc::Logger logger(Arc::Logger::getRootLogger(), "arcrm");

/// Returns number of files that failed to be deleted
int arcrm(const std::list<Arc::URL>& urls,
          Arc::UserConfig& usercfg,
          bool errcont) {

  Arc::DataHandle* handle = NULL;
  Arc::DataMover mover;
  unsigned int failed = 0;
  for (std::list<Arc::URL>::const_iterator url = urls.begin(); url != urls.end(); ++url) {

    if (!(*url)) {
      logger.msg(Arc::ERROR, "Invalid URL: %s", url->str());
      failed++;
      continue;
    }
    if (url->Protocol() == "urllist") {
      std::list<Arc::URL> url_files = Arc::ReadURLList(*url);
      if (url_files.empty()) {
        logger.msg(Arc::ERROR, "Can't read list of locations from file %s", url->Path());
        failed += 1;
      } else {
        failed += arcrm(url_files, usercfg, errcont);
      }
      continue;
    }
    // Depending on protocol SetURL() may allow reusing connections and hence
    // the same DataHandle object to delete multiple files. If it is not
    // supported SetURL() returns false and a new DataHandle must be created.
    if (!handle || !(*handle)->SetURL(*url)) {
      delete handle;
      handle = new Arc::DataHandle(*url, usercfg);

      if (!(*handle)) {
        logger.msg(Arc::ERROR, "Unsupported URL given: %s", url->str());
        failed++;
        delete handle;
        handle = NULL;
        continue;
      }

      if ((*handle)->RequiresCredentials()) {
        if (!usercfg.InitializeCredentials(Arc::initializeCredentialsType::RequireCredentials)) {
          logger.msg(Arc::ERROR, "Unable to remove file %s", url->str());
          logger.msg(Arc::ERROR, "Invalid credentials, please check proxy and/or CA certificates");
          failed++;
          delete handle;
          handle = NULL;
          continue;
        }
        Arc::Credential holder(usercfg);
        if (!holder.IsValid()) {
          if (holder.GetEndTime() < Arc::Time()) {
            logger.msg(Arc::ERROR, "Proxy expired");
          }
          logger.msg(Arc::ERROR, "Unable to remove file %s", url->str());
          logger.msg(Arc::ERROR, "Invalid credentials, please check proxy and/or CA certificates");
          failed++;
          delete handle;
          handle = NULL;
          continue;
        }
      }
    }

    // only one try
    (*handle)->SetTries(1);
    Arc::DataStatus res =  mover.Delete(**handle, errcont);
    if (!res.Passed()) {
      logger.msg(Arc::ERROR, std::string(res));
      if (res.Retryable()) {
        logger.msg(Arc::ERROR, "This seems like a temporary error, please try again later");
      }
      failed++;
    }
  }
  delete handle;
  return failed;
}

static int runmain(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("url [url ...]"),
                            istring("The arcrm command deletes files and on "
                                    "grid storage elements."));

  bool force = false;
  options.AddOption('f', "force",
                    istring("remove logical file name registration even "
                            "if not all physical instances were removed"),
                    force);

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
  if (!debug.empty()) {
    Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(debug));
  }

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

  if (debug.empty() && !usercfg.Verbosity().empty()) {
    Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(usercfg.Verbosity()));
  }

  if (params.empty()) {
   logger.msg(Arc::ERROR, "Wrong number of parameters specified");
   return 1;
  }

  std::list<Arc::URL> urls;
  for (std::list<std::string>::const_iterator i = params.begin(); i != params.end(); ++i) {
    urls.push_back(*i);
  }
  unsigned int failed = arcrm(urls, usercfg, force);
  if (failed != 0) {
    if (params.size() != 1 || failed > 1) std::cout<<failed<<" deletions failed"<<std::endl;
    return 1;
  }
  return 0;
}

int main(int argc, char **argv) {
  int xr = runmain(argc,argv);
  _exit(xr);
  return 0;
}

