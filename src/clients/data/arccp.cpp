// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <list>

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include <arc/ArcLocation.h>
#include <arc/GUID.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/User.h>
#include <arc/UserConfig.h>
#include <arc/credential/Credential.h>
#include <arc/data/FileCache.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataMover.h>
#include <arc/data/URLMap.h>
#include <arc/loader/FinderLoader.h>
#include <arc/OptionParser.h>

#include "utils.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(), "arccp");
static Arc::SimpleCondition cond;
static bool cancelled = false;

static void sig_cancel(int)
{
  if (cancelled) _exit(0);
  cancelled = true;
  cond.broadcast();
}


static void progress(FILE *o, const char*, unsigned int,
                     unsigned long long int all, unsigned long long int max,
                     double, double) {
  static int rs = 0;
  const char rs_[4] = {
    '|', '/', '-', '\\'
  };
  if (max) {
    fprintf(o, "\r|");
    unsigned int l = (74 * all + 37) / max;
    if (l > 74)
      l = 74;
    unsigned int i = 0;
    for (; i < l; i++)
      fprintf(o, "=");
    fprintf(o, "%c", rs_[rs++]);
    if (rs > 3)
      rs = 0;
    for (; i < 74; i++)
      fprintf(o, " ");
    fprintf(o, "|\r");
    fflush(o);
    return;
  }
  fprintf(o, "\r%llu kB                    \r", all / 1024);
}

static void transfer_cb(unsigned long long int bytes_transferred) {
  fprintf (stderr, "\r%llu kB                  \r", bytes_transferred / 1024);
}


static void mover_callback(Arc::DataMover* mover, Arc::DataStatus status, void* arg) {
  Arc::DataStatus* res = (Arc::DataStatus*)arg;
  *res = status;
  if (!res->Passed()) {
    logger.msg(Arc::ERROR, "Current transfer FAILED: %s", std::string(*res));
    if (res->Retryable()) {
      logger.msg(Arc::ERROR, "This seems like a temporary error, please try again later");
    }
  }
  cond.broadcast();
}

static bool checkProxy(Arc::UserConfig& usercfg, const Arc::URL& src_file) {
  if (!usercfg.InitializeCredentials(Arc::initializeCredentialsType::RequireCredentials)) {
    logger.msg(Arc::ERROR, "Unable to copy %s", src_file.str());
    logger.msg(Arc::ERROR, "Invalid credentials, please check proxy and/or CA certificates");
    return false;
  }
  Arc::Credential holder(usercfg);
  if (!holder.IsValid()) {
    if (holder.GetEndTime() < Arc::Time()) {
      logger.msg(Arc::ERROR, "Proxy expired");
    }
    logger.msg(Arc::ERROR, "Unable to copy %s", src_file.str());
    logger.msg(Arc::ERROR, "Invalid credentials, please check proxy and/or CA certificates");
    return false;
  }
  return true;
}

bool arctransfer(const Arc::URL& source_url,
                 const Arc::URL& destination_url,
                 const std::list<std::string>& locations,
                 Arc::UserConfig& usercfg,
                 bool secure,
                 bool passive,
                 bool verbose,
                 int timeout) {
  if (!source_url) {
    logger.msg(Arc::ERROR, "Invalid URL: %s", source_url.str());
    return false;
  }
  if (!destination_url) {
    logger.msg(Arc::ERROR, "Invalid URL: %s", destination_url.str());
    return false;
  }
  // Credentials are always required for 3rd party transfer
  if (!checkProxy(usercfg, source_url)) return false;
  if (timeout > 0) usercfg.Timeout(timeout);

  Arc::DataStatus res = Arc::DataPoint::Transfer3rdParty(source_url, destination_url, usercfg, verbose ? &transfer_cb : NULL);
  if (verbose) std::cerr<<std::endl;

  if (!res) {
    if (res == Arc::DataStatus::UnimplementedError) {
      logger.msg(Arc::ERROR, "Third party transfer is not supported for these endpoints");
    } else if (res.GetErrno() == EPROTONOSUPPORT) {
      logger.msg(Arc::ERROR, "Protocol(s) not supported - please check that the relevant gfal2\n"
                      "       plugins are installed (gfal2-plugin-* packages)");
    } else {
      logger.msg(Arc::ERROR, "Transfer FAILED: %s", std::string(res));
      if (res.Retryable()) {
        logger.msg(Arc::ERROR, "This seems like a temporary error, please try again later");
      }
    }
    return false;
  }
  return true;
}


