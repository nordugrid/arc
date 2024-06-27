// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <string>
#include <list>
#include <math.h>

#include <unistd.h>

#include <arc/ArcLocation.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/credential/Credential.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataPoint.h>
#include <arc/loader/FinderLoader.h>
#include <arc/OptionParser.h>

#include "utils.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(), "arcls");


void print_urls(const Arc::FileInfo& file) {
  for (std::list<Arc::URL>::const_iterator u = file.GetURLs().begin();
       u != file.GetURLs().end(); ++u)
    std::cout << "\t" << *u << std::endl;
}

void print_meta(const Arc::FileInfo& file) {
  std::map<std::string, std::string> md = file.GetMetaData();
  for (std::map<std::string, std::string>::iterator mi = md.begin(); mi != md.end(); ++mi)
    std::cout<<mi->first<<":"<<mi->second<<std::endl;
}

// formatted output of details when long list is requested
void print_details(const std::list<Arc::FileInfo>& files, bool show_urls, bool show_meta) {

  if (files.empty()) return;

  unsigned int namewidth = 0;
  unsigned int sizewidth = 0;
  unsigned int csumwidth = 0;

  // find longest length of each field to align the output
  for (std::list<Arc::FileInfo>::const_iterator i = files.begin();
      i != files.end(); ++i) {
    if (i->GetName().length() > namewidth) namewidth = i->GetName().length();
    if (i->CheckSize() && i->GetSize() > 0 && // log(0) not good!
        (unsigned int)(log10(i->GetSize()))+1 > sizewidth) sizewidth = (unsigned int)(log10(i->GetSize()))+1;
    if (i->CheckCheckSum() && i->GetCheckSum().length() > csumwidth) csumwidth = i->GetCheckSum().length();
  }
  std::cout << std::setw(namewidth) << std::left << "<Name> ";
  std::cout << "<Type>  ";
  std::cout << std::setw(sizewidth + 4) << std::left << "<Size>     ";
  std::cout << "<Modified>      ";
  std::cout << "<CheckSum> ";
  std::cout << std::setw(csumwidth) << std::right << "<Latency>";
  std::cout << std::endl;

  // set minimum widths to accommodate headers
  if (namewidth < 7) namewidth = 7;
  if (sizewidth < 7) sizewidth = 7;
  if (csumwidth < 8) csumwidth = 8;
  for (std::list<Arc::FileInfo>::const_iterator i = files.begin();
       i != files.end(); ++i) {
    std::cout << std::setw(namewidth) << std::left << i->GetName();
    switch (i->GetType()) {
      case Arc::FileInfo::file_type_file:
        std::cout << "  file";
        break;

      case Arc::FileInfo::file_type_dir:
        std::cout << "   dir";
        break;

      default:
        std::cout << " (n/a)";
        break;
    }
    if (i->CheckSize()) {
      std::cout << " " << std::setw(sizewidth) << std::right << Arc::tostring(i->GetSize());
    } else {
      std::cout << " " << std::setw(sizewidth) << std::right << "  (n/a)";
    }
    if (i->CheckModified()) {
      std::cout << " " << i->GetModified();
    } else {
      std::cout << "       (n/a)        ";
    }
    if (i->CheckCheckSum()) {
      std::cout << " " << std::setw(csumwidth) << std::left << i->GetCheckSum();
    } else {
      std::cout << " " << std::setw(csumwidth) << std::left << "   (n/a)";
    }
    if (i->CheckLatency()) {
      std::cout << "    " << i->GetLatency();
    } else {
      std::cout << "      (n/a)";
    }
    std::cout << std::endl;
    if (show_urls) print_urls(*i);
    if (show_meta) print_meta(*i);
  }
}

