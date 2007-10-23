#ifndef __ARC_DELEGATIONINTERFACE_H__
#define __ARC_DELEGATIONINTERFACE_H__

#include <string>
#include <list>
#include <map>

#include <glibmm/thread.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/message/MCC.h>
#include <arc/message/Message.h>

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
  bool DelegatedToken(std::string& credentials,const XMLNode& token);
};

class DelegationProviderSOAP: public DelegationProvider {
 protected:
  std::string request_;
  std::string id_;
 public:
  DelegationProviderSOAP(const std::string& credentials);
  ~DelegationProviderSOAP(void);
  bool DelegateCredentialsInit(MCCInterface& interface,MessageContext* context);
  bool DelegateCredentialsInit(MCCInterface& interface,MessageAttributes* attributes_in,MessageAttributes* attributes_out,MessageContext* context);
  bool UpdateCredentials(MCCInterface& interface,MessageContext* context);
  bool UpdateCredentials(MCCInterface& interface,MessageAttributes* attributes_in,MessageAttributes* attributes_out,MessageContext* context);
  bool DelegatedToken(XMLNode& parent);
};

class DelegationContainerSOAP {
 private:
  class Consumer;
  typedef std::map<std::string,Consumer> ConsumerMap;
  typedef ConsumerMap::iterator ConsumerIterator;
  ConsumerMap consumers_;
  ConsumerIterator consumers_first_;
  ConsumerIterator consumers_last_;
  void AddConsumer(const std::string& id,DelegationConsumerSOAP* consumer);
  void TouchConsumer(ConsumerIterator i);
  void RemoveConsumer(ConsumerIterator i);
  void CheckConsumers(void);
 protected:
  Glib::Mutex lock_;
  /** Max. number of delagation consumers */
  int max_size_;
  /** Lifetime of unused delegation consumer */
  int max_duration_;
  /** Max. times same delegation consumer may accept credentials */
  int max_usage_;
  /** If true delegation consumer is deleted when connection context is destroyed */
  bool context_lock_;
  /** If true all delegation phases must be performed by same identity */
  bool restricted_;
 public:
  DelegationContainerSOAP(void);
  ~DelegationContainerSOAP(void);
  bool DelegateCredentialsInit(const SOAPEnvelope& in,SOAPEnvelope& out);
  bool UpdateCredentials(std::string& credentials,const SOAPEnvelope& in,SOAPEnvelope& out);
  bool DelegatedToken(std::string& credentials,const XMLNode& token);
};

} // namespace Arc


#endif /* __ARC_DELEGATIONINTERFACE_H__ */
