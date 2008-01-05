#include <stdint.h>
#include <string>
#include <list>
#include <arc/ArcConfig.h>
#include <arc/DateTime.h>
#include <arc/Logger.h>
#include <arc/message/Message.h>
#include <arc/message/MCC_Status.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>
#include <arc/message/PayloadSOAP.h>

namespace Arc {

  class Loader;
  class MCC;

  /** Configuration for client interface.
     It contains information which can't be expressed in
     class constructor arguments. Most probably common things
     like software installation location, identity of user, etc. */
  class BaseConfig {
   protected:
    std::list<std::string> plugin_paths;
   public:
    std::string key;
    std::string cert;
    std::string proxy;
    std::string cafile;
    std::string cadir;
    BaseConfig();
    virtual ~BaseConfig() {};
    /** Adds non-standard location of plugins */
    void AddPluginsPath(const std::string& path);
    /** Add private key */
    void AddPrivateKey(const std::string& path);
    /** Add certificate */
    void AddCertificate(const std::string& path);
    /** Add credentials proxy */
    void AddProxy(const std::string& path);
    /** Add CA file */
    void AddCAFile(const std::string& path);
    /** Add CA directory */
    void AddCADir(const std::string& path);
    /** Adds configuration part corresponding to stored information into
       common configuration tree supplied in 'cfg' argument. */
    virtual XMLNode MakeConfig(XMLNode cfg) const;
  };

  class ClientInterface {
   public:
    ClientInterface():loader(NULL) {};
    ClientInterface(const BaseConfig& cfg);
    virtual ~ClientInterface();
   protected:
    Config xmlcfg;
    Loader *loader;
    MessageContext context;
    static Logger logger;
  };

  // Also supports TLS
  class ClientTCP : public ClientInterface {
   public:
    ClientTCP():tcp_entry(NULL),tls_entry(NULL) {};
    ClientTCP(const BaseConfig& cfg, const std::string& host, int port,
	      bool tls);
    virtual ~ClientTCP();
    MCC_Status process(PayloadRawInterface *request,
		       PayloadStreamInterface **response, bool tls);
    PayloadStreamInterface *stream();
   protected:
    MCC *tcp_entry;
    MCC *tls_entry;
  };

  class ClientHTTP : public ClientTCP {
   public:
    struct Info {
      int code;
      std::string reason;
      uint64_t size;
      Time lastModified;
    };
    ClientHTTP():http_entry(NULL) {};
    ClientHTTP(const BaseConfig& cfg, const std::string& host, int port,
	       bool tls, const std::string& path);
    virtual ~ClientHTTP();
    MCC_Status process(const std::string& method, PayloadRawInterface *request,
		       Info *info, PayloadRawInterface **response);
    MCC_Status process(const std::string& method, const std::string& path,
                       PayloadRawInterface *request,
		       Info *info, PayloadRawInterface **response);
    MCC_Status process(const std::string& method, const std::string& path,
                       uint64_t range_start, uint64_t range_end,
                       PayloadRawInterface *request,
		       Info *info, PayloadRawInterface **response);
   protected:
    MCC *http_entry;
  };

  /** Class with easy interface for sending/receiving SOAP messages
     over HTTP(S).
     It takes care of configuring MCC chain and making an entry point. */
  class ClientSOAP : public ClientHTTP {
   public:
    /** Constructor creates MCC chain and connects to server.
       cfg - common configuration,
       host - hostname of remote server,
       port - TCP port of remote server,
       tls - true if connection to use HTTPS, false for HTTP,
       path - internal path of service to be contacted.
       TODO: use URL.
     */
    ClientSOAP():soap_entry(NULL) {};
    ClientSOAP(const BaseConfig& cfg, const std::string& host, int port,
	       bool tls, const std::string& path);
    virtual ~ClientSOAP();
    /** Send SOAP request and receive response. */
    MCC_Status process(PayloadSOAP *request, PayloadSOAP **response);
   protected:
    MCC *soap_entry;
  };

  class MCCConfig : public BaseConfig {
   public:
    MCCConfig() : BaseConfig() {};
    virtual ~MCCConfig() {};
    virtual XMLNode MakeConfig(XMLNode cfg) const;
  };

  class DMCConfig : public BaseConfig {
   public:
    DMCConfig() : BaseConfig() {};
    virtual ~DMCConfig() {};
    virtual XMLNode MakeConfig(XMLNode cfg) const;
  };

} // namespace Arc
