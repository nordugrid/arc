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
#include <arc/data/DataPoint.h>
#include <arc/OptionParser.h>

static Arc::Logger logger(Arc::Logger::getRootLogger(), "arcls");

static bool arcls(const Arc::URL& dir_url,
           const Arc::UserConfig& usercfg,
           bool show_details, // longlist
           bool show_urls,    // locations
           bool show_meta,    // metadata
           bool no_list,      // don't list dirs
           bool force_list,   // force dir list
           bool check_access, // checkaccess
           int recursion,     // recursion 
           int timeout) {     // timeout
  bool show_header = show_details;
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
         dir != dirs.end(); dir++) {
      if(!arcls(*dir, usercfg, show_details, show_urls, show_meta,
               no_list, force_list, check_access, recursion, timeout)) r = false;
    }
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

  if(check_access) {
    std::cout << dir_url << " - ";
    if(url->Check()) {
      std::cout << "passed" << std::endl;
      return true;
    } else {
      std::cout << "failed" << std::endl;
      return false;
    }
  }

  Arc::DataPoint::DataPointInfoType verb = (Arc::DataPoint::DataPointInfoType)
                                           (Arc::DataPoint::INFO_TYPE_MINIMAL |
                                            Arc::DataPoint::INFO_TYPE_NAME);
  if(show_urls) verb = (Arc::DataPoint::DataPointInfoType)
                       (verb | Arc::DataPoint::INFO_TYPE_STRUCT);
  if(show_meta) verb = (Arc::DataPoint::DataPointInfoType)
                       (verb | Arc::DataPoint::INFO_TYPE_ALL);
  if(show_details) verb = (Arc::DataPoint::DataPointInfoType)
                          (verb |
                           Arc::DataPoint::INFO_TYPE_TYPE |
                           Arc::DataPoint::INFO_TYPE_TIMES |
                           Arc::DataPoint::INFO_TYPE_CONTENT |
                           Arc::DataPoint::INFO_TYPE_ACCESS); 
  if(recursion > 0) verb = (Arc::DataPoint::DataPointInfoType)
                           (verb | Arc::DataPoint::INFO_TYPE_TYPE);

  Arc::DataStatus res;
  Arc::FileInfo file;
  std::list<Arc::FileInfo> files;
  url->SetSecure(false);



  if(no_list) { // only requested object is queried
    res = url->Stat(file, verb);
    if(res) files.push_back(file);
  } else if(force_list) { // assume it is directory, fail otherwise
    res = url->List(files, verb);
  } else { // try to guess what to do
    res = url->Stat(file, (Arc::DataPoint::DataPointInfoType)(verb | Arc::DataPoint::INFO_TYPE_TYPE));
    if(res && (file.GetType() == Arc::FileInfo::file_type_file)) {
      // If it is file and we are sure, then just report it.
      files.push_back(file);
    } else {
      // If it is dir then we must list it. But if stat failed or
      // if type is undefined there is still chance it is directory.
      Arc::DataStatus res_ = url->List(files, verb);
      if(!res_) {
        // If listing failed maybe simply report previous result if any.
        if(res) {
          files.push_back(file);
        } else {
          res = res_;
        }
      }
    }
  }
  if (!res) {
    if (files.size() == 0) {
      logger.msg(Arc::ERROR, "Failed listing files");
      if (res.Retryable())
        logger.msg(Arc::ERROR, "This seems like a temporary error, please try again later");
      return false;
    }
    logger.msg(Arc::INFO, "Warning: "
               "Failed listing files but some information is obtained");
  }
  if (show_header && files.size()) {
    std::cout << "<Name>";
    if (show_details) {
      std::cout << " <Type>";
      std::cout << " <Size>";
      std::cout << " <Creation>";
      std::cout << " <Validity>";
      std::cout << " <CheckSum>";
      std::cout << " <Latency>";
    }
    std::cout << std::endl;
  }
  for (std::list<Arc::FileInfo>::iterator i = files.begin();
       i != files.end(); i++) {
    std::cout << i->GetName();
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
        std::cout << " (n/a)";
      if (i->CheckCreated())
        std::cout << " " << i->GetCreated();
      else
        std::cout << " (n/a)";
      if (i->CheckValid())
        std::cout << " " << i->GetValid();
      else
        std::cout << " (n/a)";
      if (i->CheckCheckSum())
        std::cout << " " << i->GetCheckSum();
      else
        std::cout << " (n/a)";
      if (i->CheckLatency())
        std::cout << " " << i->GetLatency();
      else
        std::cout << " (n/a)";
    }
    //if(!show_meta || show_details || show_urls) std::cout << std::endl;
    std::cout << std::endl;
    if (show_urls) {
      for (std::list<Arc::URL>::const_iterator u = i->GetURLs().begin();
           u != i->GetURLs().end(); u++)
        std::cout << "\t" << *u << std::endl;
    }
    if (show_meta) {
      std::map<std::string, std::string> md = i->GetMetaData();
      for (std::map<std::string, std::string>::iterator mi = md.begin(); mi != md.end(); ++mi)
        std::cout<<mi->first<<":"<<mi->second<<std::endl;
    }
    // Do recursion. Recursion has no sense if listing is forbidden.
    if ((recursion > 0) && (!no_list)) {
      if (i->GetType() == Arc::FileInfo::file_type_dir) {
        Arc::URL suburl = dir_url;
        if(suburl.Protocol() != "file") {
          if (suburl.Path()[suburl.Path().length() - 1] != '/')
            suburl.ChangePath(suburl.Path() + "/" + i->GetName());
          else
            suburl.ChangePath(suburl.Path() + i->GetName());
        } else {
          if (suburl.Path()[suburl.Path().length() - 1] != G_DIR_SEPARATOR)
            suburl.ChangePath(suburl.Path() + G_DIR_SEPARATOR_S + i->GetName());
          else
            suburl.ChangePath(suburl.Path() + i->GetName());
        }
        std::cout << std::endl;
        std::cout << suburl.str() << ":" << std::endl;
        arcls(suburl, usercfg, show_details, show_urls, show_meta,
              no_list, force_list, check_access, recursion - 1, timeout);
        std::cout << std::endl;
      }
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

  bool nolist = false;
  options.AddOption('n', "nolist", istring("show only description of requested object, do not list content of directories"),
        nolist);

  bool forcelist = false;
  options.AddOption('f', "forcelist", istring("treat requested object as directory and always try to list content"),
        forcelist);

  bool checkaccess = false;
  options.AddOption('c', "checkaccess", istring("check readability of object, does not show any information about object"),
        checkaccess);

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

  // Analyze options

  if (params.size() != 1) {
    logger.msg(Arc::ERROR, "Wrong number of parameters specified");
    return 1;
  }

  if(forcelist && nolist) {
    logger.msg(Arc::ERROR, "Incompatible options --nolist and --forcelist requested");
    return 1;
  }

  if(recursion && nolist) {
    logger.msg(Arc::ERROR, "Requesting recursion and --nolist has no sense");
    return 1;
  }

  std::list<std::string>::iterator it = params.begin();

  if(!arcls(*it, usercfg, longlist, locations, metadata,
            nolist, forcelist, checkaccess, recursion, timeout)) return 1;

  return 0;
}
