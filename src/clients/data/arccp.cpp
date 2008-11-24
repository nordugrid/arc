#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <list>

#include <arc/ArcLocation.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/User.h>
#include <arc/XMLNode.h>
#include <arc/client/UserConfig.h>
#include <arc/data/FileCache.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataMover.h>
#include <arc/data/URLMap.h>
#include <arc/OptionParser.h>

static Arc::Logger logger(Arc::Logger::getRootLogger(), "arccp");

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

void arcregister(const Arc::URL& source_url,
		 const Arc::URL& destination_url,
		 Arc::XMLNode credentials,
		 bool secure,
		 bool passive,
		 bool force_meta,
		 int timeout) {
  if (source_url.Protocol() == "urllist" &&
      destination_url.Protocol() == "urllist") {
    std::list<Arc::URL> sources = Arc::ReadURLList(source_url);
    std::list<Arc::URL> destinations = Arc::ReadURLList(destination_url);
    if (sources.size() == 0) {
      logger.msg(Arc::ERROR, "Can't read list of sources from file %s",
		 source_url.Path());
      return;
    }
    if (destinations.size() == 0) {
      logger.msg(Arc::ERROR, "Can't read list of destinations from file %s",
		 destination_url.Path());
      return;
    }
    if (sources.size() != destinations.size()) {
      logger.msg(Arc::ERROR,
		 "Numbers of sources and destinations do not match");
      return;
    }
    for (std::list<Arc::URL>::iterator source = sources.begin(),
				       destination = destinations.begin();
	 (source != sources.end()) && (destination != destinations.end());
	 source++, destination++)
      arcregister(*source, *destination, credentials, secure, passive, force_meta, timeout);
    return;
  }
  if (source_url.Protocol() == "urllist") {
    std::list<Arc::URL> sources = Arc::ReadURLList(source_url);
    if (sources.size() == 0) {
      logger.msg(Arc::ERROR, "Can't read list of sources from file %s",
		 source_url.Path());
      return;
    }
    for (std::list<Arc::URL>::iterator source = sources.begin();
	 source != sources.end(); source++)
      arcregister(*source, destination_url, credentials, secure, passive, force_meta,
		  timeout);
    return;
  }
  if (destination_url.Protocol() == "urllist") {
    std::list<Arc::URL> destinations = Arc::ReadURLList(destination_url);
    if (destinations.size() == 0) {
      logger.msg(Arc::ERROR, "Can't read list of destinations from file %s",
		 destination_url.Path());
      return;
    }
    for (std::list<Arc::URL>::iterator destination = destinations.begin();
	 destination != destinations.end(); destination++)
      arcregister(source_url, *destination, credentials, secure, passive, force_meta,
		  timeout);
    return;
  }

  if (destination_url.Path()[destination_url.Path().length() - 1] == '/') {
    logger.msg(Arc::ERROR, "Fileset registration is not supported yet");
    return;
  }
  Arc::DataHandle source(source_url);
  source->AssignCredentials(credentials);
  Arc::DataHandle destination(destination_url);
  destination->AssignCredentials(credentials);
  if (!source) {
    logger.msg(Arc::ERROR, "Unsupported source url: %s", source_url.str());
    return;
  }
  if (!destination) {
    logger.msg(Arc::ERROR, "Unsupported destination url: %s",
	       destination_url.str());
    return;
  }
  if (source->IsIndex() || !destination->IsIndex()) {
    logger.msg(Arc::ERROR, "For registration source must be ordinary URL"
	       " and destination must be indexing service");
    return;
  }
  // Obtain meta-information about source
  if (!source->Check()) {
    logger.msg(Arc::ERROR, "Source probably does not exist");
    return;
  }
  // add new location
  if (!destination->Resolve(false)) {
    logger.msg(Arc::ERROR, "Problems resolving destination");
    return;
  }
  bool replication = destination->Registered();
  destination->SetMeta(*source); // pass metadata
  std::string metaname;
  // look for similar destination
  for (destination->SetTries(1); destination->LocationValid();
       destination->NextLocation()) {
    const Arc::URL& loc_url = destination->CurrentLocation();
    if (loc_url == source_url) {
      metaname = destination->CurrentLocationMetadata();
      break;
    }
  }
  // remove locations if exist
  for (destination->SetTries(1); destination->RemoveLocation();) {}
  // add new location
  if (metaname.empty())
    metaname = source_url.ConnectionURL();
  if (!destination->AddLocation(source_url, metaname)) {
    destination->PreUnregister(replication);
    logger.msg(Arc::ERROR, "Failed to accept new file/destination");
    return;
  }
  destination->SetTries(1);
  if (!destination->PreRegister(replication, force_meta)) {
    logger.msg(Arc::ERROR, "Failed to register new file/destination");
    return;
  }
  if (!destination->PostRegister(replication)) {
    destination->PreUnregister(replication);
    logger.msg(Arc::ERROR, "Failed to register new file/destination");
    return;
  }
  return;
}

