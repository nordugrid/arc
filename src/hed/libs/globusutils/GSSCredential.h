#ifndef __ARC_GLOBUSGSS_H__
#define __ARC_GLOBUSGSS_H__

#include <string>

#include <gssapi.h>

namespace Arc {

  class Logger;

  /// Class for converting credentials stored in file 
  /// in PEM format into Globus structire.
  /// It works only for full credentials containing 
  /// private key. This limitation is due to limited
  /// API of Globus.
  class GSSCredential {
  public:
    GSSCredential(const std::string& proxyPath,
		  const std::string& certificatePath,
		  const std::string& keyPath);
    GSSCredential(): credential(GSS_C_NO_CREDENTIAL) {};
    ~GSSCredential();
    operator gss_cred_id_t&();
    operator gss_cred_id_t*();
    static std::string ErrorStr(OM_uint32 majstat, OM_uint32 minstat);
  private:
    gss_cred_id_t credential;
    //static Logger logger;
  };

} // namespace Arc

#endif // __ARC_GLOBUSGSS_H__