static bool arcls(const Arc::URL& dir_url,
           Arc::UserConfig& usercfg,
           bool show_details, // longlist
           bool show_urls,    // locations
           bool show_meta,    // metadata
           bool no_list,      // don't list dirs
           bool force_list,   // force dir list
           bool check_access, // checkaccess
           int recursion,     // recursion 
           int timeout) {     // timeout

  if (!dir_url) {
    logger.msg(Arc::ERROR, "Invalid URL: %s", dir_url.fullstr());
    return false;
  }
  if (dir_url.Protocol() == "urllist") {
    std::list<Arc::URL> dirs = Arc::ReadURLList(dir_url);
    if (dirs.empty()) {
      logger.msg(Arc::ERROR, "Can't read list of locations from file %s",
                 dir_url.Path());
      return false;
    }
    bool r = true;
    for (std::list<Arc::URL>::iterator dir = dirs.begin();
         dir != dirs.end(); ++dir) {
      if(!arcls(*dir, usercfg, show_details, show_urls, show_meta,
               no_list, force_list, check_access, recursion, timeout)) r = false;
    }
    return r;
  }

  Arc::DataHandle url(dir_url, usercfg);
  if (!url) {
    logger.msg(Arc::ERROR, "Unsupported URL given");
    return false;
  }
  if (url->RequiresCredentials()) {
    if (!usercfg.InitializeCredentials(Arc::initializeCredentialsType::RequireCredentials)) {
      logger.msg(Arc::ERROR, "Unable to list content of %s", dir_url.str());
      logger.msg(Arc::ERROR, "Invalid credentials, please check proxy and/or CA certificates");
      return false;
    }
    Arc::Credential holder(usercfg);
    if (!holder.IsValid()) {
      if (holder.GetEndTime() < Arc::Time()) {
        logger.msg(Arc::ERROR, "Proxy expired");
      }
      logger.msg(Arc::ERROR, "Unable to list content of %s", dir_url.str());
      logger.msg(Arc::ERROR, "Invalid credentials, please check proxy and/or CA certificates");
      return false;
    }
  }
  url->SetSecure(false);

  if(check_access) {
    std::cout << dir_url << " - ";
    if(url->Check(false)) {
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
                           Arc::DataPoint::INFO_TYPE_CKSUM |
                           Arc::DataPoint::INFO_TYPE_ACCESS); 
  if(recursion > 0) verb = (Arc::DataPoint::DataPointInfoType)
                           (verb | Arc::DataPoint::INFO_TYPE_TYPE);

  Arc::DataStatus res;
  Arc::FileInfo file;
  std::list<Arc::FileInfo> files;

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
        }
      } else {
        res = res_;
      }
    }
  }
  if (!res) {
    if (files.empty()) {
      logger.msg(Arc::ERROR, std::string(res));
      if (res.Retryable())
        logger.msg(Arc::ERROR, "This seems like a temporary error, please try again later");
      return false;
    }
    logger.msg(Arc::INFO, "Warning: "
               "Failed listing files but some information is obtained");
  }

  files.sort(); // Sort alphabetically by name
  if (show_details) {
    print_details(files, show_urls, show_meta);
  } else {
    for (std::list<Arc::FileInfo>::iterator i = files.begin();
       i != files.end(); ++i) {
      std::cout << i->GetName() << std::endl;
      if (show_urls) print_urls(*i);
      if (show_meta) print_meta(*i);
    }
  }
  // Do recursion. Recursion has no sense if listing is forbidden.
  if ((recursion > 0) && (!no_list)) {
    for (std::list<Arc::FileInfo>::iterator i = files.begin();
       i != files.end(); ++i) {
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

static int runmain(int argc, char **argv) {

  setlocale(LC_ALL, "");

  static Arc::LogStream logcerr(std::cerr);
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

  bool infinite_recursion = false;
  options.AddOption('r', "recursive",
                    istring("operate recursively"),
                    infinite_recursion);

  int recursion = 0;
  options.AddOption('D', "depth",
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
    std::cout << Arc::IString("%s version %s", "arcls", VERSION) << std::endl;
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

  if (debug.empty() && !usercfg.Verbosity().empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(usercfg.Verbosity()));

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
  if(infinite_recursion) recursion = INT_MAX;

  std::list<std::string>::iterator it = params.begin();

  if(!arcls(*it, usercfg, longlist, locations, metadata,
            nolist, forcelist, checkaccess, recursion, timeout)) return 1;

  return 0;
}

int main(int argc, char **argv) {
  int xr = runmain(argc,argv);
  _exit(xr);
  return 0;
}

