#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "conf_vo.h"

namespace gridftpd {

  int config_vo(AuthUser& user,Arc::ConfigIni& sect,std::string& cmd,std::string& rest,Arc::Logger* logger) {
    if(strcmp(sect.SectionMatch(),"userlist") != 0) return 1;
    if(sect.SubSection()[0] != '\0') return 1;
    if(cmd.length() == 0) return 1;
    std::string voname = sect.SectionIdentifier();
    std::string vofile;
    for(;;) {
      if(cmd == "outfile") {
        vofile=rest;
      };
      sect.ReadNext(cmd,rest);
      if(sect.SectionNew() || (cmd.length() == 0)) {
        if(voname.empty()) {
          logger->msg(Arc::WARNING, "Configuration section [userlist] is missing name.");
        } else {
          user.add_vo(voname,vofile);
        };
        if(cmd.length() == 0) return 1;
        if(strcmp(sect.SectionMatch(),"userlist") != 0) return 1;
        if(sect.SubSection()[0] != '\0') return 1;
        voname=""; vofile="";
      };
    };
    return 0;
  }

  int config_vo(std::list<AuthVO>& vos,Arc::ConfigIni& sect,std::string& cmd,std::string& rest,Arc::Logger* logger) {
    if(strcmp(sect.SectionMatch(),"userlist") != 0) return 1;
    if(sect.SubSection()[0] != '\0') return 1;
    if(cmd.length() == 0) return 1;
    std::string voname = sect.SectionIdentifier();
    std::string vofile;
    for(;;) {
      if(cmd == "outfile") {
        vofile=rest;
      };
      sect.ReadNext(cmd,rest);
      if(sect.SectionNew() || (cmd.length() == 0)) {
        if(voname.empty()) {
          logger->msg(Arc::WARNING, "Configuration section [userlist] is missing name.");
        } else {
          vos.push_back(AuthVO(voname,vofile));
        };
        if(cmd.length() == 0) return 1;
        if(strcmp(sect.SectionMatch(),"userlist") != 0) return 1;
        if(sect.SubSection()[0] != '\0') return 1;
        voname=""; vofile="";
      };
    };
    return 0;
  }

} // namespace gridftpd
