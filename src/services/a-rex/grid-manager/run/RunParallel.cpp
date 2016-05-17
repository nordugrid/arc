#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <arc/Logger.h>
#include <arc/Utils.h>
#include <arc/FileUtils.h>

#include "../conf/GMConfig.h"
#include "RunParallel.h"

namespace ARex {

typedef struct {
  const GMConfig* config;
  const GMJob* job;
  const char* reason;
} job_subst_t;

static Arc::Logger& logger = Arc::Logger::getRootLogger();

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
  subs->config->Substitute(str, subs->job->get_user());
}

class JobRefInList {
 private:
  JobId id;
  JobsList& list;
 public:
  JobRefInList(const GMJob& job, JobsList& list): id(job.get_id()), list(list) {};
  static void kicker(void* arg);
};

void JobRefInList::kicker(void* arg) {
  JobRefInList* ref = reinterpret_cast<JobRefInList*>(arg);
  if(ref) {
    logger.msg(Arc::DEBUG,"%s: Job's helper exited",ref->id);
    ref->list.RequestAttention(ref->id);
    delete ref;
  };
}

bool RunParallel::run(const GMConfig& config,const GMJob& job, JobsList& list,
                      const std::string& args,Arc::Run** ere,bool su) {
  RunPlugin* cred = config.CredPlugin();
  job_subst_t subs; subs.config=&config; subs.job=&job; subs.reason="external";
  if((!cred) || (!(*cred))) { cred=NULL; };
  std::string errlog = config.ControlDir()+"/job."+job.get_id()+".errors";
  std::string proxy = config.ControlDir() + "/job." + job.get_id() + ".proxy";
  JobRefInList* ref = new JobRefInList(job, list);
  bool result = run(config, job.get_user(), job.get_id().c_str(), errlog.c_str(),
             args, ere, proxy.c_str(), su, cred, &job_subst, &subs, &JobRefInList::kicker, ref);
  if(!result) delete ref;
  return result;
}

bool RunParallel::run(const GMConfig& config,const GMJob& job,
                      const std::string& args,Arc::Run** ere,bool su) {
  RunPlugin* cred = config.CredPlugin();
  job_subst_t subs; subs.config=&config; subs.job=&job; subs.reason="external";
  if((!cred) || (!(*cred))) { cred=NULL; };
  std::string errlog = config.ControlDir()+"/job."+job.get_id()+".errors";
  std::string proxy = config.ControlDir() + "/job." + job.get_id() + ".proxy";
  bool result = run(config, job.get_user(), job.get_id().c_str(), errlog.c_str(),
             args, ere, proxy.c_str(), su, cred, &job_subst, &subs);
  return result;
}

/* fork & execute child process with stderr redirected 
   to job.ID.errors, stdin and stdout to /dev/null */
bool RunParallel::run(const GMConfig& config, const Arc::User& user,
                      const char* procid, const char* errlog,
                      const std::string& args, Arc::Run** ere,
                      const char* jobproxy, bool su,
                      RunPlugin* cred,
                      RunPlugin::substitute_t subst, void* subst_arg,
                      void (*kicker_func)(void*), void* kicker_arg) {
  *ere=NULL;
  Arc::Run* re = new Arc::Run(args);
  if((!re) || (!(*re))) {
    if(re) delete re;
    logger.msg(Arc::ERROR,"%s: Failure creating slot for child process",procid?procid:"");
    return false;
  };
  if(kicker_func) re->AssignKicker(kicker_func,kicker_arg);
  RunParallel* rp = new RunParallel(config,user,procid,errlog,jobproxy,su,cred,subst,subst_arg);
  if((!rp) || (!(*rp))) {
    if(rp) delete rp;
    delete re;
    logger.msg(Arc::ERROR,"%s: Failure creating data storage for child process",procid?procid:"");
    return false;
  };
  re->AssignInitializer(&initializer,rp);
  if(!re->Start()) {
    delete rp;
    delete re;
    logger.msg(Arc::ERROR,"%s: Failure starting child process",procid?procid:"");
    return false;
  };
  delete rp;
  *ere=re;
  return true;
}

void RunParallel::initializer(void* arg) {
#ifdef WIN32
#error This functionality is not available in Windows environment
#else
  // child
  RunParallel* it = (RunParallel*)arg;
  struct rlimit lim;
  int max_files;
  if(getrlimit(RLIMIT_NOFILE,&lim) == 0) { max_files=lim.rlim_cur; }
  else { max_files=4096; };
  // change user
  if(it->su_) {
    if(!(it->user_.SwitchUser())) {
      logger.msg(Arc::ERROR,"%s: Failed switching user",it->procid_); sleep(10); exit(1);
    };
  } else {
    // just set good umask
    umask(0077);
  }
  if(it->cred_) {
    // run external plugin to acquire non-unix local credentials
    if(!it->cred_->run(it->subst_,it->subst_arg_)) {
      logger.msg(Arc::ERROR,"%s: Failed to run plugin",it->procid_); sleep(10); _exit(1);
    };
    if(it->cred_->result() != 0) {
      logger.msg(Arc::ERROR,"%s: Plugin failed",it->procid_); sleep(10); _exit(1);
    };
  };
  // close all handles inherited from parent
  if(max_files == RLIM_INFINITY) max_files=4096;
  for(int i=0;i<max_files;i++) { close(i); };
  int h;
  // set up stdin,stdout and stderr
  h=::open("/dev/null",O_RDONLY); 
  if(h != 0) { if(dup2(h,0) != 0) { sleep(10); exit(1); }; close(h); };
  h=::open("/dev/null",O_WRONLY);
  if(h != 1) { if(dup2(h,1) != 1) { sleep(10); exit(1); }; close(h); };
  std::string errlog;
  if(!(it->errlog_.empty())) { 
    h=::open(it->errlog_.c_str(),O_WRONLY | O_CREAT | O_APPEND,S_IRUSR | S_IWUSR);
    if(h==-1) { h=::open("/dev/null",O_WRONLY); };
  }
  else { h=::open("/dev/null",O_WRONLY); };
  if(h != 2) { if(dup2(h,2) != 2) { sleep(10); exit(1); }; close(h); };
  // setting environment  - TODO - better environment 
  if(!it->jobproxy_.empty()) {
    Arc::UnsetEnv("X509_USER_KEY");
    Arc::UnsetEnv("X509_USER_CERT");
    Arc::UnsetEnv("X509_USER_PROXY");
    Arc::UnsetEnv("X509_RUN_AS_SERVER");
    Arc::UnsetEnv("X509_CERT_DIR");
    Arc::UnsetEnv("X509_VOMS_DIR");
    Arc::SetEnv("X509_USER_PROXY",it->jobproxy_);
    // for Globus 2.2 set fake cert and key, or else it takes 
    // those from host in case of root user.
    // 2.4 needs names and 2.2 will work too.
    // 3.x requires fake ones again.
#if GLOBUS_IO_VERSION>=5
    Arc::SetEnv("X509_USER_KEY",(std::string("fake")));
    Arc::SetEnv("X509_USER_CERT",(std::string("fake")));
#else
    Arc::SetEnv("X509_USER_KEY",it->jobproxy_);
    Arc::SetEnv("X509_USER_CERT",it->jobproxy_);
#endif
    std::string cert_dir = it->config_.CertDir();
    if(!cert_dir.empty()) Arc::SetEnv("X509_CERT_DIR",cert_dir);
    std::string voms_dir = it->config_.VomsDir();
    if(!voms_dir.empty()) Arc::SetEnv("X509_VOMS_DIR",voms_dir);
  };
#endif
}

} // namespace ARex
