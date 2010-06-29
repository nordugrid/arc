#include "conf.h"
#include "environment.h"

#include "conf_vo.h"

namespace gridftpd {

  int config_vo(AuthUser& user,ConfigSections& sect,std::string& cmd,std::string& rest) {
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
        if(voname.length() && vofile.length()) {
          user.add_vo(voname,vofile);
        };
        if(cmd.length() == 0) return 1;
        if(strcmp(sect.SectionMatch(),"vo") != 0) return 1;
        voname=""; vofile="";
      };
    };
  }

  int config_vo(std::list<AuthVO>& vos,ConfigSections& sect,std::string& cmd,std::string& rest) {
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
        if(voname.length() && vofile.length()) {
          vos.push_back(AuthVO(voname,vofile));
        };
        if(cmd.length() == 0) return 1;
        if(strcmp(sect.SectionMatch(),"vo") != 0) return 1;
        voname=""; vofile="";
      };
    };
  }

  // vo name filename etc.
  int config_vo(AuthUser& user,const std::string& cmd,std::string& rest) {
    if(cmd != "vo") return 1;
    std::string voname = config_next_arg(rest);
    std::string vofile = config_next_arg(rest);
    if((voname.length() == 0) || (vofile.length() == 0)) {
      return -1;
    };
    user.add_vo(voname,vofile);
    return 0;
  }

  int config_vo(std::list<AuthVO>& vos,const std::string& cmd,std::string& rest) {
    if(cmd != "vo") return 1;
    std::string voname = config_next_arg(rest);
    std::string vofile = config_next_arg(rest);
    if((voname.length() == 0) || (vofile.length() == 0)) {
      return -1;
    };
    vos.push_back(AuthVO(voname,vofile));
    return 0;
  }

  int config_vo(AuthUser& user,const char* cmd,const char* rest) {
    std::string cmd_(cmd);
    std::string rest_(rest);
    return config_vo(user,cmd_,rest_);
  }

  int config_vo(std::list<AuthVO>& vos,const char* cmd,const char* rest) {
    std::string cmd_(cmd);
    std::string rest_(rest);
    return config_vo(vos,cmd_,rest_);
  }

} // namespace gridftpd
