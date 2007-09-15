#ifndef __ARC_DELEGATIONINTERFACE_H__
#define __ARC_DELEGATIONINTERFACE_H__

#include <string>

#include <arc/message/SOAPEnvelope.h>

namespace Arc {

/// Manages private key of delegation procedure.
/** */
class DelegationConsumer {
 protected:
  void* key_;
  bool Generate(void);
  void LogError(void);
 public:
  /** */
  DelegationConsumer(void);
  DelegationConsumer(const std::string& content);
  ~DelegationConsumer(void);
  operator bool(void) { return key_ != NULL; };
  bool operator!(void) { return key_ == NULL; };
  const std::string& ID(void);
  bool Backup(std::string& content);
  bool Restore(const std::string& content);
  bool Request(std::string& content);
  bool Acquire(std::string& content);
};

/// Manages creddentials of delegation issuer.
/** */
class DelegationProvider {
  void* key_;
  void* cert_;
  void* chain_;
  void LogError(void);
  void CleanError(void);
 public:
  DelegationProvider(const std::string& credentials);
  ~DelegationProvider(void);
  std::string Delegate(const std::string& request);
};


class DelegationConsumerSOAP: public DelegationConsumer {
 protected:
 public:
  DelegationConsumerSOAP(void);
  DelegationConsumerSOAP(const std::string& content);
  ~DelegationConsumerSOAP(void);
  bool DelegateCredentialsInit(const std::string& id,const SOAPEnvelope& in,SOAPEnvelope& out);
  bool UpdateCredentials(std::string& credentials,const SOAPEnvelope& in,SOAPEnvelope& out);
};

class DelegationProviderSOAP: public DelegationProvider {
 protected:
 public:
  DelegationProviderSOAP(const std::string& credentials);
  ~DelegationProviderSOAP(void);
  bool DelegateCredentialsInitRequest(SOAPEnvelope& req);
  bool DelegateCredentialsInitResponse(std::string& id,const SOAPEnvelope& resp);
  bool UpdateCredentialsRequest(std::string& credentials,const SOAPEnvelope& in,SOAPEnvelope& out);
  bool UpdateCredentials(std::string& credentials,const SOAPEnvelope& in,SOAPEnvelope& out);
};




} // namespace Arc


#endif /* __ARC_DELEGATIONINTERFACE_H__ */
