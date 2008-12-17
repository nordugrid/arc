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

typedef std::map<std::string,std::string> DelegationRestrictions;

/** A consumer of delegated X509 credentials.
  During delegation procedure this class acquires
 delegated credentials aka proxy - certificate, private key and
 chain of previous certificates. 
  Delegation procedure consists of calling Request() method
 for generating certificate request followed by call to Acquire()
 method for making complete credentials from certificate chain. */
class DelegationConsumer {
 protected:
  void* key_; /** Private key */
  bool Generate(void); /** Creates private key */
  void LogError(void);
 public:
  /** Creates object with new private key */
  DelegationConsumer(void);
  /** Creates object with provided private key */
  DelegationConsumer(const std::string& content);
  ~DelegationConsumer(void);
  operator bool(void) { return key_ != NULL; };
  bool operator!(void) { return key_ == NULL; };
  /** Return identifier of this object - not implemented */
  const std::string& ID(void);
  /** Stores content of this object into a string */
  bool Backup(std::string& content);
  /** Restores content of object from string */
  bool Restore(const std::string& content);
  /** Make X509 certificate request from internal private key */
  bool Request(std::string& content);
  /** Ads private key into certificates chain in 'content'
     On exit content contains complete delegated credentials.  */
  bool Acquire(std::string& content);
};

/** A provider of delegated credentials.
  During delegation procedure this class generates new credential
 to be used in proxy/delegated credential. */
class DelegationProvider {
  void* key_; /** Private key used to sign delegated certificate */
  void* cert_; /** Public key/certificate corresponding to public key */
  void* chain_; /** Chain of other certificates needed to verify 'cert_' if any */
  void LogError(void);
  void CleanError(void);
 public:
  /** Creates instance from provided credentials.
     Credentials are used to sign delegated credentials.
     Arguments should contain PEM-encoded certificate, private key and 
     optionally certificates chain. */
  DelegationProvider(const std::string& credentials);
  /** Creates instance from provided credentials.
     Credentials are used to sign delegated credentials.
     Arguments should contain filesystem path to PEM-encoded certificate and
     private key. Optionally cert_file may contain certificates chain. */
  DelegationProvider(const std::string& cert_file,const std::string& key_file,std::istream* inpwd = NULL);
  ~DelegationProvider(void);
  operator bool(void) { return key_ != NULL; };
  bool operator!(void) { return key_ == NULL; };
  /** Perform delegation.
    Takes X509 certificate request and creates proxy credentials
   excluding private key. Result is then fed into DelegationConsumer::Acquire */
  std::string Delegate(const std::string& request,const DelegationRestrictions& restrictions = DelegationRestrictions());
};

/** This class extends DelegationConsumer to support SOAP message exchange.
   Implements WS interface http://www.nordugrid.org/schemas/delegation
  described in delegation.wsdl. */
class DelegationConsumerSOAP: public DelegationConsumer {
 protected:
 public:
  /** Creates object with new private key */
  DelegationConsumerSOAP(void);
  /** Creates object with specified private key */
  DelegationConsumerSOAP(const std::string& content);
  ~DelegationConsumerSOAP(void);
  /** Process SOAP message which starts delagation.
    Generated message in 'out' is meant to be sent back to DelagationProviderSOAP.
   Argument 'id' contains identifier of procedure and is used only to produce SOAP 
   message. */
  bool DelegateCredentialsInit(const std::string& id,const SOAPEnvelope& in,SOAPEnvelope& out);
  /** Accepts delegated credentials.
     Process 'in' SOAP message and stores full proxy credentials in 'credentials'. 
    'out' message is genarated for sending to DelagationProviderSOAP. */
  bool UpdateCredentials(std::string& credentials,const SOAPEnvelope& in,SOAPEnvelope& out);
  /** Similar to UpdateCredentials but takes only DelegatedToken XML element */ 
  bool DelegatedToken(std::string& credentials,const XMLNode& token);
};

/** Extension of DelegationProvider with SOAP exchange interface. 
  This class is also a temporary container for intermediate information
 used during delegation procedure. */
class DelegationProviderSOAP: public DelegationProvider {
 protected:
  std::string request_;
  std::string id_;
 public:
  /** Creates instance from provided credentials.
     Credentials are used to sign delegated credentials. */
  DelegationProviderSOAP(const std::string& credentials);
  /** Creates instance from provided credentials.
     Credentials are used to sign delegated credentials.
     Arguments should contain filesystem path to PEM-encoded certificate and
     private key. Optionally cert_file may contain certificates chain. */
  DelegationProviderSOAP(const std::string& cert_file,const std::string& key_file,std::istream* inpwd = NULL);
  ~DelegationProviderSOAP(void);
  /** Performs DelegateCredentialsInit SOAP operation.
     As result request for delegated credentials is received by this instance and stored 
    internally. Call to UpdateCredentials should follow. */
  bool DelegateCredentialsInit(MCCInterface& mcc_interface,MessageContext* context);
  /** Extended version of DelegateCredentialsInit(MCCInterface&,MessageContext*).
     Additionally takes attributes for request and response message to make fine
    control on message processing possible. */
  bool DelegateCredentialsInit(MCCInterface& mcc_interface,MessageAttributes* attributes_in,MessageAttributes* attributes_out,MessageContext* context);
  /** Performs UpdateCredentials SOAP operation.
     This concludes delegation procedure and passes delagated credentials to 
    DelegationConsumerSOAP instance.
  */
  bool UpdateCredentials(MCCInterface& mcc_interface,MessageContext* context);
  /** Extended version of UpdateCredentials(MCCInterface&,MessageContext*).
     Additionally takes attributes for request and response message to make fine
    control on message processing possible. */
  bool UpdateCredentials(MCCInterface& mcc_interface,MessageAttributes* attributes_in,MessageAttributes* attributes_out,MessageContext* context);
  /** Generates DelegatedToken element.
     Element is created as child of provided XML element and contains structure
    described in delegation.wsdl. */
  bool DelegatedToken(XMLNode& parent);
};

/** Manages multiple delegated credentials.
   Delegation consumers are created automatically with DelegateCredentialsInit method
  up to max_size_ and assigned unique identifier. It's methods are similar to those
  of DelegationConsumerSOAP with identifier included in SOAP message used to route
  execution to one of managed DelegationConsumerSOAP instances. */
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
  ConsumerIterator RemoveConsumer(ConsumerIterator i);
  void CheckConsumers(void);
 protected:
  Glib::Mutex lock_;
  /** Max. number of delegation consumers */
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
  /** See DelegationConsumerSOAP::DelegateCredentialsInit */
  bool DelegateCredentialsInit(const SOAPEnvelope& in,SOAPEnvelope& out);
  /** See DelegationConsumerSOAP::UpdateCredentials */
  bool UpdateCredentials(std::string& credentials,const SOAPEnvelope& in,SOAPEnvelope& out);
  /** See DelegationConsumerSOAP::DelegatedToken */
  bool DelegatedToken(std::string& credentials,const XMLNode& token);
};

} // namespace Arc


#endif /* __ARC_DELEGATIONINTERFACE_H__ */
