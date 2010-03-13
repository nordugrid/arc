#include <string>
#include <arc/UserConfig.h>
#include <arc/message/MCC.h>
#include <arc/URL.h>
#include <arc/StringConv.h>

namespace Arc {

  /** This class provides functionality for storing delegated crdentials
     and retrieving them from some store services. This is very preliminary
     implementation and currently support only one type of credentials - 
     X.509 proxies, and only one type of store service - MyProxy.
     Later it will be extended to support at least following services:
      ARC delegation service, VOMS service, local file system. */
  class CredentialStore {
   private:
    static const int default_port = 7512;
    bool valid;
    int timeout;
    void set(const std::string& host, int port, const UserConfig& cfg);
    MCCConfig concfg;
    std::string host;
    int port;
   public:
    CredentialStore(const URL& url);
    CredentialStore(const std::string& host, int port = default_port);
    CredentialStore(const UserConfig& cfg, const URL& url);
    CredentialStore(const UserConfig& cfg, const std::string& host, int port = default_port);
    ~CredentialStore(void);
    operator bool(void) { return valid; };
    bool operator!(void) { return !valid; };
    bool Store(const std::string& username,const std::string& password,int lifetime,const std::string& cred);
    bool Retrieve(const std::string& username,const std::string& password,int lifetime,std::string& cred);
};

}