bool arcregister(const Arc::URL& source_url,
                 const Arc::URL& destination_url,
                 Arc::UserConfig& usercfg,
                 bool force_meta) {
  if (!source_url) {
    logger.msg(Arc::ERROR, "Invalid URL: %s", source_url.str());
    return false;
  }
  if (!destination_url) {
    logger.msg(Arc::ERROR, "Invalid URL: %s", destination_url.str());
    return false;
  }
  if (source_url.Protocol() == "urllist" &&
      destination_url.Protocol() == "urllist") {
    std::list<Arc::URL> sources = Arc::ReadURLList(source_url);
    std::list<Arc::URL> destinations = Arc::ReadURLList(destination_url);
    if (sources.empty()) {
      logger.msg(Arc::ERROR, "Can't read list of sources from file %s",
                 source_url.Path());
      return false;
    }
    if (destinations.empty()) {
      logger.msg(Arc::ERROR, "Can't read list of destinations from file %s",
                 destination_url.Path());
      return false;
    }
    if (sources.size() != destinations.size()) {
      logger.msg(Arc::ERROR, "Numbers of sources and destinations do not match");
      return false;
    }
    bool r = true;
    for (std::list<Arc::URL>::iterator source = sources.begin(),
                                       destination = destinations.begin();
         (source != sources.end()) && (destination != destinations.end());
         ++source, ++destination) {
      if (!arcregister(*source, *destination, usercfg, force_meta)) r = false;
      if (cancelled) return true;
    }
    return r;
  }
  if (source_url.Protocol() == "urllist") {
    std::list<Arc::URL> sources = Arc::ReadURLList(source_url);
    if (sources.empty()) {
      logger.msg(Arc::ERROR, "Can't read list of sources from file %s",
                 source_url.Path());
      return false;
    }
    bool r = true;
    for (std::list<Arc::URL>::iterator source = sources.begin();
         source != sources.end(); ++source) {
      if (!arcregister(*source, destination_url, usercfg, force_meta)) r = false;
      if (cancelled) return true;
    }
    return r;
  }
  if (destination_url.Protocol() == "urllist") {
    std::list<Arc::URL> destinations = Arc::ReadURLList(destination_url);
    if (destinations.empty()) {
      logger.msg(Arc::ERROR, "Can't read list of destinations from file %s",
                 destination_url.Path());
      return false;
    }
    bool r = true;
    for (std::list<Arc::URL>::iterator destination = destinations.begin();
         destination != destinations.end(); ++destination) {
      if (!arcregister(source_url, *destination, usercfg, force_meta)) r = false;
      if (cancelled) return true;
    }
    return r;
  }

  if (destination_url.Path()[destination_url.Path().length() - 1] == '/') {
    logger.msg(Arc::ERROR, "Fileset registration is not supported yet");
    return false;
  }
  Arc::DataHandle source(source_url, usercfg);
  Arc::DataHandle destination(destination_url, usercfg);
  if (!source) {
    logger.msg(Arc::ERROR, "Unsupported source url: %s", source_url.str());
    return false;
  }
  if (!destination) {
    logger.msg(Arc::ERROR, "Unsupported destination url: %s", destination_url.str());
    return false;
  }
  if ((source->RequiresCredentials() || destination->RequiresCredentials())
      && !checkProxy(usercfg, source_url)) return false;

  if (source->IsIndex() || !destination->IsIndex()) {
    logger.msg(Arc::ERROR, "For registration source must be ordinary URL"
               " and destination must be indexing service");
    return false;
  }

  // Obtain meta-information about source
  Arc::FileInfo fileinfo;
  Arc::DataPoint::DataPointInfoType verb = (Arc::DataPoint::DataPointInfoType)Arc::DataPoint::INFO_TYPE_CONTENT;
  Arc::DataStatus res = source->Stat(fileinfo, verb);
  if (!res) {
    logger.msg(Arc::ERROR, "Could not obtain information about source: %s", std::string(res));
    return false;
  }
  // Check if destination is already registered
  if (destination->Resolve(true)) {
    // Check meta-info matches source
    if (!destination->CompareMeta(*source) && !force_meta) {
      logger.msg(Arc::ERROR, "Metadata of source does not match existing "
          "destination. Use the --force option to override this.");
      return false;
    }
    // Remove existing locations
    destination->ClearLocations();
  }
  bool replication = destination->Registered();
  destination->SetMeta(*source); // pass metadata
  // Add new location
  std::string metaname = source_url.ConnectionURL();
  if (!destination->AddLocation(source_url, metaname)) {
    logger.msg(Arc::ERROR, "Failed to accept new file/destination");
    return false;
  }
  destination->SetTries(1);
  res = destination->PreRegister(replication, force_meta);
  if (!res) {
    logger.msg(Arc::ERROR, "Failed to register new file/destination: %s", std::string(res));
    return false;
  }
  res = destination->PostRegister(replication);
  if (!res) {
    destination->PreUnregister(replication);
    logger.msg(Arc::ERROR, "Failed to register new file/destination: %s", std::string(res));
    return false;
  }
  return true;
}

