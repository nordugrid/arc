#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "RegisteredService.h"

namespace Arc {

  RegisteredService::RegisteredService(Config* cfg):Service(cfg),inforeg(*cfg, this) {
  }

} // namespace Arc
