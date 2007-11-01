#include <string>
#include <list>
#include <arc/loader/Loader.h>
#include <arc/message/Message.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>
#include <arc/message/PayloadSOAP.h>

namespace Arc {

/** Configuration for client interface.
   It contains information which can't be expressed in 
  class constructor arguments. Most probably common things
  like software installation location, identity of user, etc. */
class BaseConfig {
 protected:
  std::list<std::string> plugin_paths_;
 public:
  BaseConfig();
  ~BaseConfig(void) { };
  /** Adds non-standard location of plugins */ 
  void AddPluginsPath(const std::string& path);
  /** Adds configuration part corresponding to stored 
    information into common configuration tree supplied in 
    'cfg' argument. */
  XMLNode MakeConfig(XMLNode cfg) const;
};

class ClientInterface {
 protected:
  Loader* loader_;
  MessageContext context_;
 public:
  ClientInterface(void):loader_(NULL) { };
  ~ClientInterface(void) { };
  MCC_Status process(MessagePayload* request,MessagePayload** response) { };  
};

// Also supports TLS
class ClientTCP: public ClientInterface {
 public:
  ClientTCP(const BaseConfig& cfg,const std::string& host,int port,bool tls);
  ~ClientTCP(void);
  MCC_Status process(PayloadRawInterface* request,PayloadStreamInterface** response);  
  PayloadStreamInterface* stream(void);  
};

class ClientHTTP: public ClientTCP {
 public:
  ClientHTTP(const BaseConfig& cfg,const std::string& host,int port,bool tls,const std::string& path);
  ~ClientHTTP(void);
  MCC_Status process(const std::string& method,PayloadRawInterface* request,
                    int* code,std::string& reason,PayloadRawInterface** response);  
};

/** Class with easy interface for sending/receiving SOAP messages over HTTP(S).
  It takes care of configuring MCC chain and making an entry point. */
class ClientSOAP: public ClientHTTP {
 protected:
  MCC* soap_entry_;
 public:
  /** Constructor creates MCC chain and connects to server.
     cfg - common configuration,
     host - hostname of remote server,
     port - TCP port of remote server,
     tls - true if connection to use HTTPS, false for HTTP,
     path - internal path of service to be contacted.
    TODO: use URL.
  */
  ClientSOAP(const BaseConfig& cfg,const std::string& host,int port,bool tls,const std::string& path);
  ~ClientSOAP(void);
  /** Send SOAP request and receive response. */
  MCC_Status process(PayloadSOAP* request,PayloadSOAP** response);  
};

class DMCConfig : public BaseConfig {
 public:
  DMCConfig() : BaseConfig() {};
  ~DMCConfig() {};
  XMLNode MakeConfig(XMLNode cfg) const;
};

}