static Arc::DataStatus do_mover(const Arc::URL& s_url,
                                const Arc::URL& d_url,
                                const std::list<std::string>& locations,
                                const std::string& cache_dir,
                                Arc::UserConfig& usercfg,
                                bool secure,
                                bool passive,
                                bool force_meta,
                                int tries,
                                bool verbose,
                                int timeout) {

  Arc::DataHandle source(s_url, usercfg);
  Arc::DataHandle destination(d_url, usercfg);
  if (!source) {
    logger.msg(Arc::ERROR, "Unsupported source url: %s", s_url.str());
    return Arc::DataStatus::ReadAcquireError;
  }
  if (!destination) {
    logger.msg(Arc::ERROR, "Unsupported destination url: %s", d_url.str());
    return Arc::DataStatus::WriteAcquireError;
  }
  if ((source->RequiresCredentials() || destination->RequiresCredentials())
      && !checkProxy(usercfg, s_url)) return Arc::DataStatus::CredentialsExpiredError;

  if (!locations.empty()) {
    std::string meta(destination->GetURL().Protocol()+"://"+destination->GetURL().Host());
    for (std::list<std::string>::const_iterator i = locations.begin(); i != locations.end(); ++i) {
      destination->AddLocation(*i, meta);
    }
  }
  Arc::DataMover mover;
  mover.secure(secure);
  mover.passive(passive);
  mover.verbose(verbose);
  mover.force_to_meta(force_meta);
  if (tries) {
    mover.retry(true); // go through all locations
    source->SetTries(tries); // try all locations "tries" times
    destination->SetTries(tries);
  }
  Arc::User cache_user;
  Arc::FileCache cache;
  if (!cache_dir.empty()) cache = Arc::FileCache(cache_dir+" .", "", cache_user.get_uid(), cache_user.get_gid());
  if (verbose) mover.set_progress_indicator(&progress);

  Arc::DataStatus callback_res;
  Arc::DataStatus res = mover.Transfer(*source, *destination, cache, Arc::URLMap(),
                                       0, 0, 0, timeout, &mover_callback, &callback_res);
  if (!res.Passed()) {
    logger.msg(Arc::ERROR, "Current transfer FAILED: %s", std::string(res));
    if (res.Retryable()) {
      logger.msg(Arc::ERROR, "This seems like a temporary error, please try again later");
    }
    return res;
  }
  cond.wait(); // wait for mover_callback

  if (verbose) std::cerr<<std::endl;
  if (cache) cache.Release();

  return callback_res;
}

