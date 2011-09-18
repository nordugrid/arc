#include <string>
#include <map>
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
    CredentialStore(const UserConfig& cfg, const URL& url);
    ~CredentialStore(void);
    operator bool(void) { return valid; };
    bool operator!(void) { return !valid; };
    // Store delegated credentials (or an end-entity credential) to credential store.
    // The options contains key=value pairs affecting how credentials are
    // stored. For MyProxy following options are supported -
    //  username, password, credname, lifetime.
    // If cred is not empty it should contains credentials to delegate/store.
    // Otherwise credentials of user configuration are used.
    bool Store(const std::map<std::string,std::string>& options,
               const std::string& cred = "", bool if_delegate = true, const Arc::Time deleg_start = Arc::Time(), const Arc::Period deleg_period = 604800);
    bool Retrieve(const std::map<std::string,std::string>& options,
                  std::string& cred, bool if_delegate = true);
    bool Info(const std::map<std::string,std::string>& options,std::string& respinfo);
    bool Destroy(const std::map<std::string,std::string>& options);
    bool ChangePassword(const std::map<std::string,std::string>& options);
};

}
