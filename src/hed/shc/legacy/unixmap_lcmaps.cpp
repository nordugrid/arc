#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <dlfcn.h>
#include <stdio.h>

#include <arc/Logger.h>
#include <arc/Thread.h>

//#include "../misc/escaped.h"
//#include "../misc/proxy.h"
//#include "../run/run_plugin.h"
#include "unixmap.h"

namespace ArcSHCLegacy {

static Arc::Logger logger(Arc::Logger::getRootLogger(),"UnixMap");

/*
#ifdef HAVE_LCMAPS
#define ALLOW_EMPTY_CREDENTIALS 1
#define LCMAPS_GSI_MODE 1
extern "C" {
#define extern typedef
#define lcmaps_init (*lcmaps_init_t)
#define lcmaps_run_and_return_username (*lcmaps_run_and_return_username_t)
#define lcmaps_term (*lcmaps_term_t)
#include <lcmaps.h>
#undef lcmaps_init
#undef lcmaps_run_and_return_username
#undef lcmaps_term
#undef extern
}
#else
//#warning Using hardcoded definition of LCMAPS functions - software will break during runtime if interface changed
extern "C" {
typedef char* lcmaps_request_t;
typedef int (*lcmaps_init_t)(FILE *fp);
typedef int (*lcmaps_run_and_return_username_t)(char *user_dn_tmp,gss_cred_id_t user_cred,lcmaps_request_t request,char **usernamep,int npols,char **policynames);
typedef int (*lcmaps_term_t)(void);
}
#endif

static Arc::SimpleCondition lcmaps_global_lock;
static std::string lcmaps_db_file_old;
static std::string lcmaps_dir_old;

void set_lcmaps_env(const std::string& lcmaps_db_file,const std::string& lcmaps_dir) {
  lcmaps_global_lock.lock();
  char* s;
  s=getenv("LCMAPS_DB_FILE"); if(s) lcmaps_db_file_old=s;
  if(lcmaps_db_file.length() != 0) setenv("LCMAPS_DB_FILE",lcmaps_db_file.c_str(),1);
  s=getenv("LCMAPS_DIR"); if(s) lcmaps_dir_old=s;
  if(lcmaps_dir.length() != 0) setenv("LCMAPS_DIR",lcmaps_dir.c_str(),1);
}

void recover_lcmaps_env(void) {
  if(lcmaps_db_file_old.length() == 0) {
    unsetenv("LCMAPS_DB_FILE");
  } else {
    setenv("LCMAPS_DB_FILE",lcmaps_db_file_old.c_str(),1);
  };
  if(lcmaps_dir_old.length() == 0) {
    unsetenv("LCMAPS_DIR");
  } else {
    setenv("LCMAPS_DIR",lcmaps_dir_old.c_str(),1);
  };
  lcmaps_global_lock.unlock();
}
*/

bool UnixMap::map_lcmaps(const AuthUser& user,unix_user_t& unix_user,const char* line) {
/*
  int n;
  bool res = false;
  std::string lcmaps_library;
  std::string lcmaps_db_file;
  std::string lcmaps_dir;
  n=gridftpd::input_escaped_string(line,lcmaps_library,' ','"'); line+=n;
  if(lcmaps_library.length() == 0) {
    logger.msg(Arc::ERROR, "Missing name of LCMAPS library");
    return false;
  };
  n=gridftpd::input_escaped_string(line,lcmaps_dir,' ','"'); line+=n;
  if(n != 0) { n=gridftpd::input_escaped_string(line,lcmaps_db_file,' ','"'); line+=n; };
  if(lcmaps_dir == "*") lcmaps_dir.resize(0);
  if(lcmaps_db_file == "*") lcmaps_db_file.resize(0);
  if((lcmaps_library[0] != '/') && (lcmaps_library[0] != '.')) {
    if(lcmaps_dir.length() != 0) lcmaps_library=lcmaps_dir+"/"+lcmaps_library;
  };
  char** policynames = NULL;
  int npols = 0;
  {
    std::string s(line);
    policynames=gridftpd::string_to_args(s);
  };
  if(policynames == NULL) {
    logger.msg(Arc::ERROR, "Can't read policy names");
    return false;
  };
  for(;policynames[npols];npols++) { };
  set_lcmaps_env(lcmaps_db_file,lcmaps_dir);
  void* lcmaps_handle = dlopen(lcmaps_library.c_str(),RTLD_NOW | RTLD_GLOBAL);
  if(lcmaps_handle == NULL) {
    recover_lcmaps_env(); gridftpd::free_args(policynames);
    logger.msg(Arc::ERROR, "Can't load LCMAPS library %s: %s", lcmaps_library, dlerror());
    return false;
  };
  lcmaps_init_t lcmaps_init_f = (lcmaps_init_t)dlsym(lcmaps_handle,"lcmaps_init");
  lcmaps_run_and_return_username_t lcmaps_run_and_return_username_f = (lcmaps_run_and_return_username_t)dlsym(lcmaps_handle,"lcmaps_run_and_return_username");
  lcmaps_term_t lcmaps_term_f = (lcmaps_term_t)dlsym(lcmaps_handle,"lcmaps_term");
  if((lcmaps_init_f == NULL) ||
     (lcmaps_run_and_return_username_f == NULL) ||
     (lcmaps_term_f == NULL)) {
    dlclose(lcmaps_handle);
    recover_lcmaps_env(); gridftpd::free_args(policynames);
    logger.msg(Arc::ERROR, "Can't find LCMAPS functions in a library %s", lcmaps_library);
    return false;
  };
  FILE* lcmaps_log = fdopen(STDERR_FILENO,"a");
  if((*lcmaps_init_f)(lcmaps_log) != 0) {
    dlclose(lcmaps_handle);
    recover_lcmaps_env(); gridftpd::free_args(policynames);
    logger.msg(Arc::ERROR, "Failed to initialize LCMAPS");
    return false;
  };
  gss_cred_id_t cred = NULL;
  if((user.proxy() != NULL) && (user.proxy()[0] != 0)) {
    // User without credentials is useless for LCMAPS ?
    cred=gridftpd::read_proxy(user.proxy());
  };
  char* username = NULL;
  if((*lcmaps_run_and_return_username_f)(
           (char*)(user.DN()),cred,(char*)"",&username,npols,policynames
     ) == 0) {
    if(username != NULL) {
      res=true;
      unix_user.name=username;
    };
  };
  if((*lcmaps_term_f)() != 0) {
    logger.msg(Arc::ERROR, "Failed to terminate LCMAPS - has to keep library loaded");
  } else {
    dlclose(lcmaps_handle);
  };
  gridftpd::free_proxy(cred);
  recover_lcmaps_env(); gridftpd::free_args(policynames);
  return res;
*/
  return false;
}

} // namespace ArcSHCLegacy

