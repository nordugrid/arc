#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "RegisteredService.h"

#include <arc/infosys/InfoRegister.h>


namespace Arc {

  RegisteredService::RegisteredService(Arc::Config* cfg):Service(cfg) {
      // Register to the ISIS
      Arc::InfoRegister inforeg((Arc::XMLNode&)(*cfg), this);
  }

} // namespace Arc
