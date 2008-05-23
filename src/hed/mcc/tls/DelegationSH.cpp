#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <openssl/ssl.h>
#include <openssl/x509v3.h>

#include "PayloadTLSStream.h"
#include "DelegationSecAttr.h"

#include "DelegationSH.h"

namespace ArcSec {

using namespace Arc;

static Arc::Logger logger(Arc::Logger::getRootLogger(),"DelegationSH");

DelegationSH::DelegationSH(Config *cfg,ChainContext* ctx):SecHandler(cfg) {
}

DelegationSH::~DelegationSH(void) {
}


// TODO: In a future accept and store ALL policies. Let it fail at PDP level.
//       Currently code fails immediately if policy not recognized.
//       Alternatively behavior may be configurable.
static bool get_proxy_policy(X509* cert,DelegationMultiSecAttr* sattr) {
  bool result = false;
#ifdef HAVE_OPENSSL_PROXY
  PROXY_CERT_INFO_EXTENSION *pci = (PROXY_CERT_INFO_EXTENSION*)X509_get_ext_d2i(cert,NID_proxyCertInfo,NULL,NULL);
  if(!pci) return true; // No proxy 
  switch (OBJ_obj2nid(pci->proxyPolicy->policyLanguage)) {
    case NID_Independent: { // No rights granted
      // Either such situation should be disallowed or 
      // policy should be generated which granst no right for enything.
      // First option is easier to implement so using it at least yet.
      logger.msg(VERBOSE,"Independent proxy - no rights granted");
    }; break; 
    case NID_id_ppl_inheritAll: {
      // All right granted. No additional policies should be enforced.
      logger.msg(VERBOSE,"Proxy with all rights inherited");
      result=true;
    }; break;
    case NID_id_ppl_anyLanguage: { // Here we store ARC policy
      // Either this is ARC policy is determined by examining content
      const char* policy_str = (const char *)(pci->proxyPolicy->policy->data);
      int policy_length = pci->proxyPolicy->policy->length;
      if((policy_str == NULL) || (policy_length <= 0)) {
        logger.msg(VERBOSE,"Proxy with empty policy  - fail on unrecognized policy");
        break;
      };
      {
        std::string s(policy_str,policy_length);
        logger.msg(VERBOSE,"Proxy with specific policy: %s",s);
      };
      result = sattr->Add(policy_str,policy_length);
      if(result) {
        logger.msg(VERBOSE,"Proxy with ARC Policy");
      } else {
        logger.msg(VERBOSE,"Proxy with unknown policy  - fail on unrecognized policy");
      };
    };
    default: {
      // Unsupported policy - fail
    }; break;
  };
  PROXY_CERT_INFO_EXTENSION_free(pci);
#endif
  return result;
}

bool DelegationSH::Handle(Arc::Message* msg){
  DelegationMultiSecAttr* sattr = NULL;
  try {
    MessagePayload* mpayload = msg->Payload();
    if(!mpayload) return false; // No payload in this message
    PayloadTLSStream* tstream = dynamic_cast<PayloadTLSStream*>(msg->Payload());
    // Currently only TLS payloads are supported
    if(!tstream) return false;
    SecAttr* sattr_ = msg->Auth()->get("DELEGATION POLICY");
    if(sattr_) sattr=dynamic_cast<DelegationMultiSecAttr*>(sattr_);
    if(!sattr) sattr=new DelegationMultiSecAttr;
    X509* cert = tstream->GetPeerCert();
    if (cert != NULL) {
      if(!get_proxy_policy(cert,sattr)) {
        X509_free(cert);
        throw std::exception();
      };
      X509_free(cert);
    };
    STACK_OF(X509)* peerchain = tstream->GetPeerChain();
    if(peerchain != NULL) {
      for(int idx = 0;;++idx) {
        if(idx >= sk_X509_num(peerchain)) break;
        X509* cert = sk_X509_value(peerchain,idx);
        if(cert) {
          if(!get_proxy_policy(cert,sattr)) throw std::exception();
        };
      };
    };
    if(!sattr_) msg->Auth()->set("DELEGATION POLICY",sattr);
    return true;
  } catch(std::exception&) { };
  if(sattr) delete sattr;
  return false;
}


SecHandler* DelegationSH::get_sechandler(Arc::Config *cfg, Arc::ChainContext* ctx) {
    return new DelegationSH(cfg,ctx);
}

}

sechandler_descriptors ARC_SECHANDLER_LOADER = {
    { "delegation.collector", 0, &ArcSec::DelegationSH::get_sechandler},
    { NULL, 0, NULL }
};

