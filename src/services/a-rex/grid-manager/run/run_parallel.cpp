#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//@ #include "../std.h"
#include <dlfcn.h>

#include <string>

#include "../jobs/users.h"
#include "../jobs/states.h"
#include <sys/resource.h>
#include <sys/wait.h>
#include <pthread.h>
#include "../conf/environment.h"
#include "../conf/conf.h"
//@ #include "../misc/substitute.h"
//@ #include "../misc/log_time.h"
#include "run_parallel.h"
 
//@
//@ #include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define olog std::cerr
//@


extern char** environ;

typedef struct {
  const JobUser* user;
  const JobDescription* job;
  const char* reason;
} job_subst_t;

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

bool RunParallel::run(JobUser& user,const JobDescription& desc,char *const args[],RunElement** ere,bool su) {
  RunPlugin* cred = user.CredPlugin();
  job_subst_t subs; subs.user=&user; subs.job=&desc; subs.reason="external";
  if((!cred) || (!(*cred))) { cred=NULL; };
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
bool RunParallel::run(JobUser& user,const char* jobid,char *const args[],RunElement** ere,bool su,bool job_proxy,RunPlugin* cred,RunPlugin::substitute_t subst,void* subst_arg) {
  (*ere)=NULL;
  /* create slot */
  RunElement* re = add_handled();
  if(re == NULL) {
    olog<<(jobid?jobid:"")<<": Failure creating slot for child process."<<std::endl;
    return false;
  };
  block();
  pid_t* p_pid = &(re->pid);
  // { sigset_t sig; sigemptyset(&sig);sigaddset(&sig,SIGCHLD); if(sigprocmask(SIG_BLOCK,&sig,NULL)) perror("sigprocmask"); };
  (*p_pid)=fork();
  if((*p_pid) == -1) {
    //{ sigset_t sig; sigemptyset(&sig);sigaddset(&sig,SIGCHLD); if(sigprocmask(SIG_UNBLOCK,&sig,NULL)) perror("sigprocmask"); };
    unblock();
    release(re);
    olog<<(jobid?jobid:"")<<": Failure forking child process."<<std::endl;
    return false;
  };
  if((*p_pid) != 0) { /* parent */
    //{ sigset_t sig; sigemptyset(&sig);sigaddset(&sig,SIGCHLD); if(sigprocmask(SIG_UNBLOCK,&sig,NULL)) perror("sigprocmask"); };
    unblock();
    (*ere)=re;
    return true;
  };
  /* child */
  sched_yield(); // let parent write child's pid into list in case sigprocmask does not work.
  struct rlimit lim;
  int max_files;
  if(getrlimit(RLIMIT_NOFILE,&lim) == 0) { max_files=lim.rlim_cur; }
  else { max_files=4096; };
  /* change user */
  if(!(user.SwitchUser(su))) {
    olog<<(jobid?jobid:"")<<": Failed switching user"<<std::endl; sleep(10); exit(1);
  };
  if(cred) {
    // run external plugin to acquire non-unix local credentials
    if(!cred->run(subst,subst_arg)) {
      olog<<(jobid?jobid:"")<<": Failed to run plugin"<<std::endl; sleep(10); exit(1);
    };
    if(cred->result() != 0) {
      olog<<(jobid?jobid:"")<<": Plugin failed"<<std::endl; sleep(10); exit(1);
    };
  };
  /* close all handles inherited from parent */
  if(max_files == RLIM_INFINITY) max_files=4096;
  for(int i=0;i<max_files;i++) { close(i); };
  int h;
  /* set up stdin,stdout and stderr */
  h=open("/dev/null",O_RDONLY); 
  if(h != 0) { if(dup2(h,0) != 0) { sleep(10); exit(1); }; close(h); };
  h=open("/dev/null",O_WRONLY);
  if(h != 1) { if(dup2(h,1) != 1) { sleep(10); exit(1); }; close(h); };
  std::string errlog;
  if(jobid) { 
    errlog = user.ControlDir() + "/job." + jobid + ".errors";
    h=open(errlog.c_str(),O_WRONLY | O_CREAT | O_APPEND,S_IRUSR | S_IWUSR);
    if(h==-1) { h=open("/dev/null",O_WRONLY); };
  }
  else { h=open("/dev/null",O_WRONLY); };
  if(h != 2) { if(dup2(h,2) != 2) { sleep(10); exit(1); }; close(h); };
  /* setting environment  - TODO - better environment */
  if(job_proxy) {
    setenv("GLOBUS_LOCATION",globus_loc.c_str(),1);
    unsetenv("X509_USER_KEY");
    unsetenv("X509_USER_CERT");
    unsetenv("X509_USER_PROXY");
    unsetenv("X509_RUN_AS_SERVER");
    if(jobid) {
      std::string proxy = user.ControlDir() + "/job." + jobid + ".proxy";
      setenv("X509_USER_PROXY",proxy.c_str(),1);
      /* for Globus 2.2 set fake cert and key, or else it takes 
         those from host in case of root user.
         2.4 needs names and 2.2 will work too.
         3.x requires fake ones again.
      */
#if GLOBUS_IO_VERSION>=5
      setenv("X509_USER_KEY","fake",1);
      setenv("X509_USER_CERT","fake",1);
#else
      setenv("X509_USER_KEY",proxy.c_str(),1);
      setenv("X509_USER_CERT",proxy.c_str(),1);
#endif
    };
  };
  execv(args[0],args);
  perror("execv");
  std::cerr<<(jobid?jobid:"")<<"Failed to start external program: "<<args[0]<<std::endl;
  sleep(10); exit(1);
}

