#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm.h>

#include <openssl/x509.h>
#include <openssl/evp.h>

#include <gssapi.h>
#include <globus_gsi_credential.h>
#include <globus_gsi_cert_utils.h>

#include <arc/Logger.h>
#include <arc/Utils.h>
#include <arc/globusutils/GlobusErrorUtils.h>
#include "cert_util.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(),"LCAS");

#ifdef HAVE_LCAS
#include <openssl/x509.h>
#define ALLOW_EMPTY_CREDENTIALS 1
extern "C" {
#define extern typedef
#define getMajorVersionNumber lcas_getMajorVersionNumber
#define getMinorVersionNumber lcas_getMinorVersionNumber
#define getPatchVersionNumber lcas_getPatchVersionNumber
#define lcas_init (*lcas_init_t)
#define lcas_get_fabric_authorization (*lcas_get_fabric_authorization_t)
#define lcas_term (*lcas_term_t)
#include <lcas.h>
#undef lcas_init
#undef lcas_get_fabric_authorization
#undef lcas_term
#undef getMajorVersionNumber
#undef getMinorVersionNumber
#undef getPatchVersionNumber
#undef extern
};
#else
//#warning Using hardcoded definition of LCAS functions - software will break if interface changed
extern "C" {
typedef char* lcas_request_t;
typedef int (*lcas_init_t)(FILE *fp);
typedef int (*lcas_term_t)(void);
typedef int (*lcas_get_fabric_authorization_t)(char *user_dn_tmp,gss_cred_id_t user_cred,lcas_request_t request);
}
#endif

extern "C" {

// Definitions taken from gssapi_openssl.h

typedef struct gss_cred_id_desc_struct {
    globus_gsi_cred_handle_t            cred_handle;
    gss_name_t                          globusid;
    gss_cred_usage_t                    cred_usage;
    SSL_CTX *                           ssl_context;
} gss_cred_id_desc;

extern gss_OID_desc * gss_nt_x509;
#define GLOBUS_GSS_C_NT_X509 gss_nt_x509

}; // extern "C"

static std::string lcas_db_file_old;
static std::string lcas_dir_old;

void set_lcas_env(const std::string& lcas_db_file,const std::string& lcas_dir) {
  lcas_db_file_old=Arc::GetEnv("LCAS_DB_FILE");
  if(lcas_db_file.length() != 0) Arc::SetEnv("LCAS_DB_FILE",lcas_db_file,true);
  lcas_dir_old=Arc::GetEnv("LCAS_DIR");
  if(lcas_dir.length() != 0) Arc::SetEnv("LCAS_DIR",lcas_dir,true);
}

void recover_lcas_env(void) {
  if(lcas_db_file_old.length() == 0) {
    Arc::UnsetEnv("LCAS_DB_FILE");
  } else {
    Arc::SetEnv("LCAS_DB_FILE",lcas_db_file_old,true);
  };
  if(lcas_dir_old.length() == 0) {
    Arc::UnsetEnv("LCAS_DIR");
  } else {
    Arc::SetEnv("LCAS_DIR",lcas_dir_old,true);
  };
}

gss_cred_id_t read_globus_credentials(const std::string& filename) {
  X509* cert = NULL;
  STACK_OF(X509)* cchain = NULL;
  EVP_PKEY* key = NULL;
  
  LoadCertificateFile(filename, cert, cchain);
  LoadKeyFile(filename, key);

  globus_gsi_cred_handle_t chandle;
  Arc::GlobusResult(globus_gsi_cred_handle_init(&chandle, NULL));
  if(cert) Arc::GlobusResult(globus_gsi_cred_set_cert(chandle, cert));
  if(key) Arc::GlobusResult(globus_gsi_cred_set_key(chandle, key));
  if(cchain) Arc::GlobusResult(globus_gsi_cred_set_cert_chain(chandle, cchain));

  gss_cred_id_desc* ccred = (gss_cred_id_desc*)::malloc(sizeof(gss_cred_id_desc));
  if(ccred) {
    ::memset(ccred,0,sizeof(gss_cred_id_desc));
    ccred->cred_handle = chandle; chandle = NULL;
    // cred_usage
    // ssl_context
    X509* identity_cert = NULL;
    if(cert) {
      globus_gsi_cert_utils_cert_type_t ctype = GLOBUS_GSI_CERT_UTILS_TYPE_DEFAULT;
      Arc::GlobusResult(globus_gsi_cert_utils_get_cert_type(cert,&ctype));
      if(ctype == GLOBUS_GSI_CERT_UTILS_TYPE_EEC) {
        identity_cert = cert;
      };
    };
    if(!identity_cert && cchain) {
      // For compatibility with older globus not using
      //Arc::GlobusResult(globus_gsi_cert_utils_get_identity_cert(cchain,&identity_cert));
      for(int n = 0; n < sk_X509_num(cchain); ++n) {
        X509* tmp_cert = sk_X509_value(cchain, n);
        if(tmp_cert) {
          globus_gsi_cert_utils_cert_type_t ctype = GLOBUS_GSI_CERT_UTILS_TYPE_DEFAULT;
          Arc::GlobusResult(globus_gsi_cert_utils_get_cert_type(tmp_cert,&ctype));
          if(ctype == GLOBUS_GSI_CERT_UTILS_TYPE_EEC) {
            identity_cert = tmp_cert;
            break;
          };
        };
      };
    }
    gss_buffer_desc peer_buffer;
#if GLOBUS_GSSAPI_GSI_OLD_OPENSSL
    peer_buffer.value = identity_cert;
    peer_buffer.length = identity_cert?sizeof(X509):0;
#else
    peer_buffer.value = identity_cert;
    peer_buffer.length = identity_cert?sizeof(X509*):0; // Globus expects this size despite strored value is X509, not X509*
#endif
    OM_uint32 majstat, minstat;
    majstat = gss_import_name(&minstat, &peer_buffer,
                              identity_cert?GLOBUS_GSS_C_NT_X509:GSS_C_NT_ANONYMOUS,
                              &ccred->globusid);
    if (GSS_ERROR(majstat)) {
      logger.msg(Arc::ERROR, "Failed to convert GSI credential to "
         "GSS credential (major: %d, minor: %d)", majstat, minstat);
      majstat = gss_release_cred(&minstat, &ccred);
    };
  } else {
    ccred = GSS_C_NO_CREDENTIAL;
  };
  if(cert) X509_free(cert);
  if(key) EVP_PKEY_free(key);
  if(cchain) sk_X509_pop_free(cchain, X509_free);
  if(chandle) Arc::GlobusResult(globus_gsi_cred_handle_destroy(chandle));
  return ccred;
}

