#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <arc/Logger.h>

#include "../conf/environment.h"
#include "run_parallel.h"

#ifdef HAVE_GLIBMM_GETENV
#include <glibmm/miscutils.h>
#define GetEnv(NAME) Glib::getenv(NAME)
#else
#define GetEnv(NAME) (getenv(NAME)?getenv(NAME):"")
#endif

#ifdef HAVE_GLIBMM_SETENV
#include <glibmm/miscutils.h>
#define SetEnv(NAME,VALUE) Glib::setenv(NAME,VALUE)
#else
#ifdef HAVE_SETENV
#define SetEnv(NAME,VALUE) setenv(NAME,VALUE.c_str(),1)
#else
#define SetEnv(NAME,VALUE) { char* __s = strdup((std::string(NAME)+"="+VALUE).c_str()); putenv(__s);  }
#endif
#endif

#ifdef HAVE_GLIBMM_UNSETENV
#include <glibmm/miscutils.h>
#define UnsetEnv(NAME) Glib::unsetenv(NAME)
#else
#ifdef HAVE_UNSETENV
#define UnsetEnv(NAME) unsetenv(NAME)
#else
#define UnsetEnv(NAME) { char* __s = strdup(NAME); putenv(__s);  }
#endif
#endif

typedef struct {
  const JobUser* user;
  const JobDescription* job;
  const char* reason;
} job_subst_t;

static Arc::Logger& logger = Arc::Logger::getRootLogger();

void (*RunParallel::kicker_func_)(void*) = NULL;
void* RunParallel::kicker_arg_ = NULL;

static void job_subst(std::string& str,void* arg) {
  job_subst_t* subs = (job_subst_t*)arg;
  for(std::string::size_type p = 0;;) {
    p=str.find('%',p);
    if(p==std::string::npos) break;
    if(str[p+1]=='I') {
      str.replace(p,2,subs->job->get_id().c_str());
      p+=subs->job->get_id().length();
    } else if(str[p+1]=='S') {
      str.replace(p,2,subs->job->get_state_name());
      p+=strlen(subs->job->get_state_name());
    } else if(str[p+1]=='O') {
      str.replace(p,2,subs->reason);
      p+=strlen(subs->reason);
    } else {
      p+=2;
    };
  };
  subs->user->substitute(str);
}

bool RunParallel::run(JobUser& user,const JobDescription& desc,const char *const args[],Arc::Run** ere,bool su) {
  RunPlugin* cred = user.CredPlugin();
  job_subst_t subs; subs.user=&user; subs.job=&desc; subs.reason="external";
  if((!cred) || (!(*cred))) { cred=NULL; };
  //RunPlugin* cred = NULL;
  if(user.get_uid() == 0) {
    JobUser tmp_user(desc.get_uid());
    if(!tmp_user.is_valid()) return false;
    tmp_user.SetControlDir(user.ControlDir());
    tmp_user.SetSessionRoot(user.SessionRoot());
    return run(tmp_user,desc.get_id().c_str(),args,ere,su,
                                        true,cred,&job_subst,&subs);
  };
  return run(user,desc.get_id().c_str(),args,ere,su,
                                      true,cred,&job_subst,&subs);
}

/* fork & execute child process with stderr redirected 
   to job.ID.errors, stdin and stdout to /dev/null */
bool RunParallel::run(JobUser& user,const char* jobid,const char *const args[],Arc::Run** ere,bool su,bool job_proxy,RunPlugin* cred,RunPlugin::substitute_t subst,void* subst_arg) {
  *ere=NULL;
  std::list<std::string> args_;
  for(int n = 0;args[n];++n) args_.push_back(std::string(args[n]));
  Arc::Run* re = new Arc::Run(args_);
  if((!re) || (!(*re))) {
    if(re) delete re;
    logger.msg(Arc::ERROR,"%s: Failure creating slot for child process.",jobid?jobid:"");
    return false;
  };
  if(kicker_func_) re->AssignKicker(kicker_func_,kicker_arg_);
  RunParallel* rp = new RunParallel(user,jobid,su,job_proxy,cred,subst,subst_arg);
  if((!rp) || (!(*rp))) {
    if(rp) delete rp;
    delete re;
    logger.msg(Arc::ERROR,"%s: Failure creating data storage for child process.",jobid?jobid:"");
    return false;
  };
  re->AssignInitializer(&initializer,rp);
  if(!re->Start()) {
    delete rp;
    delete re;
    logger.msg(Arc::ERROR,"%s: Failure starting child process.",jobid?jobid:"");
    return false;
  };
  delete rp;
  *ere=re;
  return true;
}

