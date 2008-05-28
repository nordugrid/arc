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
#include <arc/data/DataHandle.h>
#include <arc/misc/OptionParser.h>

static Arc::Logger logger(Arc::Logger::getRootLogger(), "arcls");

void arcls(const Arc::URL& dir_url,
	   bool show_details,
	   bool show_urls,
	   int recursion,
	   int timeout) {
  if (dir_url.Protocol() == "urllist") {
    std::list<Arc::URL> dirs = Arc::ReadURLList(dir_url);
    if (dirs.size() == 0) {
      logger.msg(Arc::ERROR, "Can't read list of locations from file %s",
		 dir_url.Path());
      return;
    }
    for (std::list<Arc::URL>::iterator dir = dirs.begin();
	 dir != dirs.end(); dir++)
      arcls(*dir, show_details, show_urls, recursion, timeout);
    return;
  }

  Arc::DataHandle url(dir_url);
  if (!url) {
    logger.msg(Arc::ERROR, "Unsupported url given");
    return;
  }
  std::list<Arc::FileInfo> files;
  url->SetSecure(false);
  if (!url->ListFiles(files, show_details)) {
    if (files.size() == 0) {
      logger.msg(Arc::ERROR, "Failed listing metafiles");
      return;
    }
    logger.msg(Arc::INFO, "Warning: "
	       "Failed listing metafiles but some information is obtained");
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
    }
    std::cout << std::endl;
    if (show_urls)
      for (std::list<Arc::URL>::const_iterator u = i->GetURLs().begin();
	   u != i->GetURLs().end(); u++)
	std::cout << "\t" << *u << std::endl;
    // Do recursion
    if (recursion > 0)
      if (i->GetType() == Arc::FileInfo::file_type_dir) {
	Arc::URL suburl = dir_url;
	if (suburl.Path()[suburl.Path().length() - 1] != '/')
	  suburl.ChangePath(suburl.Path() + "/" + i->GetName());
	else
	  suburl.ChangePath(suburl.Path() + i->GetName());
	std::cout << suburl.str() << ":" << std::endl;
	arcls(suburl, show_details, show_urls, recursion - 1, timeout);
	std::cout << std::endl;
      }
  }
  return;
}

int main(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("url"));

  bool longlist = false;
  options.AddOption('l', "long", istring("long format (more information)"),
		    longlist);

  bool locations = false;
  options.AddOption('L', "locations", istring("show URLs of file locations"),
		    locations);

  int recursion = 0;
  options.AddOption('r', "recursive",
		    istring("operate recursively up to specified level"),
		    istring("level"), recursion);

  int timeout = 20;
  options.AddOption('t', "timeout", istring("timeout in seconds (default 20)"),
		    istring("seconds"), timeout);

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

  if (version) {
    std::cout << Arc::IString("%s version %s", "arcls", VERSION) << std::endl;
    return 0;
  }

  if (params.size() != 1) {
    logger.msg(Arc::ERROR, "Wrong number of parameters specified");
    return 1;
  }

  std::list<std::string>::iterator it = params.begin();
  arcls(*it, longlist, locations, recursion, timeout);

  return 0;
}
