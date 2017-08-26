#ifndef __GRIDFTPD_CONFIG_VO_H__
#define __GRIDFTPD_CONFIG_VO_H__

#include <string>
#include <list>

#include <arc/ArcConfigIni.h>

#include "../auth/auth.h"

namespace gridftpd {

  int config_vo(AuthUser& user,Arc::ConfigIni& sect,std::string& cmd,std::string& rest,Arc::Logger* logger = NULL);
  int config_vo(std::list<AuthVO>& vos,Arc::ConfigIni& sect,std::string& cmd,std::string& rest,Arc::Logger* logger = NULL);
  
} // namespace gridftpd

#endif // __GRIDFTPD_CONFIG_VO_H__
