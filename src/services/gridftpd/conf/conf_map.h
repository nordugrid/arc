#ifndef __GRIDFTPD_CONFIG_MAP_H__
#define __GRIDFTPD_CONFIG_MAP_H__

#include <arc/data/URLMap.h>

#include "environment.h"

namespace gridftpd {

  /*
    Look URLMap.h for functionality.
    This object automatically reads configuration file
    and fills list of mapping for UrlMap.
  */
  class UrlMapConfig: public Arc::URLMap {
   public:
    UrlMapConfig(GMEnvironment& env);
    ~UrlMapConfig(void);
  };

} // namespace gridftpd

#endif // __GRIDFTPD_CONFIG_MAP_H__
