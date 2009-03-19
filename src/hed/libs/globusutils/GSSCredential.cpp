#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>

#include <arc/Logger.h>

#include "GlobusErrorUtils.h"
#include "GSSCredential.h"

namespace Arc {

  static Logger logger(Logger::getRootLogger(), "GSSCredential");

  GSSCredential::GSSCredential(const std::string& proxyPath,
			       const std::string& certificatePath,
			       const std::string& keyPath)
    : credential(GSS_C_NO_CREDENTIAL) {

    std::string credbuf;
  
    if (!proxyPath.empty()) {
      std::ifstream is(proxyPath.c_str());
      getline(is, credbuf, '\0');
      if(!is || credbuf.empty()) {
	logger.msg(ERROR, "Failed to read proxy file: %s", proxyPath);
	return;
      }
    }
    else if (!certificatePath.empty() && !keyPath.empty()) {
      std::ifstream is(certificatePath.c_str());
      getline(is, credbuf, '\0');
      if(!is || credbuf.empty()) {
	logger.msg(ERROR, "Failed to read certificate file: %s",
		   certificatePath);
	return;
      }
      std::string keybuf;
      std::ifstream ik(keyPath.c_str());
      getline(ik, keybuf, '\0');
      if(!ik || keybuf.empty()) {
	logger.msg(ERROR, "Failed to read private key file: %s", keyPath);
	return;
      }
      credbuf += "\n";
      credbuf += keybuf;
    }

    if(!credbuf.empty()) { 
      //Convert to GSS credental only if find credential content
      OM_uint32 majstat, minstat;
      gss_buffer_desc gbuf;

      gbuf.value = (void*)credbuf.c_str();
      gbuf.length = credbuf.length();

      majstat = gss_import_cred(&minstat, &credential, NULL, 0,
			      &gbuf, GSS_C_INDEFINITE, NULL);

      if (GSS_ERROR(majstat)) {
        logger.msg(ERROR, "Failed to convert GSI credential to "
                    "GSS credential (major: %d, minor: %d)%s", majstat, minstat, ErrorStr(majstat, minstat));
        return;
      }
    }
  }

  GSSCredential::~GSSCredential() {

    if (credential != GSS_C_NO_CREDENTIAL) {
      OM_uint32 majstat, minstat;
      majstat = gss_release_cred(&minstat, &credential);
      if (GSS_ERROR(majstat)) {
	logger.msg(ERROR, "Failed to release GSS credential "
		   "(major: %d, minor: %d):%s", majstat, minstat, ErrorStr(majstat, minstat));
	return;
      }
    }
  }

  GSSCredential::operator gss_cred_id_t&() {
    return credential;
  }

  GSSCredential::operator gss_cred_id_t*() {
    return &credential;
  }

  std::string GSSCredential::ErrorStr(OM_uint32 majstat, OM_uint32 /*minstat*/) {
    std::string errstr;
    if(majstat & GSS_S_BAD_MECH) errstr+=":GSS_S_BAD_MECH";
    if(majstat & GSS_S_BAD_NAME) errstr+=":GSS_S_BAD_NAME";
    if(majstat & GSS_S_BAD_NAMETYPE) errstr+=":GSS_S_BAD_NAMETYPE";
    if(majstat & GSS_S_BAD_BINDINGS) errstr+=":GSS_S_BAD_BINDINGS";
    if(majstat & GSS_S_BAD_STATUS) errstr+=":GSS_S_BAD_STATUS";
    if(majstat & GSS_S_BAD_SIG) errstr+=":GSS_S_BAD_SIG";
    if(majstat & GSS_S_NO_CRED) errstr+=":GSS_S_NO_CRED";
    if(majstat & GSS_S_NO_CONTEXT) errstr+=":GSS_S_NO_CONTEXT";
    if(majstat & GSS_S_DEFECTIVE_TOKEN) errstr+=":GSS_S_DEFECTIVE_TOKEN";
    if(majstat & GSS_S_DEFECTIVE_CREDENTIAL) errstr+=":GSS_S_DEFECTIVE_CREDENTIAL";
    if(majstat & GSS_S_CREDENTIALS_EXPIRED) errstr+=":GSS_S_CREDENTIALS_EXPIRED";
    if(majstat & GSS_S_CONTEXT_EXPIRED) errstr+=":GSS_S_CONTEXT_EXPIRED";
    if(majstat & GSS_S_FAILURE) errstr+=":GSS_S_FAILURE";
    if(majstat & GSS_S_BAD_QOP) errstr+=":GSS_S_BAD_QOP";
    if(majstat & GSS_S_UNAUTHORIZED) errstr+=":GSS_S_UNAUTHORIZED";
    if(majstat & GSS_S_UNAVAILABLE) errstr+=":GSS_S_UNAVAILABLE";
    if(majstat & GSS_S_DUPLICATE_ELEMENT) errstr+=":GSS_S_DUPLICATE_ELEMENT";
    if(majstat & GSS_S_NAME_NOT_MN) errstr+=":GSS_S_NAME_NOT_MN";
    if(majstat & GSS_S_EXT_COMPAT) errstr+=":GSS_S_EXT_COMPAT";
    return errstr;
  }
} // namespace Arc
