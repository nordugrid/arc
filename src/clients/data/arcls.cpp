// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <string>
#include <list>

#include <arc/ArcLocation.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/credential/Credential.h>
#include <arc/data/DataHandle.h>
#include <arc/OptionParser.h>

static Arc::Logger logger(Arc::Logger::getRootLogger(), "arcls");

bool arcls(const Arc::URL& dir_url,
           const Arc::UserConfig& usercfg,
           bool show_details,
           bool show_urls,
           bool show_meta,
           int recursion,
           int timeout) {
  if (!dir_url) {
    logger.msg(Arc::ERROR, "Invalid URL: %s", dir_url.fullstr());
    return false;
  }
  if (dir_url.Protocol() == "urllist") {
    std::list<Arc::URL> dirs = Arc::ReadURLList(dir_url);
    if (dirs.size() == 0) {
      logger.msg(Arc::ERROR, "Can't read list of locations from file %s",
                 dir_url.Path());
      return false;
    }
    bool r = true;
    for (std::list<Arc::URL>::iterator dir = dirs.begin();
         dir != dirs.end(); dir++)
      if(!arcls(*dir, usercfg, show_details, show_urls, show_meta,
               recursion, timeout)) r = false;
    return r;
  }

  if (dir_url.IsSecureProtocol() && !Arc::Credential::IsCredentialsValid(usercfg)) {
    logger.msg(Arc::ERROR, "Unable to list content of %s: No valid credentials found", dir_url.str());
    return false;
  }

  Arc::DataHandle url(dir_url, usercfg);
  if (!url) {
    logger.msg(Arc::ERROR, "Unsupported URL given");
    return false;
  }
  std::list<Arc::FileInfo> files;
  url->SetSecure(false);
  Arc::DataStatus res = url->ListFiles(files, (show_details || recursion > 0), show_urls, show_meta);
  if (!res.Passed()) {
    if (files.size() == 0) {
      logger.msg(Arc::ERROR, "Failed listing metafiles");
      if (res.Retryable())
        logger.msg(Arc::ERROR, "This seems like a temporary error, please try again later");
      return false;
    }
    logger.msg(Arc::INFO, "Warning: "
               "Failed listing metafiles but some information is obtained");
  }
  for (std::list<Arc::FileInfo>::iterator i = files.begin();
       i != files.end(); i++) {
    if(!show_meta || show_details || show_urls) std::cout << i->GetName();
    if (show_details) {
      switch (i->GetType()) {
      case Arc::FileInfo::file_type_file:
        std::cout << " file";
        break;

      case Arc::FileInfo::file_type_dir:
        std::cout << " dir";
        break;

      default:
        std::cout << " unknown";
        break;
      }
      if (i->CheckSize())
        std::cout << " " << i->GetSize();
      else
        std::cout << " *";
      if (i->CheckCreated())
        std::cout << " " << i->GetCreated();
      else
        std::cout << " *";
      if (i->CheckValid())
        std::cout << " " << i->GetValid();
      else
        std::cout << " *";
      if (i->CheckCheckSum())
        std::cout << " " << i->GetCheckSum();
      else
        std::cout << " *";
      if (i->CheckLatency())
        std::cout << " " << i->GetLatency();
    }
    if(!show_meta || show_details || show_urls) std::cout << std::endl;
    if (show_urls)
      for (std::list<Arc::URL>::const_iterator u = i->GetURLs().begin();
           u != i->GetURLs().end(); u++)
        std::cout << "\t" << *u << std::endl;
    if (show_meta) {
      std::map<std::string, std::string> md = i->GetMetaData();
      for (std::map<std::string, std::string>::iterator mi = md.begin(); mi != md.end(); ++mi)
        std::cout<<mi->first<<":"<<mi->second<<std::endl;
    }
    // Do recursion
    else if (recursion > 0)
      if (i->GetType() == Arc::FileInfo::file_type_dir) {
        Arc::URL suburl = dir_url;
        if (suburl.Path()[suburl.Path().length() - 1] != '/')
          suburl.ChangePath(suburl.Path() + "/" + i->GetName());
        else
          suburl.ChangePath(suburl.Path() + i->GetName());
        std::cout << suburl.str() << ":" << std::endl;
        arcls(suburl, usercfg, show_details, show_urls, show_meta, recursion - 1, timeout);
        std::cout << std::endl;
      }
  }
  return true;
}

int main(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("url"),
                            istring("The arcls command is used for listing "
                                    "files in grid storage elements "
                                    "and file\nindex catalogues."));

  bool longlist = false;
  options.AddOption('l', "long", istring("long format (more information)"),
                    longlist);

  bool locations = false;
  options.AddOption('L', "locations", istring("show URLs of file locations"),
                    locations);

  bool metadata = false;
  options.AddOption('m', "metadata", istring("display all available metadata"),
        metadata);

  int recursion = 0;
  options.AddOption('r', "recursive",
                    istring("operate recursively up to specified level"),
                    istring("level"), recursion);

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
    std::cout << Arc::IString("%s version %s", "arcls", VERSION) << std::endl;
    return 0;
  }

  // If debug is specified as argument, it should be set before loading the configuration.
  if (!debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));

  Arc::UserConfig usercfg(conffile);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }
  usercfg.UtilsDirPath(Arc::UserConfig::ARCUSERDIRECTORY);

  if (debug.empty() && !usercfg.Verbosity().empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(usercfg.Verbosity()));

  if (params.size() != 1) {
    logger.msg(Arc::ERROR, "Wrong number of parameters specified");
    return 1;
  }

  std::list<std::string>::iterator it = params.begin();

  if(!arcls(*it, usercfg, longlist, locations, metadata, recursion, timeout))
    return 1;

  return 0;
}
