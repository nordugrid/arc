#ifndef __ARC_SEC_SIMPLELISTAUTHZ_H__
#define __ARC_SEC_SIMPLELISTAUTHZ_H__

#include <stdlib.h>

#include <arc/ArcConfig.h>
#include <arc/message/Message.h>
#include "SecHandler.h"
#include "PDP.h"
#include <arc/loader/PDPFactory.h>

namespace ArcSec {
class SimpleListAuthZ : public SecHandler {
 public:
  typedef std::map<std::string, PDP *>  pdp_container_t;

 private:
  /** Link to Factory responsible for loading and creation of PDP objects */
  Arc::PDPFactory *pdp_factory;
  /** One Handler can include few PDP */
  pdp_container_t pdps_;
  /** PDP*/
  void MakePDPs(Arc::Config* cfg);
 public:
  SimpleListAuthZ(Arc::Config *cfg, Arc::ChainContext* ctx);
  virtual ~SimpleListAuthZ(void);
  
  /** Get authorization decision*/
  virtual bool Handle(Arc::Message* msg);  
};

} // namespace ArcSec

#endif /* __ARC_SEC_SIMPLELISTAUTHZ_H__ */

