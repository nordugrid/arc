#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "RegisteredService.h"

namespace Arc {

  RegisteredService::RegisteredService(Config* cfg, PluginArgument* parg):
      Service(cfg, parg),inforeg(*cfg, this)
  {
  }

  RegisteredService::~RegisteredService(void) {
  }
} // namespace Arc
