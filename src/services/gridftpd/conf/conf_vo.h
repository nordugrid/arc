#ifndef __GRIDFTPD_CONFIG_VO_H__
#define __GRIDFTPD_CONFIG_VO_H__

#include <string>
#include <list>

#include "../auth/auth.h"
#include "conf_sections.h"

namespace gridftpd {

  int config_vo(AuthUser& user,ConfigSections& sect,std::string& cmd,std::string& rest);
  int config_vo(AuthUser& user,const std::string& cmd,std::string& rest);
  int config_vo(AuthUser& user,const char* cmd,const char* rest);
  int config_vo(std::list<AuthVO>& vos,ConfigSections& sect,std::string& cmd,std::string& rest);
  int config_vo(std::list<AuthVO>& vos,const std::string& cmd,std::string& rest);
  int config_vo(std::list<AuthVO>& vos,const char* cmd,const char* rest);
  
} // namespace gridftpd

#endif // __GRIDFTPD_CONFIG_VO_H__