void RunParallel::initializer(void* arg) {
#ifdef WIN32
#error This functionality is not available in Windows environement
#else
  // child
  RunParallel* it = (RunParallel*)arg;
  struct rlimit lim;
  int max_files;
  if(getrlimit(RLIMIT_NOFILE,&lim) == 0) { max_files=lim.rlim_cur; }
  else { max_files=4096; };
  // change user
  if(!(it->user_.SwitchUser(it->su_))) {
    logger.msg(Arc::ERROR,"%s: Failed switching user",it->jobid_); sleep(10); exit(1);
  };
  if(it->cred_) {
    // run external plugin to acquire non-unix local credentials
    if(!it->cred_->run(it->subst_,it->subst_arg_)) {
      logger.msg(Arc::ERROR,"%s: Failed to run plugin",it->jobid_); sleep(10); _exit(1);
    };
    if(it->cred_->result() != 0) {
      logger.msg(Arc::ERROR,"%s: Plugin failed",it->jobid_); sleep(10); _exit(1);
    };
  };
  // close all handles inherited from parent
  if(max_files == RLIM_INFINITY) max_files=4096;
  for(int i=0;i<max_files;i++) { close(i); };
  int h;
  // set up stdin,stdout and stderr
  h=open("/dev/null",O_RDONLY); 
  if(h != 0) { if(dup2(h,0) != 0) { sleep(10); exit(1); }; close(h); };
  h=open("/dev/null",O_WRONLY);
  if(h != 1) { if(dup2(h,1) != 1) { sleep(10); exit(1); }; close(h); };
  std::string errlog;
  if(!(it->jobid_.empty())) { 
    errlog = it->user_.ControlDir() + "/job." + it->jobid_ + ".errors";
    h=open(errlog.c_str(),O_WRONLY | O_CREAT | O_APPEND,S_IRUSR | S_IWUSR);
    if(h==-1) { h=open("/dev/null",O_WRONLY); };
  }
  else { h=open("/dev/null",O_WRONLY); };
  if(h != 2) { if(dup2(h,2) != 2) { sleep(10); exit(1); }; close(h); };
  // setting environment  - TODO - better environment 
  if(it->job_proxy_) {
    SetEnv("GLOBUS_LOCATION",globus_loc);
    UnsetEnv("X509_USER_KEY");
    UnsetEnv("X509_USER_CERT");
    UnsetEnv("X509_USER_PROXY");
    UnsetEnv("X509_RUN_AS_SERVER");
    if(!(it->jobid_.empty())) {
      std::string proxy = it->user_.ControlDir() + "/job." + it->jobid_ + ".proxy";
      SetEnv("X509_USER_PROXY",proxy);
      // for Globus 2.2 set fake cert and key, or else it takes 
      // those from host in case of root user.
      // 2.4 needs names and 2.2 will work too.
      // 3.x requires fake ones again.
#if GLOBUS_IO_VERSION>=5
      SetEnv("X509_USER_KEY",(std::string("fake")));
      SetEnv("X509_USER_CERT",(std::string("fake")));
#else
      SetEnv("X509_USER_KEY",proxy);
      SetEnv("X509_USER_CERT",proxy);
#endif
    };
  };
  //# execv(args[0],args);
  //# perror("execv");
  //# std::cerr<<(jobid?jobid:"")<<"Failed to start external program: "<<args[0]<<std::endl;
  //# sleep(10); exit(1);
#endif
}