bool arccp(const Arc::URL& source_url_,
           const Arc::URL& destination_url_,
           const std::list<std::string>& locations,
           const std::string& cache_dir,
           Arc::UserConfig& usercfg,
           bool secure,
           bool passive,
           bool force_meta,
           int recursion,
           int tries,
           bool verbose,
           int timeout) {
  Arc::URL source_url(source_url_);
  if (!source_url) {
    logger.msg(Arc::ERROR, "Invalid URL: %s", source_url.str());
    return false;
  }
  Arc::URL destination_url(destination_url_);
  if (!destination_url) {
    logger.msg(Arc::ERROR, "Invalid URL: %s", destination_url.str());
    return false;
  }

  if (timeout <= 0) timeout = 300; // 5 minute default
  if (tries < 0) tries = 0;
  if (source_url.Protocol() == "urllist" &&
      destination_url.Protocol() == "urllist") {
    std::list<Arc::URL> sources = Arc::ReadURLList(source_url);
    std::list<Arc::URL> destinations = Arc::ReadURLList(destination_url);
    if (sources.empty()) {
      logger.msg(Arc::ERROR, "Can't read list of sources from file %s",
                 source_url.Path());
      return false;
    }
    if (destinations.empty()) {
      logger.msg(Arc::ERROR, "Can't read list of destinations from file %s",
                 destination_url.Path());
      return false;
    }
    if (sources.size() != destinations.size()) {
      logger.msg(Arc::ERROR,
                 "Numbers of sources and destinations do not match");
      return false;
    }
    bool r = true;
    for (std::list<Arc::URL>::iterator source = sources.begin(),
                                       destination = destinations.begin();
         (source != sources.end()) && (destination != destinations.end());
         ++source, ++destination) {
      if (!arccp(*source, *destination, locations, cache_dir, usercfg, secure, passive,
                 force_meta, recursion, tries, verbose, timeout)) r = false;
      if (cancelled) return true;
    }
    return r;
  }
  if (source_url.Protocol() == "urllist") {
    std::list<Arc::URL> sources = Arc::ReadURLList(source_url);
    if (sources.empty()) {
      logger.msg(Arc::ERROR, "Can't read list of sources from file %s",
                 source_url.Path());
      return false;
    }
    bool r = true;
    for (std::list<Arc::URL>::iterator source = sources.begin();
         source != sources.end(); ++source) {
      if (!arccp(*source, destination_url, locations, cache_dir, usercfg, secure,
                 passive, force_meta, recursion, tries, verbose, timeout)) r = false;
      if (cancelled) return true;
    }
    return r;
  }
  if (destination_url.Protocol() == "urllist") {
    std::list<Arc::URL> destinations = Arc::ReadURLList(destination_url);
    if (destinations.empty()) {
      logger.msg(Arc::ERROR, "Can't read list of destinations from file %s",
                 destination_url.Path());
      return false;
    }
    bool r = true;
    for (std::list<Arc::URL>::iterator destination = destinations.begin();
         destination != destinations.end(); ++destination) {
      if (!arccp(source_url, *destination, locations, cache_dir, usercfg, secure,
                 passive, force_meta, recursion, tries, verbose, timeout)) r = false;
      if (cancelled) return true;
    }
    return r;
  }

  if (destination_url.Path()[destination_url.Path().length() - 1] != '/') {
    if (source_url.Path()[source_url.Path().length() - 1] == '/' &&
        source_url.MetaDataOption("guid").empty()) { // files specified by guid may have path '/'
      logger.msg(Arc::ERROR,
                 "Fileset copy to single object is not supported yet");
      return false;
    }
  }
  else {
    // Copy TO fileset/directory
    if (source_url.Path()[source_url.Path().length() - 1] != '/') {
      // Copy FROM single object
      std::string::size_type p = source_url.Path().rfind('/');
      if (p == std::string::npos) {
        logger.msg(Arc::ERROR, "Can't extract object's name from source url");
        return false;
      }
      destination_url.ChangePath(destination_url.Path() +
                                 source_url.Path().substr(p + 1));
    }
    else {
      // Fileset copy
      Arc::DataHandle source(source_url, usercfg);
      if (!source) {
        logger.msg(Arc::ERROR, "Unsupported source url: %s", source_url.str());
        return false;
      }
      if (source->RequiresCredentials() && !checkProxy(usercfg, source_url)) return false;

      std::list<Arc::FileInfo> files;
      Arc::DataStatus result = source->List(files, (Arc::DataPoint::DataPointInfoType)
                                           (Arc::DataPoint::INFO_TYPE_NAME | Arc::DataPoint::INFO_TYPE_TYPE));
      if (!result.Passed()) {
        logger.msg(Arc::ERROR, "%s. Cannot copy fileset", std::string(result));
        return false;
      }
      bool failures = false;
      // Handle transfer of files first (treat unknown like files)
      for (std::list<Arc::FileInfo>::iterator i = files.begin();
           i != files.end(); ++i) {
        if ((i->GetType() != Arc::FileInfo::file_type_unknown) &&
            (i->GetType() != Arc::FileInfo::file_type_file)) continue;

        logger.msg(Arc::INFO, "Name: %s", i->GetName());
        Arc::URL s_url(std::string(source_url.str() + i->GetName()));
        Arc::URL d_url(std::string(destination_url.str() + i->GetName()));
        logger.msg(Arc::INFO, "Source: %s", s_url.str());
        logger.msg(Arc::INFO, "Destination: %s", d_url.str());

        Arc::DataStatus res = do_mover(s_url, d_url, locations, cache_dir, usercfg, secure, passive,
                                       force_meta, tries, verbose, timeout);
        if (cancelled) return true;
        if (!res.Passed()) failures = true;
        else logger.msg(Arc::INFO, "Current transfer complete");
      }
      if (failures) {
        logger.msg(Arc::ERROR, "Some transfers failed");
        return false;
      }
      // Go deeper if allowed
      bool r = true;
      if (recursion > 0)
        // Handle directories recursively
        for (std::list<Arc::FileInfo>::iterator i = files.begin();
             i != files.end(); ++i) {
          if (i->GetType() != Arc::FileInfo::file_type_dir) continue;
          if (verbose) logger.msg(Arc::INFO, "Directory: %s", i->GetName());
          std::string s_url(source_url.str());
          std::string d_url(destination_url.str());
          s_url += i->GetName();
          d_url += i->GetName();
          s_url += "/";
          d_url += "/";
          if (!arccp(s_url, d_url, locations, cache_dir, usercfg, secure, passive,
                     force_meta, recursion - 1, tries, verbose, timeout)) r = false;
          if (cancelled) return true;
        }
      return r;
    }
  }

  Arc::DataStatus res = do_mover(source_url, destination_url, locations, cache_dir, usercfg, secure, passive,
                                 force_meta, tries, verbose, timeout);
  if (cancelled) return true;
  if (!res.Passed()) return false;

  logger.msg(Arc::INFO, "Transfer complete");
  return true;
}

