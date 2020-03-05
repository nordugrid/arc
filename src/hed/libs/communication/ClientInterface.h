// -*- indent-tabs-mode: nil -*-

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

  class MCCLoader;
  class Logger;
  class MCC;
   
  //! Utility base class for MCC
  /** The ClientInterface class is a utility base class used for
   *  configuring a client side Message Chain Component (MCC) chain
   *  and loading it into memory. It has several specializations of
   *  increasing complexity of the MCC chains.
   *  This class is not supposed to be used directly. Instead its
   *  descendants like ClientTCP, ClientHTTP, etc. must be used.
   **/
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
    /// Initializes communication chain for this object.
    /// Call to this method in derived class is not needed
    /// if process() methods are used. It is only needed if 
    /// GetEntry() is used before process() is called.
    virtual MCC_Status Load();
  protected:
    Config xmlcfg;
    XMLNode overlay;
    MCCLoader *loader;
    MessageContext context;
    static Logger logger;
    static void AddSecHandler(XMLNode mcccfg, XMLNode handlercfg);
    static void AddPlugin(XMLNode mcccfg, const std::string& libname, const std::string& libpath = "");
  };

  enum SecurityLayer {
    NoSec,      //< No security applied to this communication
    TLSSec,     //< HTTPS-like security is used (handshake negotiates highest possible protocol)
    GSISec,     //< GSI compatible communication
    SSL3Sec,    //< Start communication from SSLv3 handshake
    GSIIOSec,   //< Globus GSI implemwntation compatible communication
    TLS10Sec,   //< TLSv1.0 only protocol
    TLS11Sec,   //< TLSv1.1 only protocol
    TLS12Sec,   //< TLSv1.2 only protocol
    DTLSSec,    //< Automatic selection of DTLS protocol
    DTLS10Sec,  //< DTLSv1.0 only protocol
    DTLS12Sec,  //< DTLSv1.0 only protocol
  };

  enum EncryptionLevel {
    NoEnc,      //< No data encryption to be performed
    RequireEnc, //< Force data encryption
    PreferEnc,  //< Use data encryption if possible
    OptionalEnc //< Use data encryption only if needed
  };

  class TCPSec {
  public:
    SecurityLayer sec;
    EncryptionLevel enc;
    TCPSec(void):sec(NoSec),enc(NoEnc) { };
    TCPSec(SecurityLayer s):sec(s),enc((s==NoSec)?NoEnc:RequireEnc) { };
    TCPSec(SecurityLayer s, EncryptionLevel e):sec(s),enc(e) { };
  };

  //! Class for setting up a MCC chain for TCP communication
  /** The ClientTCP class is a specialization of the ClientInterface
   * which sets up a client MCC chain for TCP communication, and
   * optionally with a security layer on top which can be either TLS,
   * GSI or SSL3.
   **/
  class ClientTCP
    : public ClientInterface {
  public:
    ClientTCP()
      : tcp_entry(NULL),
        tls_entry(NULL) {}
    ClientTCP(const BaseConfig& cfg, const std::string& host, int port,
              TCPSec sec, int timeout = -1, bool no_delay = false);
    virtual ~ClientTCP();
    MCC_Status process(PayloadRawInterface *request,
                       PayloadStreamInterface **response, bool tls);
    MCC_Status process(PayloadStreamInterface *request,
                       PayloadStreamInterface **response, bool tls);
    /** Returns entry point to TCP or TLS MCC in configured chain.
       To initialize entry point Load() method must be called. */
    MCC* GetEntry() {
      return tls_entry ? tls_entry : tcp_entry;
    }
    virtual MCC_Status Load();
    void AddSecHandler(XMLNode handlercfg, TCPSec sec, const std::string& libname = "", const std::string& libpath = "");
  protected:
    MCC *tcp_entry;
    MCC *tls_entry;
  };

  struct HTTPClientInfo {
    int code; /// HTTP response code
    std::string reason; /// HTTP response reason
    uint64_t size; /// Size of body (content-length)
    Time lastModified; /// Reported modification time
    std::string type; /// Content-type
    std::list<std::string> cookies; /// All collected cookies
    /// All returned headers
    /**
     * \since Added in 4.1.0.
     **/
    std::multimap<std::string, std::string> headers;
    URL location; /// Value of location attribute in HTTP response
    bool keep_alive;
  };


  class ClientHTTP;
  //! Proxy class for handling request parameters
  /** The purpose of this calss is to reduce number of methods
     in ClientHTTP class. Use only for temporary variables. */
  class ClientHTTPAttributes {
  friend class ClientHTTP;
  public:
    ClientHTTPAttributes(const std::string& method);
    ClientHTTPAttributes(const std::string& method,
                       std::multimap<std::string, std::string>& attributes);
    ClientHTTPAttributes(const std::string& method, const std::string& path);
    ClientHTTPAttributes(const std::string& method, const std::string& path,
                       std::multimap<std::string, std::string>& attributes);
    ClientHTTPAttributes(const std::string& method, const std::string& path,
                       uint64_t range_start, uint64_t range_end);
    ClientHTTPAttributes(const std::string& method, const std::string& path,
                       std::multimap<std::string, std::string>& attributes,
                       uint64_t range_start, uint64_t range_end);
  protected:
    const std::string default_path_;
    std::multimap<std::string, std::string> default_attributes_;
    const std::string& method_;
    const std::string& path_;
    std::multimap<std::string, std::string>& attributes_;
    uint64_t range_start_;
    uint64_t range_end_;
  };

  //! Class for setting up a MCC chain for HTTP communication
  /** The ClientHTTP class inherits from the ClientTCP class and adds
   * an HTTP MCC to the chain.
   **/
  class ClientHTTP
    : public ClientTCP {
  public:
    ClientHTTP()
      : http_entry(NULL), relative_uri(false), sec(NoSec), closed(false) {}
    ClientHTTP(const BaseConfig& cfg, const URL& url, int timeout = -1, const std::string& proxy_host = "", int proxy_port = 0);
    virtual ~ClientHTTP();
    MCC_Status process(const std::string& method, PayloadRawInterface *request,
                       HTTPClientInfo *info, PayloadRawInterface **response);
    MCC_Status process(const std::string& method,
                       std::multimap<std::string, std::string>& attributes,
                       PayloadRawInterface *request,
                       HTTPClientInfo *info, PayloadRawInterface **response);
    MCC_Status process(const std::string& method, const std::string& path,
                       PayloadRawInterface *request,
                       HTTPClientInfo *info, PayloadRawInterface **response);
    MCC_Status process(const std::string& method, const std::string& path,
                       std::multimap<std::string, std::string>& attributes,
                       PayloadRawInterface *request,
                       HTTPClientInfo *info, PayloadRawInterface **response);
    MCC_Status process(const std::string& method, const std::string& path,
                       uint64_t range_start, uint64_t range_end,
                       PayloadRawInterface *request,
                       HTTPClientInfo *info, PayloadRawInterface **response);
    MCC_Status process(const std::string& method, const std::string& path,
                       std::multimap<std::string, std::string>& attributes,
                       uint64_t range_start, uint64_t range_end,
                       PayloadRawInterface *request,
                       HTTPClientInfo *info, PayloadRawInterface **response);
    MCC_Status process(const ClientHTTPAttributes &reqattr,
                       PayloadRawInterface *request,
                       HTTPClientInfo *info, PayloadRawInterface **response);
    MCC_Status process(const ClientHTTPAttributes &reqattr,
                       PayloadStreamInterface *request,
                       HTTPClientInfo *info, PayloadRawInterface **response);
    MCC_Status process(const ClientHTTPAttributes &reqattr,
                       PayloadRawInterface *request,
                       HTTPClientInfo *info, PayloadStreamInterface **response);
    MCC_Status process(const ClientHTTPAttributes &reqattr,
                       PayloadStreamInterface *request,
                       HTTPClientInfo *info, PayloadStreamInterface **response);
    /** Returns entry point to HTTP MCC in configured chain.
       To initialize entry point Load() method must be called. */
    MCC* GetEntry() {
      return http_entry;
    }
    void AddSecHandler(XMLNode handlercfg, const std::string& libname = "", const std::string& libpath = "");
    virtual MCC_Status Load();
    void RelativeURI(bool val) { relative_uri=val; };
    const URL& GetURL() const { return default_url; };
    bool GetClosed() const { return closed; }
  protected:
    MCC *http_entry;
    URL default_url;
    bool relative_uri;
    TCPSec sec;
    bool closed;
    MCC_Status process(const std::string& method, const std::string& path,
                       std::multimap<std::string, std::string>& attributes,
                       uint64_t range_start, uint64_t range_end,
                       MessagePayload *request,
                       HTTPClientInfo *info, MessagePayload **response);
  };

  /** Class with easy interface for sending/receiving SOAP messages
      over HTTP(S/G).
      It takes care of configuring MCC chain and making an entry point. */
  class ClientSOAP
    : public ClientHTTP {
  public:
    /** Constructor creates MCC chain and connects to server. */
    ClientSOAP()
      : soap_entry(NULL) {}
    ClientSOAP(const BaseConfig& cfg, const URL& url, int timeout = -1);
    virtual ~ClientSOAP();
    /** Send SOAP request and receive response. */
    MCC_Status process(PayloadSOAP *request, PayloadSOAP **response);
    /** Send SOAP request + additional HTTP header attributes and receive response. */
    MCC_Status process(const std::multimap<std::string, std::string> &http_attr, PayloadSOAP *request, PayloadSOAP **response);
    /** Send SOAP request with specified SOAP action and receive response. */
    MCC_Status process(const std::string& action, PayloadSOAP *request,
                       PayloadSOAP **response);
    /** Send SOAP request with specified SOAP action + additional HTTP header attributes and receive response. */
    MCC_Status process(const std::multimap<std::string, std::string> &http_attr, const std::string& action, PayloadSOAP *request,
                       PayloadSOAP **response);
    /** Returns entry point to SOAP MCC in configured chain.
       To initialize entry point Load() method must be called. */
    MCC* GetEntry() {
      return soap_entry;
    }
    /** Adds security handler to configuration of SOAP MCC */
    void AddSecHandler(XMLNode handlercfg, const std::string& libname = "", const std::string& libpath = "");
    /** Instantiates pluggable elements according to generated configuration */
    virtual MCC_Status Load();
  protected:
    MCC *soap_entry;
  };

  // Convenience base class for creating configuration
  // security handlers.
  class SecHandlerConfig
    : public XMLNode {
  public:
    SecHandlerConfig(const std::string& name, const std::string& event = "incoming");
  };

  class DNListHandlerConfig
    : public SecHandlerConfig {
  public:
    DNListHandlerConfig(const std::list<std::string>& dns, const std::string& event = "incoming");
    void AddDN(const std::string& dn);
  };

  class ARCPolicyHandlerConfig
    : public SecHandlerConfig {
  public:
    ARCPolicyHandlerConfig(const std::string& event = "incoming");
    ARCPolicyHandlerConfig(XMLNode policy, const std::string& event = "incoming");
    void AddPolicy(XMLNode policy);
    void AddPolicy(const std::string& policy);
  };

} // namespace Arc

#endif // __ARC_CLIENTINTERFACE_H__