void arccp(const Arc::URL& source_url_,
	   const Arc::URL& destination_url_,
	   Arc::XMLNode credentials,
	   bool secure,
	   bool passive,
	   bool force_meta,
	   int recursion,
	   int tries,
	   bool verbose,
	   int timeout) {
  Arc::URL source_url(source_url_);
  Arc::URL destination_url(destination_url_);
  std::string cache_path;
  std::string cache_data_path;
  std::string id = "<ngcp>";
  if (timeout <= 0)
    timeout = 300; // 5 minute default
  if (tries < 0)
    tries = 0;
  if (source_url.Protocol() == "urllist" &&
      destination_url.Protocol() == "urllist") {
    std::list<Arc::URL> sources = Arc::ReadURLList(source_url);
    std::list<Arc::URL> destinations = Arc::ReadURLList(destination_url);
    if (sources.size() == 0) {
      logger.msg(Arc::ERROR, "Can't read list of sources from file %s",
		 source_url.Path());
      return;
    }
    if (destinations.size() == 0) {
      logger.msg(Arc::ERROR, "Can't read list of destinations from file %s",
		 destination_url.Path());
      return;
    }
    if (sources.size() != destinations.size()) {
      logger.msg(Arc::ERROR,
		 "Numbers of sources and destinations do not match");
      return;
    }
    for (std::list<Arc::URL>::iterator source = sources.begin(),
				       destination = destinations.begin();
	 (source != sources.end()) && (destination != destinations.end());
	 source++, destination++)
      arccp(*source, *destination, credentials, secure, passive, force_meta, recursion,
	    tries, verbose, timeout);
    return;
  }
  if (source_url.Protocol() == "urllist") {
    std::list<Arc::URL> sources = Arc::ReadURLList(source_url);
    if (sources.size() == 0) {
      logger.msg(Arc::ERROR, "Can't read list of sources from file %s",
		 source_url.Path());
      return;
    }

    for (std::list<Arc::URL>::iterator source = sources.begin();
	 source != sources.end(); source++)
      arccp(*source, destination_url, credentials, secure, passive, force_meta, recursion,
	    tries, verbose, timeout);
    return;
  }
  if (destination_url.Protocol() == "urllist") {
    std::list<Arc::URL> destinations = Arc::ReadURLList(destination_url);
    if (destinations.size() == 0) {
      logger.msg(Arc::ERROR, "Can't read list of destinations from file %s",
		 destination_url.Path());
      return;
    }
    for (std::list<Arc::URL>::iterator destination = destinations.begin();
	 destination != destinations.end(); destination++)
      arccp(source_url, *destination, credentials, secure, passive, force_meta, recursion,
	    tries, verbose, timeout);
    return;
  }

  if (destination_url.Path()[destination_url.Path().length() - 1] != '/') {
    if (source_url.Path()[source_url.Path().length() - 1] == '/') {
      logger.msg(Arc::ERROR,
		 "Fileset copy to single object is not supported yet");
      return;
    }
  }
  else {
    // Copy TO fileset/directory
    if (source_url.Path()[source_url.Path().length() - 1] != '/') {
      // Copy FROM single object
      std::string::size_type p = source_url.Path().rfind('/');
      if (p == std::string::npos) {
	logger.msg(Arc::ERROR, "Can't extract object's name from source url");
	return;
      }
      destination_url.ChangePath(destination_url.Path() +
				 source_url.Path().substr(p + 1));
    }
    else {
      // Fileset copy
      // Find out if source can be listed (TODO - through datapoint)
      if ((source_url.Protocol() != "rc") &&
	  (source_url.Protocol() != "rls") &&
	  (source_url.Protocol() != "fireman") &&
	  (source_url.Protocol() != "file") &&
	  (source_url.Protocol() != "se") &&
	  (source_url.Protocol() != "gsiftp") &&
	  (source_url.Protocol() != "ftp")) {
	logger.msg(Arc::ERROR,
		   "Fileset copy for this kind of source is not supported");
	return;
      }
      Arc::DataHandle source(source_url);
      source->AssignCredentials(credentials);
      if (!source) {
	logger.msg(Arc::ERROR, "Unsupported source url: %s", source_url.str());
	return;
      }
      std::list<Arc::FileInfo> files;
      if (source->IsIndex()) {
	if (!source->ListFiles(files, true, false)) {
	  logger.msg(Arc::ERROR, "Failed listing metafiles");
	  return;
	}
      }
      else
	if (!source->ListFiles(files, true, false)) {
	  logger.msg(Arc::ERROR, "Failed listing files");
	  return;
	}
      bool failures = false;
      // Handle transfer of files first (treat unknown like files)
      for (std::list<Arc::FileInfo>::iterator i = files.begin();
	   i != files.end(); i++) {
	if ((i->GetType() != Arc::FileInfo::file_type_unknown) &&
	    (i->GetType() != Arc::FileInfo::file_type_file))
	  continue;
	logger.msg(Arc::INFO, "Name: %s", i->GetName());
	std::string s_url(source_url.str());
	std::string d_url(destination_url.str());
	s_url += i->GetName();
	d_url += i->GetName();
	logger.msg(Arc::INFO, "Source: %s", s_url);
	logger.msg(Arc::INFO, "Destination: %s", d_url);
	Arc::DataHandle source(s_url);
	source->AssignCredentials(credentials);
	Arc::DataHandle destination(d_url);
	destination->AssignCredentials(credentials);
	if (!source) {
	  logger.msg(Arc::INFO, "Unsupported source url: %s", s_url);
	  continue;
	}
	if (!destination) {
	  logger.msg(Arc::INFO, "Unsupported destination url: %s", d_url);
	  continue;
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
	std::string failure;
	if (!mover.Transfer(*source, *destination, cache, Arc::URLMap(),
			    0, 0, 0, timeout, failure)) {
	  if (!failure.empty())
	    logger.msg(Arc::INFO, "Current transfer FAILED: %s", failure);
	  else
	    logger.msg(Arc::INFO, "Current transfer FAILED");
	  destination->SetTries(1);
	  // It is not clear how to clear half-registered file. So remove it
	  // only in case of explicit destination.
	  if (!(destination->IsIndex()))
	    mover.Delete(*destination);
	  failures = true;
	}
	else
	  logger.msg(Arc::INFO, "Current transfer complete");
      }
      if (failures) {
	logger.msg(Arc::ERROR, "Some transfers failed");
	return;
      }
      // Go deeper if allowed
      if (recursion > 0)
	// Handle directories recursively
	for (std::list<Arc::FileInfo>::iterator i = files.begin();
	     i != files.end(); i++) {
	  if (i->GetType() != Arc::FileInfo::file_type_dir)
	    continue;
	  if (verbose)
	    logger.msg(Arc::INFO, "Directory: %s", i->GetName());
	  std::string s_url(source_url.str());
	  std::string d_url(destination_url.str());
	  s_url += i->GetName();
	  d_url += i->GetName();
	  s_url += "/";
	  d_url += "/";
	  arccp(s_url, d_url, credentials, secure, passive, force_meta, recursion - 1,
		tries, verbose, timeout);
	}
      return;
    }
  }
  Arc::DataHandle source(source_url);
  source->AssignCredentials(credentials);
  Arc::DataHandle destination(destination_url);
  destination->AssignCredentials(credentials);
  if (!source) {
    logger.msg(Arc::ERROR, "Unsupported source url: %s", source_url.str());
    return;
  }
  if (!destination) {
    logger.msg(Arc::ERROR, "Unsupported destination url: %s",
	       destination_url.str());
    return;
  }
  Arc::DataMover mover;
  mover.secure(secure);
  mover.passive(passive);
  mover.verbose(verbose);
  mover.force_to_meta(force_meta);
  if (tries) { // 0 means default behavior
    mover.retry(true); // go through all locations
    source->SetTries(tries); // try all locations "tries" times
    destination->SetTries(tries);
  }
  Arc::User cache_user;
  Arc::FileCache cache;
  if (verbose)
    mover.set_progress_indicator(&progress);
  std::string failure;
  if (!mover.Transfer(*source, *destination, cache, Arc::URLMap(),
		      0, 0, 0, timeout, failure)) {
    if (failure.length()) {
      logger.msg(Arc::ERROR, "Transfer FAILED: %s", failure);
      return;
    }
    else {
      logger.msg(Arc::ERROR, "Transfer FAILED");
      return;
    }
    destination->SetTries(1);
    // It is not clear how to clear half-registered file. So remove it only
    // in case of explicit destination.
    if (!(destination->IsIndex()))
      mover.Delete(*destination);
  }
  logger.msg(Arc::INFO, "Transfer complete");
  return;
}

int main(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("source destination"),
			    istring("The arccp command copies files to, from "
				    "and between grid storage elements."));

  bool passive = false;
  options.AddOption('p', "passive",
		    istring("use passive transfer (does not work if secure "
			    "is on, default if secure is not requested)"),
		    passive);

  bool notpassive = false;
  options.AddOption('n', "nopassive",
		    istring("do not try to force passive transfer"),
		    notpassive);

  bool force = false;
  options.AddOption('f', "force",
		    istring("if the destination is an indexing service "
			    "and not the same as the source and the "
			    "destination is already registered, then "
			    "the copy is normally not done. However, if "
			    "this option is specified the source is "
			    "assumed to be a replica of the destination "
			    "created in an uncontrolled way and the "
			    "copy is done like in case of replication"),
		    force);

  bool verbose = false;
  options.AddOption('i', "indicate", istring("show progress indicator"),
		    verbose);

  bool nocopy = false;
  options.AddOption('T', "notransfer",
		    istring("do not transfer file, just register it - "
			    "destination must be non-existing meta-url"),
		    nocopy);

  bool secure = false;
  options.AddOption('u', "secure",
		    istring("use secure transfer (insecure by default)"),
		    secure);

  std::string cache_path;
  options.AddOption('y', "cache",
		    istring("path to local cache (use to put file into cache)"),
		    istring("path"), cache_path);

  std::string cache_data_path;
  options.AddOption('Y', "cachedata",
		    istring("path for cache data (if different from -y)"),
		    istring("path"), cache_data_path);

  int recursion = 0;
  options.AddOption('r', "recursive",
		    istring("operate recursively up to specified level"),
		    istring("level"), recursion);

  int retries = 0;
  options.AddOption('R', "retries",
		    istring("number of retries before failing file transfer"),
		    istring("number"), retries);

  int timeout = 20;
  options.AddOption('t', "timeout", istring("timeout in seconds (default 20)"),
		    istring("seconds"), timeout);

  std::string conffile;
  options.AddOption('z', "conffile",
		    istring("configuration file (default ~/.arc/client.xml)"),
		    istring("filename"), conffile);

  std::string debug;
  options.AddOption('d', "debug",
		    istring("FATAL, ERROR, WARNING, INFO, DEBUG or VERBOSE"),
		    istring("debuglevel"), debug);

  bool version = false;
  options.AddOption('v', "version", istring("print version information"),
		    version);

  std::list<std::string> params = options.Parse(argc, argv);

  if (!debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));

  Arc::UserConfig usercfg(conffile);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }

  if (debug.empty() && usercfg.ConfTree()["Debug"]) {
    debug = (std::string)usercfg.ConfTree()["Debug"];
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));
  }

  if (version) {
    std::cout << Arc::IString("%s version %s", "arccp", VERSION) << std::endl;
    return 0;
  }

  if (params.size() != 2) {
    logger.msg(Arc::ERROR, "Wrong number of parameters specified");
    return 1;
  }

  if (passive && notpassive) {
    logger.msg(Arc::ERROR, "Options 'p' and 'n' can't be used simultaneously");
    return 1;
  }

  if ((!secure) && (!notpassive))
    passive = true;

  std::list<std::string>::iterator it = params.begin();
  std::string source = *it;
  ++it;
  std::string destination = *it;

  Arc::NS ns;
  Arc::XMLNode cred(ns, "cred");
  usercfg.ApplySecurity(cred);

  if (nocopy)
    arcregister(source, destination, cred, secure, passive, force, timeout);
  else
    arccp(source, destination, cred, secure, passive, force, recursion,
	  retries + 1, verbose, timeout);

  return 0;
}
