#ifndef __ARC_DELEGATIONINTERFACE_H__
#define __ARC_DELEGATIONINTERFACE_H__

#include <string>
#include <list>
#include <map>

#include <arc/Thread.h>
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
  /** Includes the functionality of Acquire(content) plus extracting the 
     credential identity. */  
  bool Acquire(std::string& content,std::string& identity);
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
   excluding private key. Result is then to be fed into 
   DelegationConsumer::Acquire */
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
  /** Process SOAP message which starts delegation.
    Generated message in 'out' is meant to be sent back to DelagationProviderSOAP.
   Argument 'id' contains identifier of procedure and is used only to produce SOAP 
   message. */
  bool DelegateCredentialsInit(const std::string& id,const SOAPEnvelope& in,SOAPEnvelope& out);
  /** Accepts delegated credentials.
     Process 'in' SOAP message and stores full proxy credentials in 'credentials'. 
    'out' message is generated for sending to DelagationProviderSOAP. */
  bool UpdateCredentials(std::string& credentials,const SOAPEnvelope& in,SOAPEnvelope& out);
  /** Includes the functionality in above UpdateCredentials method; plus extracting the
   credential identity*/
  bool UpdateCredentials(std::string& credentials,std::string& identity,const SOAPEnvelope& in,SOAPEnvelope& out);
  /** Similar to UpdateCredentials but takes only DelegatedToken XML element */ 
  bool DelegatedToken(std::string& credentials,XMLNode token);
  bool DelegatedToken(std::string& credentials,std::string& identity,XMLNode token);
};

/** Extension of DelegationProvider with SOAP exchange interface. 
  This class is also a temporary container for intermediate information
 used during delegation procedure. */
class DelegationProviderSOAP: public DelegationProvider {
 protected:
  /** Delegation request as returned by service accepting delegations. */
  std::string request_;
  /** Assigned delegation identifier. */
  std::string id_;
 public:
  typedef enum {
    ARCDelegation,
    GDS10,
    GDS10RENEW,
    GDS20,
    GDS20RENEW,
    EMIES,
    EMIDS,
    EMIDSRENEW
  } ServiceType;
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
  bool DelegateCredentialsInit(MCCInterface& mcc_interface,MessageContext* context,ServiceType stype = ARCDelegation);
  /** Extended version of DelegateCredentialsInit(MCCInterface&,MessageContext*).
     Additionally takes attributes for request and response message to make fine
    control on message processing possible. */
  bool DelegateCredentialsInit(MCCInterface& mcc_interface,MessageAttributes* attributes_in,MessageAttributes* attributes_out,MessageContext* context,ServiceType stype = ARCDelegation);
  /** Performs UpdateCredentials SOAP operation.
     This concludes delegation procedure and passes delagated credentials to 
    DelegationConsumerSOAP instance.
  */
  bool UpdateCredentials(MCCInterface& mcc_interface,MessageContext* context,const DelegationRestrictions& restrictions = DelegationRestrictions(),ServiceType stype = ARCDelegation);
  /** Extended version of UpdateCredentials(MCCInterface&,MessageContext*).
     Additionally takes attributes for request and response message to make fine
    control on message processing possible. */
  bool UpdateCredentials(MCCInterface& mcc_interface,MessageAttributes* attributes_in,MessageAttributes* attributes_out,MessageContext* context,const DelegationRestrictions& restrictions = DelegationRestrictions(),ServiceType stype = ARCDelegation);
  /** Generates DelegatedToken element.
     Element is created as child of provided XML element and contains structure
    described in delegation.wsdl. */
  bool DelegatedToken(XMLNode parent);
  /** Returns the identifier provided by service accepting delegated credentials.
     This identifier may then be used to refer to credentials stored 
     at service. */
  const std::string& ID(void) { return id_;};
  /**
   * Assigns identifier to be used for while initiating delegation procedure.
   * Assigning identifier is useful only for *RENEW ServiceTypes.
   * 
   * \since Added in 4.1.0.
   **/
  void ID(const std::string& id) { id_ = id; };
};

