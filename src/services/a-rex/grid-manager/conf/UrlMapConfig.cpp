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
  Arc::ConfigIni* cf = NULL;
  //if(nordugrid_config_loc().empty()) read_env_vars(true);
  if(!cfile.open(config.ConfigFile())) {
    glogger.msg(Arc::ERROR,"Can't open configuration file");
    return;
  };
  switch(cfile.detect()) {
    case Arc::ConfigFile::file_XML: {
      Arc::XMLNode cfg;
      if(!cfg.ReadFromStream(cfile)) {
        glogger.msg(Arc::ERROR,"Can't interpret configuration file as XML");
      } else {
        /*
         dataTransfer
           mapURL (link)
            from
            to
            at
         */
        Arc::XMLNode datanode = cfg["dataTransfer"];
        if(datanode) {
          Arc::XMLNode mapnode = datanode["mapURL"];
          for(;mapnode;++mapnode) {
            bool is_link = false;
            if(!Arc::Config::elementtobool(mapnode,"link",is_link)) {
              glogger.msg(Arc::ERROR,"Value for 'link' element in mapURL is incorrect");
              continue;
            };
            std::string initial = mapnode["from"];
            std::string replacement = mapnode["to"];
            if(initial.empty()) {
              glogger.msg(Arc::ERROR,"Missing 'from' element in mapURL");
              continue;
            };
            if(replacement.empty()) {
              glogger.msg(Arc::ERROR,"Missing 'to' element in mapURL");
              continue;
            };
            if(is_link) {
              std::string access = mapnode["at"];
              if(access.empty()) access = replacement;
              add(initial,replacement,access);
            } else {
              add(initial,replacement);
            };
          };
        };
      };
    }; break;
    case Arc::ConfigFile::file_INI: {
      cf=new Arc::ConfigIni(cfile);
      cf->AddSection("common");
      cf->AddSection("data-staging");
      for(;;) {
        std::string rest;
        std::string command;
        cf->ReadNext(command,rest);
        if(command.length() == 0) break;
        else if(command == "copyurl") {
          std::string initial = Arc::ConfigIni::NextArg(rest);
          std::string replacement = Arc::ConfigIni::NextArg(rest);
          if((initial.length() == 0) || (replacement.length() == 0)) {
            glogger.msg(Arc::ERROR,"Not enough parameters in copyurl");
            continue;
          };
          add(initial,replacement);
        }
        else if(command == "linkurl") {
          std::string initial = Arc::ConfigIni::NextArg(rest);
          std::string replacement = Arc::ConfigIni::NextArg(rest);
          if((initial.length() == 0) || (replacement.length() == 0)) {
            glogger.msg(Arc::ERROR,"Not enough parameters in linkurl");
            continue;
          };
          std::string access = Arc::ConfigIni::NextArg(rest);
          if(access.length() == 0) access = replacement;
          add(initial,replacement,access);
        };
      };
    }; break;
    default: {
      glogger.msg(Arc::ERROR,"Can't recognize type of configuration file");
    }; break;
  };
  cfile.close();
  if(cf) delete cf;
}

UrlMapConfig::~UrlMapConfig(void) {
}

} // namespace ARex
