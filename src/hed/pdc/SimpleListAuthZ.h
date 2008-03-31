#ifndef __ARC_SEC_SIMPLELISTAUTHZ_H__
#define __ARC_SEC_SIMPLELISTAUTHZ_H__

#include <stdlib.h>

#include <arc/ArcConfig.h>
#include <arc/message/Message.h>
#include <arc/security/SecHandler.h>
#include <arc/security/PDP.h>
#include <arc/loader/PDPFactory.h>

namespace ArcSec {

/// Tests message against list of PDPs
/** This class implements SecHandler interface. It's Handle() method runs provided 
  Message instance against all PDPs specified in configuration. If any of PDPs 
  returns positive result Handle() return true, otherwise false. */


class SimpleListAuthZ : public SecHandler {
 private:
  class PDPDesc {
   public:
    PDP* pdp;
    enum {
      breakOnAllow,
      breakOnDeny,
      breakAlways,
      breakNever
    } action;
    PDPDesc(const std::string& action,PDP* pdp);
  };
  typedef std::map<std::string,PDPDesc>  pdp_container_t;

  /** Link to Factory responsible for loading and creation of PDP objects */
  Arc::PDPFactory *pdp_factory;
  /** One Handler can include few PDP */
  pdp_container_t pdps_;

 protected:
  /** Create PDP according to conf info */
  bool MakePDPs(Arc::Config* cfg);

 public:
  SimpleListAuthZ(Arc::Config *cfg, Arc::ChainContext* ctx);
  virtual ~SimpleListAuthZ(void);
  
  /** Get authorization decision*/
  virtual bool Handle(Arc::Message* msg);  
};

} // namespace ArcSec

#endif /* __ARC_SEC_SIMPLELISTAUTHZ_H__ */

