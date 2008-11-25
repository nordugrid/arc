#ifndef __ARC_CLIENTINTERFACE_H__
#define __ARC_CLIENTINTERFACE_H__

#include <string>
#include <list>

#include <inttypes.h>

#include <arc/ArcConfig.h>
#include <arc/DateTime.h>
#include <arc/URL.h>
#include <arc/message/Message.h>
#include <arc/message/MCC_Status.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>
#include <arc/message/PayloadSOAP.h>

namespace Arc {

  class Loader;
  class Logger;
  class MCC;

  class ClientInterface {
  public:
    ClientInterface()
      : loader(NULL) {}
    ClientInterface(const BaseConfig& cfg);
    virtual ~ClientInterface();
    void Overlay(XMLNode cfg);
    const Config& GetConfig() const {
      return xmlcfg;
    }
    MessageContext& GetContext() {
      return context;
    }
    virtual void Load();
  protected:
    Config xmlcfg;
    XMLNode overlay;
    Loader *loader;
    MessageContext context;
    static Logger logger;
  };

  // Also supports TLS
  class ClientTCP
    : public ClientInterface {
  public:
    ClientTCP()
      : tcp_entry(NULL),
	tls_entry(NULL) {}
    ClientTCP(const BaseConfig& cfg, const std::string& host, int port,
	      bool tls);
    virtual ~ClientTCP();
    MCC_Status process(PayloadRawInterface *request,
		       PayloadStreamInterface **response, bool tls);
    MCC* GetEntry() {
      return tls_entry ? tls_entry : tcp_entry;
    }
    virtual void Load();
  protected:
    MCC *tcp_entry;
    MCC *tls_entry;
  };

  struct HTTPClientInfo {
    int code;
    std::string reason;
    uint64_t size;
    Time lastModified;
    std::string type;
    std::string cookie;
    std::string location;
  };

  class ClientHTTP
    : public ClientTCP {
  public:
    ClientHTTP()
      : http_entry(NULL) {}
    ClientHTTP(const BaseConfig& cfg, const URL& url);
    virtual ~ClientHTTP();
    MCC_Status process(const std::string& method, PayloadRawInterface *request, 
		       HTTPClientInfo *info, PayloadRawInterface **response);
    MCC_Status process(const std::string& method,
		       std::map<std::string, std::string>& attributes,
		       PayloadRawInterface *request,
		       HTTPClientInfo *info, PayloadRawInterface **response);
    MCC_Status process(const std::string& method, const std::string& path, 
		       PayloadRawInterface *request,
		       HTTPClientInfo *info, PayloadRawInterface **response);
    MCC_Status process(const std::string& method, const std::string& path, 
		       std::map<std::string, std::string>& attributes,
		       PayloadRawInterface *request,
		       HTTPClientInfo *info, PayloadRawInterface **response);
    MCC_Status process(const std::string& method, const std::string& path,
		       uint64_t range_start, uint64_t range_end,
		       PayloadRawInterface *request,
		       HTTPClientInfo *info, PayloadRawInterface **response);
    MCC_Status process(const std::string& method, const std::string& path, 
		       std::map<std::string, std::string>& attributes,
		       uint64_t range_start, uint64_t range_end,
		       PayloadRawInterface *request,
		       HTTPClientInfo *info, PayloadRawInterface **response);
    MCC* GetEntry() {
      return http_entry;
    }
    virtual void Load();
  protected:
    MCC *http_entry;
    std::string host;
    int port;
    bool tls;
  };

  /** Class with easy interface for sending/receiving SOAP messages
      over HTTP(S).
      It takes care of configuring MCC chain and making an entry point. */
  class ClientSOAP
    : public ClientHTTP {
  public:
    /** Constructor creates MCC chain and connects to server. */
    ClientSOAP()
      : soap_entry(NULL) {}
    ClientSOAP(const BaseConfig& cfg, const URL& url);
    virtual ~ClientSOAP();
    /** Send SOAP request and receive response. */
    MCC_Status process(PayloadSOAP *request, PayloadSOAP **response);
    /** Send SOAP request with specified SOAP action and receive response. */
    MCC_Status process(const std::string& action, PayloadSOAP *request,
		       PayloadSOAP **response);
    MCC* GetEntry() {
      return soap_entry;
    }
    virtual void Load();
  protected:
    MCC *soap_entry;
  };

} // namespace Arc

#endif // __ARC_CLIENTINTERFACE_H__
