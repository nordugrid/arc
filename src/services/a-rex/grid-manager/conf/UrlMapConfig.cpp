#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <fstream>

#include <arc/Logger.h>

#include <arc/ArcConfig.h>
#include <arc/ArcConfigFile.h>
#include <arc/ArcConfigIni.h>

#include "UrlMapConfig.h"

namespace ARex {

static Arc::Logger& glogger = Arc::Logger::getRootLogger();

UrlMapConfig::UrlMapConfig(const GMConfig& config) {
  Arc::ConfigFile cfile;

  if (!cfile.open(config.ConfigFile())) {
    glogger.msg(Arc::ERROR,"Can't open configuration file");
    return;
  }

  if (cfile.detect() != Arc::ConfigFile::file_INI) {
    glogger.msg(Arc::ERROR,"Can't recognize type of configuration file");
    cfile.close();
    return;
  }

  Arc::ConfigIni cf(cfile);
  cf.AddSection("arex/data-staging");
  for (;;) {
    std::string rest;
    std::string command;
    cf.ReadNext(command, rest);
    if (command.empty()) break;
    if (command == "copyurl") {
      std::string initial = Arc::ConfigIni::NextArg(rest);
      std::string replacement = rest;
      if ((initial.length() == 0) || (replacement.length() == 0)) {
        glogger.msg(Arc::ERROR,"Not enough parameters in copyurl");
        continue;
      }
      add(initial,replacement);
    }
    else if (command == "linkurl") {
      std::string initial = Arc::ConfigIni::NextArg(rest);
      std::string replacement = Arc::ConfigIni::NextArg(rest);
      if ((initial.length() == 0) || (replacement.length() == 0)) {
        glogger.msg(Arc::ERROR,"Not enough parameters in linkurl");
        continue;
      }
      std::string access = rest;
      if (access.length() == 0) access = replacement;
      add(initial,replacement,access);
    }
  }
  cfile.close();
}

UrlMapConfig::~UrlMapConfig(void) {
}

} // namespace ARex
