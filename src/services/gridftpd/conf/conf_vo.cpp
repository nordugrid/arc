#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "environment.h"

#include "conf_vo.h"

namespace gridftpd {

  int config_vo(AuthUser& user,Arc::ConfigIni& sect,std::string& cmd,std::string& rest,Arc::Logger* logger) {
    if(strcmp(sect.SectionMatch(),"vo") != 0) return 1;
    if(cmd.length() == 0) return 1;
    std::string voname = sect.SubSection();
    std::string vofile;
    for(;;) {
      if((cmd == "name") || (cmd == "vo")) {
        voname=rest;
      } else if(cmd == "file") {
        vofile=rest;
      };
      sect.ReadNext(cmd,rest);
      if(sect.SectionNew() || (cmd.length() == 0)) {
        if(voname.empty()) {
          logger->msg(Arc::WARNING, "Configuration section [vo] is missing name. Check for presence of name= or vo= option.");
        } else {
          user.add_vo(voname,vofile);
        };
        if(cmd.length() == 0) return 1;
        if(strcmp(sect.SectionMatch(),"vo") != 0) return 1;
        voname=""; vofile="";
      };
    };
    return 0;
  }

  int config_vo(std::list<AuthVO>& vos,Arc::ConfigIni& sect,std::string& cmd,std::string& rest,Arc::Logger* logger) {
    if(strcmp(sect.SectionMatch(),"vo") != 0) return 1;
    if(cmd.length() == 0) return 1;
    std::string voname = sect.SubSection();
    std::string vofile;
    for(;;) {
      if((cmd == "name") || (cmd == "vo")) {
        voname=rest;
      } else if(cmd == "file") {
        vofile=rest;
      };
      sect.ReadNext(cmd,rest);
      if(sect.SectionNew() || (cmd.length() == 0)) {
        if(voname.empty()) {
          logger->msg(Arc::WARNING, "Configuration section [vo] is missing name. Check for presence of name= or vo= option.");
        } else {
          vos.push_back(AuthVO(voname,vofile));
        };
        if(cmd.length() == 0) return 1;
        if(strcmp(sect.SectionMatch(),"vo") != 0) return 1;
        voname=""; vofile="";
      };
    };
    return 0;
  }

  // vo name filename etc.
  int config_vo(AuthUser& user,const std::string& cmd,std::string& rest,Arc::Logger* logger) {
    if(cmd != "vo") return 1;
    std::string voname = Arc::ConfigIni::NextArg(rest);
    std::string vofile = Arc::ConfigIni::NextArg(rest);
    if(voname.empty()) {
      logger->msg(Arc::WARNING, "Configuration section [vo] is missing name. Check for presence of name= or vo= option.");
      return -1;
    };
    user.add_vo(voname,vofile);
    return 0;
  }

  int config_vo(std::list<AuthVO>& vos,const std::string& cmd,std::string& rest,Arc::Logger* logger) {
    if(cmd != "vo") return 1;
    std::string voname = Arc::ConfigIni::NextArg(rest);
    std::string vofile = Arc::ConfigIni::NextArg(rest);
    if(voname.empty()) {
      logger->msg(Arc::WARNING, "Configuration section [vo] is missing name. Check for presence of name= or vo= option.");
      return -1;
    };
    vos.push_back(AuthVO(voname,vofile));
    return 0;
  }

  int config_vo(AuthUser& user,const char* cmd,const char* rest,Arc::Logger* logger) {
    std::string cmd_(cmd);
    std::string rest_(rest);
    return config_vo(user,cmd_,rest_,logger);
  }

  int config_vo(std::list<AuthVO>& vos,const char* cmd,const char* rest,Arc::Logger* logger) {
    std::string cmd_(cmd);
    std::string rest_(rest);
    return config_vo(vos,cmd_,rest_,logger);
  }

} // namespace gridftpd
