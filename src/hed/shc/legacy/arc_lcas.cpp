#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm.h>

#include <gssapi.h>

#include <arc/Logger.h>
#include <arc/Utils.h>
#include <arc/globusutils/GSSCredential.h>

static Arc::Logger logger(Arc::Logger::getRootLogger(),"AuthUserLCAS");

#ifdef HAVE_LCAS
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

int main(int argc,char* argv[]) {
  Arc::LogStream err(std::cerr);
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
  lcas_init_t lcas_init_f = NULL;
  lcas_get_fabric_authorization_t lcas_get_fabric_authorization_f = NULL;
  lcas_term_t lcas_term_f = NULL;
  if((!lcas_handle.get_symbol("lcas_init",(void*&)lcas_init_f)) ||
     (!lcas_handle.get_symbol("lcas_get_fabric_authorization",(void*&)lcas_get_fabric_authorization_f)) ||
     (!lcas_handle.get_symbol("lcas_term",(void*&)lcas_term_f))) {
    recover_lcas_env();
    logger.msg(Arc::ERROR, "Can't find LCAS functions in a library %s", lcas_library);
    return -1;
  };
  FILE* lcas_log = fdopen(STDERR_FILENO,"a");
  if((*lcas_init_f)(lcas_log) != 0) {
    recover_lcas_env();
    logger.msg(Arc::ERROR, "Failed to initialize LCAS");
    return -1;
  };
  // User without credentials is useless for LCAS ?
  Arc::GSSCredential cred(filename,"","");
  int res = 1;
  if((*lcas_get_fabric_authorization_f)((char*)(subject.c_str()),(gss_cred_id_t)cred,(char*)"") == 0) {
    res=0;
  };
  if((*lcas_term_f)() != 0) {
    logger.msg(Arc::WARNING, "Failed to terminate LCAS");
  };
  recover_lcas_env();
  return res;
}
