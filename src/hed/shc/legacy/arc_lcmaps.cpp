#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <dlfcn.h>
#include <stdio.h>

#include <arc/Logger.h>
#include <arc/Utils.h>
#include <arc/globusutils/GSSCredential.h>

#include "unixmap.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(),"UnixMap");

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
  lcmaps_db_file_old=Arc::GetEnv("LCMAPS_DB_FILE");
  if(lcmaps_db_file.length() != 0) Arc::SetEnv("LCMAPS_DB_FILE",lcmaps_db_file,true);
  lcmaps_dir_old=Arc::GetEnv("LCMAPS_DIR");
  if(lcmaps_dir.length() != 0) Arc::SetEnv("LCMAPS_DIR",lcmaps_dir,true);
}

void recover_lcmaps_env(void) {
  if(lcmaps_db_file_old.length() == 0) {
    Arc::UnsetEnv("LCMAPS_DB_FILE");
  } else {
    Arc::SetEnv("LCMAPS_DB_FILE",lcmaps_db_file_old,true);
  };
  if(lcmaps_dir_old.length() == 0) {
    Arc::UnsetEnv("LCMAPS_DIR");
  } else {
    Arc::SetEnv("LCMAPS_DIR",lcmaps_dir_old,true);
  };
  lcmaps_global_lock.unlock();
}

int main(int argc,char* argv[]) {
  Arc::LogStream err(std::cerr);
  Arc::Logger::rootLogger.addDestination(err);
  std::string lcmaps_library;
  std::string lcmaps_db_file;
  std::string lcmaps_dir;
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
  if(argc > 3) lcmaps_library = argv[3];
  if(lcmaps_library.empty()) {
    logger.msg(Arc::ERROR, "Missing name of LCMAPS library");
    return -1;
  };
  if(argc > 4) lcmaps_dir = argv[4];
  if(argc > 5) lcmaps_db_file = argv[5];

  if(lcmaps_dir == "*") lcmaps_dir.resize(0);
  if(lcmaps_db_file == "*") lcmaps_db_file.resize(0);

  if((lcmaps_library[0] != G_DIR_SEPARATOR) && (lcmaps_library[0] != '.')) {
    if(lcmaps_dir.length() != 0) lcmaps_library=lcmaps_dir+G_DIR_SEPARATOR_S+lcmaps_library;
  };

  if(argc <= 6) {
    logger.msg(Arc::ERROR, "Can't read policy names");
    return -1;
  };
  char** policynames = argv+6;
  int npols = 0;
  for(;policynames[npols];npols++) { };

  set_lcmaps_env(lcmaps_db_file,lcmaps_dir);
  Glib::Module lcmaps_handle(lcmaps_library,Glib::ModuleFlags(0));
  if(!lcmaps_handle) {
    recover_lcmaps_env();
    logger.msg(Arc::ERROR, "Can't load LCMAPS library %s: %s", lcmaps_library, Glib::Module::get_last_error());
    return -1;
  };
  lcmaps_init_t lcmaps_init_f = NULL;
  lcmaps_run_and_return_username_t lcmaps_run_and_return_username_f = NULL;
  lcmaps_term_t lcmaps_term_f = NULL;
  if((!lcmaps_handle.get_symbol("lcmaps_init",(void*&)lcmaps_init_f)) ||
     (!lcmaps_handle.get_symbol("lcmaps_run_and_return_username",(void*&)lcmaps_run_and_return_username_f)) ||
     (!lcmaps_handle.get_symbol("lcmaps_term",(void*&)lcmaps_term_f))) {
    recover_lcmaps_env();
    logger.msg(Arc::ERROR, "Can't find LCMAPS functions in a library %s", lcmaps_library);
    return -1;
  };
  FILE* lcmaps_log = fdopen(STDERR_FILENO,"a");
  if((*lcmaps_init_f)(lcmaps_log) != 0) {
    recover_lcmaps_env();
    logger.msg(Arc::ERROR, "Failed to initialize LCMAPS");
    return -1;
  };
  // User without credentials is useless for LCMAPS ?
  Arc::GSSCredential cred(filename,"","");
  char* username = NULL;
  int res = 1;
  if((*lcmaps_run_and_return_username_f)(
           (char*)(subject.c_str()),cred,(char*)"",&username,npols,policynames
     ) == 0) {
    if(username != NULL) {
      res=1;
      std::cout<<username<<std::flush;
    };
  };
  if((*lcmaps_term_f)() != 0) {
    logger.msg(Arc::WARNING, "Failed to terminate LCMAPS");
  };
  recover_lcmaps_env();
  return res;
}

