#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ServiceISIS.h"

#include <arc/infosys/InfoRegister.h>


namespace Arc {

  ServiceISIS::ServiceISIS(Arc::Config* cfg):Service(cfg) {
      // Register to the ISIS
      Arc::InfoRegister inforeg((Arc::XMLNode&)(*cfg), this);
  }

} // namespace Arc
