#ifndef __ARC_SIMPLELISTAUTHZ_H__
#define __ARC_SIMPLELISTAUTHZ_H__

#include <stdlib.h>

#include "common/ArcConfig.h"
#include "../message/Message.h"
#include "Handler.h"
#include "PDP.h"
#include "../../libs/loader/PDPFactory.h"

namespace Arc {
class SimpleListAuthZ : public Handler {
 public:
  typedef std::map<std::string, PDP *>  pdp_container_t;

 private:
  /** Link to Factory responsible for loading and creation of MCC objects */
  PDPFactory *pdp_factory;
  /**One AuthZ Handeler can include a few PDP */
  pdp_container_t pdps_;

 public:
  SimpleListAuthZ(Arc::Config *cfg);
  virtual ~SimpleListAuthZ(void) { };
  
  /** PDP*/
  virtual void *MakePDP(Arc::Config* cfg);

  /** Get authorization decision*/
  virtual bool SecHandle(Message* msg);  
};

} // namespace Arc

#endif /* __ARC_SIMPLELISTAUTHZ_H__ */

