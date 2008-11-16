#include <globus_gsi_credential.h>

#include <arc/Logger.h>

#include "GlobusErrorUtils.h"
#include "GSSCredential.h"

// This is a bit of a hack - using an internal globus gssapi function
// which is not available in any public header.
// But I have not found any other way to use a non-standard proxy or
// certificate location without setting environment variables,
// which we don't want to do since we are using threads.

extern "C" {
OM_uint32 globus_i_gsi_gss_create_cred(OM_uint32 *minor_status,
                                       const gss_cred_usage_t cred_usage,
                                       gss_cred_id_t *output_cred_handle_P,
                                       globus_gsi_cred_handle_t *cred_handle);
}

static int pwck(char*, int, int) {
  return -1;
}

namespace Arc {

  Logger GSSCredential::logger(Logger::getRootLogger(), "GSSCredential");

  GSSCredential::GSSCredential(const std::string& proxyPath,
			       const std::string& certificatePath,
			       const std::string& keyPath)
    : credential(GSS_C_NO_CREDENTIAL) {

    GlobusResult result;

    globus_gsi_cred_handle_t handle;
    result = globus_gsi_cred_handle_init(&handle, NULL);
    if (!result) {
      logger.msg(ERROR, "Failed to initialize credential handle: %s",
		 result.str());
      return;
    }

    if (!proxyPath.empty()) {
      result = globus_gsi_cred_read_proxy(handle, proxyPath.c_str());
      if (!result) {
	logger.msg(ERROR, "Failed to read proxy file: %s", result.str());
	globus_gsi_cred_handle_destroy(handle);
	return;
      }
    }
    else if (!certificatePath.empty() && !keyPath.empty()) {
      result =
	globus_gsi_cred_read_cert(handle,
				  const_cast<char*>(certificatePath.c_str()));
      if (!result) {
	logger.msg(ERROR, "Failed to read certificate file: %s", result.str());
	globus_gsi_cred_handle_destroy(handle);
	return;
      }
      result =
	globus_gsi_cred_read_key(handle,
				 const_cast<char*>(keyPath.c_str()),
				 (int(*)())pwck);
      if (!result) {
	logger.msg(ERROR, "Failed to read key file: %s", result.str());
	globus_gsi_cred_handle_destroy(handle);
	return;
      }
    }

    OM_uint32 majstat, minstat;
    majstat = globus_i_gsi_gss_create_cred(&minstat, GSS_C_BOTH,
					   &credential, &handle);
    if (GSS_ERROR(majstat)) {
      logger.msg(ERROR, "Failed to convert GSI credential to "
		 "GSS credential (major: %d, minor: %d)", majstat, minstat);
      globus_gsi_cred_handle_destroy(handle);
      return;
    }

    result = globus_gsi_cred_handle_destroy(handle);
    if (!result) {
      logger.msg(ERROR, "Failed to destroy credential handle: %s",
		 result.str());
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

} // namespace Arc
