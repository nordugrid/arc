#include <string>
#include <list>
#include <arc/loader/Loader.h>
#include <arc/message/Message.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>
#include <arc/message/PayloadSOAP.h>

namespace Arc {

class BaseConfig {
 protected:
  std::string prefix_;
  std::list<std::string> plugin_paths_;
 public:
  BaseConfig(const std::string& prefix = "");
  ~BaseConfig(void) { };
  void AddPluginsPath(const std::string& path);
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

class ClientSOAP: public ClientHTTP {
 protected:
  MCC* soap_entry_;
 public:
  ClientSOAP(const BaseConfig& cfg,const std::string& host,int port,bool tls,const std::string& path);
  ~ClientSOAP(void);
  MCC_Status process(PayloadSOAP* request,PayloadSOAP** response);  
};

}

