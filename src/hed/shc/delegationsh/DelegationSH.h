#ifndef __ARC_SEC_DELEGATIONSH_H__
#define __ARC_SEC_DELEGATIONSH_H__

#include <stdlib.h>

#include <arc/ArcConfig.h>
#include <arc/message/Message.h>
#include <arc/security/SecHandler.h>

namespace ArcSec {

/// 

class DelegationContext;

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
  std::string peers_endpoint_; //endpoint of the peer service, to which
                               //the real service invokation will be called.
                               //This variable is only valid for the client
                               //role Delegation handler.
  std::string delegation_id_; //The delegation ID which is used to 
                              //be send to the peer service side. The 
                              //variable is only valid for the client role
                              //Delegation handler.
                              //The client role delegation handler is only need
                              //to be set if it is configured in a client.
                              //If the client role Delegation handler is configured 
                              //in a service, then delegation_id_ delegation_id
                              //does not need to set.
  std::string delegation_cred_identity_;
  std::string cert_file_;
  std::string key_file_;
  std::string proxy_file_;
  std::string ca_file_;
  std::string ca_dir_;

  Arc::MessageContextElement* mcontext_;

 protected:
  static Arc::Logger logger;
 
 private:
  DelegationContext* get_delegcontext(Arc::Message& msg);

 public:
  DelegationSH(Arc::Config *cfg, Arc::ChainContext* ctx);
  virtual ~DelegationSH(void);
  static Arc::Plugin* get_sechandler(Arc::PluginArgument* arg);
  virtual bool Handle(Arc::Message* msg);  
};

} // namespace ArcSec

#endif /* __ARC_SEC_DELEGATIONSH_H__ */

