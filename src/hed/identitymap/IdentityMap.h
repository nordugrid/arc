#ifndef __ARC_SEC_IDENTITYMAP_H__
#define __ARC_SEC_IDENTITYMAP_H__

#include <arc/ArcConfig.h>
#include <arc/message/Message.h>
#include <arc/security/SecHandler.h>
#include <arc/security/PDP.h>
#include <arc/loader/PDPFactory.h>

namespace ArcSec {

/// Apply Tests message against list of PDPs
/** This class implements SecHandler interface. It's Handle() method runs provided 
  Message instance against all PDPs specified in configuration. If any of PDPs 
  returns positive result Handle() return true, otherwise false. */
class IdentityMap : public SecHandler {
 private:

  typedef struct {
    PDP* pdp;
    std::string* uid;
  } map_pair_t;

  std::list<map_pair_t> maps_;

 public:
  IdentityMap(Arc::Config *cfg, Arc::ChainContext* ctx);
  virtual ~IdentityMap(void);
  
  /** Get authorization decision*/
  virtual bool Handle(Arc::Message* msg);  
};

} // namespace ArcSec

#endif /* __ARC_SEC_IDENTITYMAP_H__ */

