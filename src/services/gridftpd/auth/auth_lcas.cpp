#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <dlfcn.h>
#include <stdio.h>

#include <arc/Thread.h>
#include <arc/Logger.h>

#include "../misc/escaped.h"
#include "../misc/proxy.h"
#include "auth.h"

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
#warning Using hardcoded definition of LCAS functions - software will break if interface changed
extern "C" {
typedef char* lcas_request_t;
typedef int (*lcas_init_t)(FILE *fp);
typedef int (*lcas_term_t)(void);
typedef int (*lcas_get_fabric_authorization_t)(char *user_dn_tmp,gss_cred_id_t user_cred,lcas_request_t request);
}
#endif

static Arc::SimpleCondition lcas_global_lock;
static std::string lcas_db_file_old;
static std::string lcas_dir_old;

void set_lcas_env(const std::string& lcas_db_file,const std::string& lcas_dir) {
  lcas_global_lock.lock();
  char* s;
  s=getenv("LCAS_DB_FILE"); if(s) lcas_db_file_old=s;
  if(lcas_db_file.length() != 0) setenv("LCAS_DB_FILE",lcas_db_file.c_str(),1);
  s=getenv("LCAS_DIR"); if(s) lcas_dir_old=s;
  if(lcas_dir.length() != 0) setenv("LCAS_DIR",lcas_dir.c_str(),1);
}

void recover_lcas_env(void) {
  if(lcas_db_file_old.length() == 0) {
    unsetenv("LCAS_DB_FILE");
  } else {
    setenv("LCAS_DB_FILE",lcas_db_file_old.c_str(),1);
  };
  if(lcas_dir_old.length() == 0) {
    unsetenv("LCAS_DIR");
  } else {
    setenv("LCAS_DIR",lcas_dir_old.c_str(),1);
  };
  lcas_global_lock.unlock();
}

int AuthUser::match_lcas(const char* line) {
  int n;
  int res = AAA_NO_MATCH;
  std::string lcas_library;
  std::string lcas_db_file;
  std::string lcas_dir;
  n=gridftpd::input_escaped_string(line,lcas_library,' ','"'); line+=n;
  if(lcas_library.length() == 0) {
    logger.msg(Arc::ERROR, "Missing name of LCAS library");
    return AAA_FAILURE;
  };
  n=gridftpd::input_escaped_string(line,lcas_dir,' ','"'); line+=n;
  if(n != 0) { n=gridftpd::input_escaped_string(line,lcas_db_file,' ','"');  line+=n; };
  if(lcas_dir == "*") lcas_dir.resize(0);
  if(lcas_db_file == "*") lcas_db_file.resize(0);
  if((lcas_library[0] != '/') && (lcas_library[0] != '.')) {
    if(lcas_dir.length() != 0) lcas_library=lcas_dir+"/"+lcas_library;
  };
  set_lcas_env(lcas_db_file,lcas_dir);
  void* lcas_handle = dlopen(lcas_library.c_str(),RTLD_NOW | RTLD_GLOBAL);
  if(lcas_handle == NULL) {
    recover_lcas_env();
    logger.msg(Arc::ERROR, "Can't load LCAS library %s: %s", lcas_library, dlerror());
    return AAA_FAILURE;
  };
  lcas_init_t lcas_init_f = (lcas_init_t)dlsym(lcas_handle,"lcas_init");
  lcas_get_fabric_authorization_t lcas_get_fabric_authorization_f = (lcas_get_fabric_authorization_t)dlsym(lcas_handle,"lcas_get_fabric_authorization");
  lcas_term_t lcas_term_f = (lcas_term_t)dlsym(lcas_handle,"lcas_term");
  if((lcas_init_f == NULL) ||
     (lcas_get_fabric_authorization_f == NULL) ||
     (lcas_term_f == NULL)) {
    dlclose(lcas_handle);
    recover_lcas_env();
    logger.msg(Arc::ERROR, "Can't find LCAS functions in a library %s", lcas_library);
    return AAA_FAILURE;
  };
  FILE* lcas_log = fdopen(STDERR_FILENO,"a");
  if((*lcas_init_f)(lcas_log) != 0) {
    dlclose(lcas_handle);
    recover_lcas_env();
    logger.msg(Arc::ERROR, "Failed to initialize LCAS");
    return AAA_FAILURE;
  };
  gss_cred_id_t cred = NULL;
  if(filename.length() != 0) {
    // User without credentials is useless for LCAS ?
    cred=gridftpd::read_proxy(filename.c_str());
  };
  if((*lcas_get_fabric_authorization_f)((char*)(subject.c_str()),cred,(char*)"") == 0) {
    res=AAA_POSITIVE_MATCH;
  };
  gridftpd::free_proxy(cred);
  if((*lcas_term_f)() != 0) {
    logger.msg(Arc::ERROR, "Failed to terminate LCAS - has to keep library loaded");
  } else {
    dlclose(lcas_handle);
  };
  recover_lcas_env();
  return res;
}
