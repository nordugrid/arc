#ifndef __ARC_SEC_DELEGATIONSH_H__
#define __ARC_SEC_DELEGATIONSH_H__

#include <stdlib.h>

#include <arc/ArcConfig.h>
#include <arc/message/Message.h>
#include <arc/security/SecHandler.h>

namespace ArcSec {

/// 

class DelegationSH : public SecHandler {
 private:
  enum {
    delegation_client,
    delegation_service
  } delegation_role_;
  enum {
    delegation_x509,
    delegation_saml
  } delegation_type_;
  std::string ds_endpoint_; //endpoint of delegation service,
                            // to which this Sec handler will
                            // create a delegation credential
  std::string cert_file_;
  std::string key_file_;
  std::string ca_file_;
  std::string ca_dir_;

 public:
  DelegationSH(Arc::Config *cfg, Arc::ChainContext* ctx);
  virtual ~DelegationSH(void);
  static Arc::Plugin* get_sechandler(Arc::PluginArgument* arg);
  virtual bool Handle(Arc::Message* msg);  
};

} // namespace ArcSec

#endif /* __ARC_SEC_DELEGATIONSH_H__ */

