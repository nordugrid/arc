#ifndef __ARC_SIMPLELISTAUTHZ_H__
#define __ARC_SIMPLELISTAUTHZ_H__

#include <stdlib.h>

#include "common/ArcConfig.h"
#include "message/Message.h"
#include "SecHandler.h"
#include "PDP.h"
#include "loader/PDPFactory.h"

namespace Arc {
class SimpleListAuthZ : public SecHandler {
 public:
  typedef std::map<std::string, PDP *>  pdp_container_t;

 private:
  /** Link to Factory responsible for loading and creation of PDP objects */
  PDPFactory *pdp_factory;
  /** One Handler can include few PDP */
  pdp_container_t pdps_;
  /** PDP*/
  void MakePDPs(Arc::Config* cfg);
 public:
  SimpleListAuthZ(Arc::Config *cfg,ChainContext* ctx);
  virtual ~SimpleListAuthZ(void);
  
  /** Get authorization decision*/
  virtual bool Handle(Message* msg);  
};

} // namespace Arc

#endif /* __ARC_SIMPLELISTAUTHZ_H__ */

