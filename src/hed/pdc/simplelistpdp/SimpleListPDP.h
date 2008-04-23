#ifndef __ARC_SEC_SIMPLEPDP_H__
#define __ARC_SEC_SIMPLEPDP_H__

#include <stdlib.h>

#include <arc/ArcConfig.h>
#include <arc/security/PDP.h>

namespace ArcSec {

/// Tests X509 subject against list of subjects in file
/** This class implements PDP interface. It's isPermitted() method
  compares X590 subject of requestor obtained from TLS layer (TLS:PEERDN)
  to list of subjects (ne per line) in external file. Locations of 
  file is defined by 'location' attribute of PDP caonfiguration.
  Returns true if subject is present in list, otherwise false. */
class SimpleListPDP : public PDP {
 public:
  static PDP* get_simplelist_pdp(Arc::Config *cfg, Arc::ChainContext *ctx);
  SimpleListPDP(Arc::Config* cfg);
  virtual ~SimpleListPDP() {};
  virtual bool isPermitted(Arc::Message *msg);
 private:
  std::string location;
 protected:
  static Arc::Logger logger;
};

} // namespace ArcSec

#endif /* __ARC_SEC_SIMPLELISTPDP_H__ */