int main(int argc,char* argv[]) {
  Arc::LogStream err(std::cerr);
  err.setFormat(Arc::EmptyFormat);
  Arc::Logger::rootLogger.addDestination(err);
  std::string lcas_library;
  std::string lcas_db_file;
  std::string lcas_dir;
  std::string subject;
  std::string filename;

  if(argc > 1) subject = argv[1];
  if(subject.empty()) {
    logger.msg(Arc::ERROR, "Missing subject name");
    return -1;
  };
  if(argc > 2) filename = argv[2];
  if(filename.empty()) {
    logger.msg(Arc::ERROR, "Missing path of credentials file");
    return -1;
  };
  if(argc > 3) lcas_library = argv[3];
  if(lcas_library.empty()) {
    logger.msg(Arc::ERROR, "Missing name of LCAS library");
    return -1;
  };
  if(argc > 4) lcas_dir = argv[4];
  if(argc > 5) lcas_db_file = argv[5];

  if(lcas_dir == "*") lcas_dir.resize(0);
  if(lcas_db_file == "*") lcas_db_file.resize(0);

  if((lcas_library[0] != G_DIR_SEPARATOR) && (lcas_library[0] != '.')) {
    if(lcas_dir.length() != 0) lcas_library=lcas_dir+G_DIR_SEPARATOR_S+lcas_library;
  };

  set_lcas_env(lcas_db_file,lcas_dir);
  Glib::Module lcas_handle(lcas_library,Glib::ModuleFlags(0));
  if(!lcas_handle) {
    recover_lcas_env();
    logger.msg(Arc::ERROR, "Can't load LCAS library %s: %s", lcas_library, Glib::Module::get_last_error());
    return -1;
  };
  void *lcas_init_p = NULL;
  void *lcas_get_fabric_authorization_p = NULL;
  void *lcas_term_p = NULL;
  if((!lcas_handle.get_symbol("lcas_init",lcas_init_p)) ||
     (!lcas_handle.get_symbol("lcas_get_fabric_authorization",lcas_get_fabric_authorization_p)) ||
     (!lcas_handle.get_symbol("lcas_term",lcas_term_p))) {
    recover_lcas_env();
    logger.msg(Arc::ERROR, "Can't find LCAS functions in a library %s", lcas_library);
    return -1;
  };
  lcas_init_t lcas_init_f = (lcas_init_t)lcas_init_p;
  lcas_get_fabric_authorization_t lcas_get_fabric_authorization_f =
    (lcas_get_fabric_authorization_t)lcas_get_fabric_authorization_p;
  lcas_term_t lcas_term_f = (lcas_term_t)lcas_term_p;
  FILE* lcas_log = fdopen(STDERR_FILENO,"a");
  if((*lcas_init_f)(lcas_log) != 0) {
    recover_lcas_env();
    logger.msg(Arc::ERROR, "Failed to initialize LCAS");
    return -1;
  };
  // In case anything is not initialized yet
  Arc::GlobusResult(globus_module_activate(GLOBUS_GSI_GSSAPI_MODULE));
  Arc::GlobusResult(globus_module_activate(GLOBUS_GSI_CREDENTIAL_MODULE));
  Arc::GlobusResult(globus_module_activate(GLOBUS_GSI_CERT_UTILS_MODULE));
  // User without credentials is useless for LCAS ?
  //Arc::GSSCredential cred(filename,"","");
  gss_cred_id_t cred = read_globus_credentials(filename);
  int res = 1;
  if((*lcas_get_fabric_authorization_f)((char*)(subject.c_str()),(gss_cred_id_t)cred,(char*)"") == 0) {
    res=0;
  };
  if((*lcas_term_f)() != 0) {
    logger.msg(Arc::WARNING, "Failed to terminate LCAS");
  };
  if(cred != GSS_C_NO_CREDENTIAL) {
    OM_uint32 majstat, minstat;
    majstat = gss_release_cred(&minstat, &cred);
  };
  recover_lcas_env();
  Arc::GlobusResult(globus_module_deactivate(GLOBUS_GSI_CERT_UTILS_MODULE));
  Arc::GlobusResult(globus_module_deactivate(GLOBUS_GSI_CREDENTIAL_MODULE));
  Arc::GlobusResult(globus_module_deactivate(GLOBUS_GSI_GSSAPI_MODULE));
  return res;
}
