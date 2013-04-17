#ifndef __ARC_GLOBUSGSS_H__
#define __ARC_GLOBUSGSS_H__

#include <string>

#include <gssapi.h>

namespace Arc {

  class Logger;
  class UserConfig;

  /// Class for converting credentials stored in file 
  /// in PEM format into Globus structure.
  /// It works only for full credentials containing 
  /// private key. This limitation is due to limited
  /// API of Globus.
  class GSSCredential {
  public:
    /// Load credentials from file(s)
    GSSCredential(const std::string& proxyPath,
                  const std::string& certificatePath,
                  const std::string& keyPath);
    /// Load credentials from UserConfig information. First tries string then files.
    GSSCredential(const UserConfig& usercfg);
    GSSCredential(): credential(GSS_C_NO_CREDENTIAL) {};
    ~GSSCredential();
    operator gss_cred_id_t&();
    operator gss_cred_id_t*();
    static std::string ErrorStr(OM_uint32 majstat, OM_uint32 minstat);
  private:
    std::string readCredFromFiles(const std::string& proxyPath,
                                  const std::string& certificatePath,
                                  const std::string& keyPath);
    void initCred(const std::string& credbuf);
    gss_cred_id_t credential;
    //static Logger logger;
  };

} // namespace Arc

#endif // __ARC_GLOBUSGSS_H__
