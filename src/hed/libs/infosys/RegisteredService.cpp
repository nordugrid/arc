#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "RegisteredService.h"

namespace Arc {

  RegisteredService::RegisteredService(Arc::Config* cfg):Service(cfg),inforeg((Arc::XMLNode&)(*cfg), this) {
  }

} // namespace Arc
