// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <list>

#include <arc/ArcLocation.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/client/UserConfig.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataMover.h>
#include <arc/OptionParser.h>

static Arc::Logger logger(Arc::Logger::getRootLogger(), "arcrm");

void arcrm(const Arc::URL& file_url,
           Arc::XMLNode credentials,
           bool errcont,
           int timeout) {
  if (!file_url) {
    logger.msg(Arc::ERROR, "Invalid URL: %s", file_url.str());
    return;
  }
  if (file_url.Protocol() == "filelist") {
    std::list<Arc::URL> files = Arc::ReadURLList(file_url);
    if (files.size() == 0) {
      logger.msg(Arc::ERROR, "Can't read list of locations from file %s",
                 file_url.Path());
      return;
    }
    for (std::list<Arc::URL>::iterator file = files.begin();
         file != files.end(); file++)
      arcrm(*file, credentials, errcont, timeout);
    return;
  }

  Arc::DataHandle url(file_url);
  if (!url) {
    logger.msg(Arc::ERROR, "Unsupported url given");
    return;
  }
  url->AssignCredentials(credentials);
  Arc::DataMover mover;
  mover.Delete(*url,errcont);
  return;
}

int main(int argc, char **argv) {

  setlocale(LC_ALL, "");

  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::ArcLocation::Init(argv[0]);

  Arc::OptionParser options(istring("url"),
                            istring("The arcrm command deletes files and on "
                                    "grid storage elements."));

  bool force = false;
  options.AddOption('f', "force",
                    istring("remove logical file name registration even "
                            "if not all physical instances were removed"),
                    force);

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

  Arc::UserConfig usercfg(conffile);
  if (!usercfg) {
    logger.msg(Arc::ERROR, "Failed configuration initialization");
    return 1;
  }

  if (!debug.empty())
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));
  else if (debug.empty() && usercfg.ConfTree()["Debug"]) {
    debug = (std::string)usercfg.ConfTree()["Debug"];
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(debug));
  }

  if (version) {
    std::cout << Arc::IString("%s version %s", "arcrm", VERSION) << std::endl;
    return 0;
  }

  // Proxy check
  //if (!usercfg.CheckProxy())
  //  return 1;

  if (params.size() != 1) {
    logger.msg(Arc::ERROR, "Wrong number of parameters specified");
    return 1;
  }

  Arc::NS ns;
  Arc::XMLNode cred(ns, "cred");
  usercfg.ApplyToConfig(cred);

  std::list<std::string>::iterator it = params.begin();
  arcrm(*it, cred, force, timeout);

  return 0;
}
