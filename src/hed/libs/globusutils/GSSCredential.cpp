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

    OM_uint32 majstat, minstat;
    gss_buffer_desc gbuf;

    gbuf.value = (void*)credbuf.c_str();
    gbuf.length = credbuf.length();

    majstat = gss_import_cred(&minstat, &credential, NULL, 0,
			      &gbuf, GSS_C_INDEFINITE, NULL);

    if (GSS_ERROR(majstat)) {
      logger.msg(ERROR, "Failed to convert GSI credential to "
		 "GSS credential (major: %d, minor: %d)", majstat, minstat);
      return;
    }
  }

  GSSCredential::~GSSCredential() {

    if (credential != GSS_C_NO_CREDENTIAL) {
      OM_uint32 majstat, minstat;
      majstat = gss_release_cred(&minstat, &credential);
      if (GSS_ERROR(majstat)) {
	logger.msg(ERROR, "Failed to release GSS credential "
		   "(major: %d, minor: %d)", majstat, minstat);
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

} // namespace Arc