// Implementaion of the container for delegation credentials
/** Manages multiple delegated credentials.
   Delegation consumers are created automatically with DelegateCredentialsInit method
  up to max_size_ and assigned unique identifier. It's methods are similar to those
  of DelegationConsumerSOAP with identifier included in SOAP message used to route
  execution to one of managed DelegationConsumerSOAP instances. */
class DelegationContainerSOAP {
 protected:
  Glib::Mutex lock_;
  /// Stores description of last error. Derived classes should store their errors here.
  std::string failure_;
  class Consumer;
  /**
   * \since Changed in 4.1.0. Mapped value (Consumer) changed to pointer.
   **/
  typedef std::map<std::string,Consumer*> ConsumerMap;
  typedef ConsumerMap::iterator ConsumerIterator;
  ConsumerMap consumers_;
  ConsumerIterator consumers_first_;
  ConsumerIterator consumers_last_;
  ConsumerIterator find(DelegationConsumerSOAP* c);
  bool remove(ConsumerIterator i);
  /** Max. number of delegation consumers */
  int max_size_;
  /** Lifetime of unused delegation consumer */
  int max_duration_;
  /** Max. times same delegation consumer may accept credentials */
  int max_usage_;
  /** If true delegation consumer is deleted when connection context is destroyed */
  bool context_lock_;

  // Rewritable interface to the box
  /** Creates new consumer object, if empty assigns id and stores in intenal store */
  virtual DelegationConsumerSOAP* AddConsumer(std::string& id,const std::string& client);
  /** Finds previously created consumer in internal store */
  virtual DelegationConsumerSOAP* FindConsumer(const std::string& id,const std::string& client);
  /** Marks consumer as recently used and acquire new credentials */
  virtual bool TouchConsumer(DelegationConsumerSOAP* c,const std::string& credentials);
  /** Obtain stored credentials - not all containers may provide this functionality */
  virtual bool QueryConsumer(DelegationConsumerSOAP* c,std::string& credentials);
  /** Releases consumer obtained by call to AddConsumer() or FindConsumer() */
  virtual void ReleaseConsumer(DelegationConsumerSOAP* c);
  /** Releases consumer obtained by call to AddConsumer() or FindConsumer() and deletes it */
  virtual bool RemoveConsumer(DelegationConsumerSOAP* c);
  /** Periodic management of stored consumers */
  virtual void CheckConsumers(void);

  // Helper methods

  /** See DelegationConsumerSOAP::DelegateCredentialsInit
     If 'client' is not empty then all subsequent calls involving access to generated
     credentials must contain same value in their 'client' arguments. */
  bool DelegateCredentialsInit(const SOAPEnvelope& in,SOAPEnvelope& out,const std::string& client = "");

  /** See DelegationConsumerSOAP::UpdateCredentials */
  bool UpdateCredentials(std::string& credentials,const SOAPEnvelope& in,SOAPEnvelope& out,const std::string& client = "");
  bool UpdateCredentials(std::string& credentials,std::string& identity,const SOAPEnvelope& in,SOAPEnvelope& out,const std::string& client = "");

 public:
  DelegationContainerSOAP(void);
  virtual ~DelegationContainerSOAP(void);

  /** See DelegationConsumerSOAP::DelegatedToken */
  bool DelegatedToken(std::string& credentials,XMLNode token,const std::string& client = "");
  bool DelegatedToken(std::string& credentials,std::string& identity,XMLNode token,const std::string& client = "");

  bool Process(const SOAPEnvelope& in,SOAPEnvelope& out,const std::string& client = "");
  bool Process(std::string& credentials,const SOAPEnvelope& in,SOAPEnvelope& out,const std::string& client = "");
  /** Match namespace of SOAP request against supported interfaces.
     Returns true if namespace is supported. */
  bool MatchNamespace(const SOAPEnvelope& in);
  /** Returns textual description of last failure. */
  std::string GetFailure(void);
};

} // namespace Arc


#endif /* __ARC_DELEGATIONINTERFACE_H__ */