static int runmain(int argc, char **argv) {

  setlocale(LC_ALL, "");

  // set signal handlers for safe cancellation
  signal(SIGTERM, sig_cancel);
  signal(SIGINT, sig_cancel);

  static Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("source destination"),
                            istring("The arccp command copies files to, from "
                                    "and between grid storage elements."));

  bool passive = false;
  options.AddOption('p', "passive",
                    istring("use passive transfer (off by default if secure "
                            "is on, on by default if secure is not requested)"),
                    passive);

  bool notpassive = false;
  options.AddOption('n', "nopassive",
                    istring("do not try to force passive transfer"),
                    notpassive);

  bool force = false;
  options.AddOption('f', "force",
                    istring("force overwrite of existing destination"),
                    force);

  bool verbose = false;
  options.AddOption('i', "indicate", istring("show progress indicator"),
                    verbose);

  bool nocopy = false;
  options.AddOption('T', "notransfer",
                    istring("do not transfer, but register source into "
                            "destination. destination must be a meta-url."),
                    nocopy);

  bool secure = false;
  options.AddOption('u', "secure",
                    istring("use secure transfer (insecure by default)"),
                    secure);

  std::string cache_path;
  options.AddOption('y', "cache",
                    istring("path to local cache (use to put file into cache)"),
                    istring("path"), cache_path);

  bool infinite_recursion = false;
  options.AddOption('r', "recursive",
                    istring("operate recursively"),
                    infinite_recursion);

  int recursion = 0;
  options.AddOption('D', "depth",
                    istring("operate recursively up to specified level"),
                    istring("level"), recursion);

  int retries = 0;
  options.AddOption('R', "retries",
                    istring("number of retries before failing file transfer"),
                    istring("number"), retries);

  std::list<std::string> locations;
  options.AddOption('L', "location",
                    istring("physical location to write to when destination is an indexing service."
                            " Must be specified for indexing services which do not automatically"
                            " generate physical locations. Can be specified multiple times -"
                            " locations will be tried in order until one succeeds."),
                    istring("URL"), locations);

  bool thirdparty = false;
  options.AddOption('3', "thirdparty",
                    istring("perform third party transfer, where the destination pulls"
                            " from the source (only available with GFAL plugin)"),
                    thirdparty);

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

  std::string debug;
  options.AddOption('d', "debug",
                    istring("FATAL, ERROR, WARNING, INFO, VERBOSE or DEBUG"),
                    istring("debuglevel"), debug);

  bool version = false;
  options.AddOption('v', "version", istring("print version information"),
                    version);

  std::list<std::string> params = options.Parse(argc, argv);

  if (version) {
    std::cout << Arc::IString("%s version %s", "arccp", VERSION) << std::endl;
    return 0;
  }

  // If debug is specified as argument, it should be set before loading the configuration.
  if (!debug.empty()) Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(debug));

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

  // Attempt to acquire credentials. Whether they are required will be
  // determined later depending on the protocol.
  Arc::UserConfig usercfg(conffile, Arc::initializeCredentialsType::TryCredentials);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }
  usercfg.UtilsDirPath(Arc::UserConfig::ARCUSERDIRECTORY());
  if (force_system_ca) usercfg.CAUseSystem(true);
  if (force_grid_ca) usercfg.CAUseSystem(false);

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

  if (debug.empty() && !usercfg.Verbosity().empty()) {
    Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(usercfg.Verbosity()));
  }

  if (params.size() != 2) {
    logger.msg(Arc::ERROR, "Wrong number of parameters specified");
    return 1;
  }

  if (passive && notpassive) {
    logger.msg(Arc::ERROR, "Options 'p' and 'n' can't be used simultaneously");
    return 1;
  }

  if ((!secure) && (!notpassive)) passive = true;
  if (infinite_recursion) recursion = INT_MAX;

  std::list<std::string>::iterator it = params.begin();
  std::string source = *it;
  ++it;
  std::string destination = *it;

  if (source == "-") source = "stdio:///stdin";
  if (destination == "-") destination = "stdio:///stdout";

  if (thirdparty) {
    if (!arctransfer(source, destination, locations, usercfg, secure, passive, verbose, timeout)) return 1;
  } else if (nocopy) {
    if (!arcregister(source, destination, usercfg, force)) return 1;
  } else {
    if (!arccp(source, destination, locations, cache_path, usercfg, secure, passive, force,
               recursion, retries + 1, verbose, timeout)) return 1;
  }

  return 0;
}

int main(int argc, char **argv) {
  int xr = runmain(argc,argv);
  _exit(xr);
  return 0;
}

