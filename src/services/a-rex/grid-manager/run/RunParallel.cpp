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
#include "../files/ControlFileHandling.h"
#include "RunParallel.h"

namespace ARex {

static Arc::Logger& logger = Arc::Logger::getRootLogger();

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

bool RunParallel::run(const GMConfig& config,const GMJob& job, JobsList& list, std::string* errstr,
                      const std::string& args,Arc::Run** ere,bool su) {
  std::string errlog = job_control_path(config.ControlDir(),job.get_id(),sfx_errors);
  std::string proxy = job_control_path(config.ControlDir(),job.get_id(),sfx_proxy);
  JobRefInList* ref = new JobRefInList(job, list);
  bool result = run(config, job.get_user(), job.get_id().c_str(), errlog.c_str(), errstr,
             args, ere, proxy.c_str(), su, &JobRefInList::kicker, ref);
  if(!result) delete ref;
  return result;
}

bool RunParallel::run(const GMConfig& config,const GMJob& job, std::string* errstr,
                      const std::string& args,Arc::Run** ere,bool su) {
  std::string errlog = job_control_path(config.ControlDir(),job.get_id(),sfx_errors);
  std::string proxy = job_control_path(config.ControlDir(),job.get_id(),sfx_proxy);
  bool result = run(config, job.get_user(), job.get_id().c_str(), errlog.c_str(), errstr,
             args, ere, proxy.c_str(), su);
  return result;
}

/* fork & execute child process with stderr redirected 
   to job.ID.errors, stdin and stdout to /dev/null */
bool RunParallel::run(const GMConfig& config, const Arc::User& user,
                      const char* procid, const char* errlog, std::string* errstr,
                      const std::string& args, Arc::Run** ere,
                      const char* jobproxy, bool su,
                      void (*kicker_func)(void*), void* kicker_arg) {
  *ere=NULL;
  Arc::Run* re = new Arc::Run(args);
  if((!re) || (!(*re))) {
    if(re) delete re;
    logger.msg(Arc::ERROR,"%s: Failure creating slot for child process",procid?procid:"");
    return false;
  };
  if(kicker_func) re->AssignKicker(kicker_func,kicker_arg);
  re->AssignInitializer(&initializer,(void*)errlog,false);
  if(su) {
    // change user
    re->AssignUserId(user.get_uid());
    re->AssignGroupId(user.get_gid());
  };
  // setting environment  - TODO - better environment 
  if(jobproxy && jobproxy[0]) {
    re->RemoveEnvironment("X509_RUN_AS_SERVER");

    re->AddEnvironment("X509_USER_PROXY",jobproxy);
    // for Globus 2.2 set fake cert and key, or else it takes 
    // those from host in case of root user.
    // 2.4 needs names and 2.2 will work too.
    // 3.x requires fake ones again.
#if GLOBUS_IO_VERSION>=5
    re->AddEnvironment("X509_USER_KEY",(std::string("fake")));
    re->AddEnvironment("X509_USER_CERT",(std::string("fake")));
#else
    re->AddEnvironment("X509_USER_KEY",jobproxy);
    re->AddEnvironment("X509_USER_CERT",jobproxy);
#endif
    std::string cert_dir = config.CertDir();
    if(!cert_dir.empty()) {
      re->AddEnvironment("X509_CERT_DIR",cert_dir);
    } else {
      re->RemoveEnvironment("X509_CERT_DIR");
    };
    std::string voms_dir = config.VomsDir();
    if(!voms_dir.empty()) {
      re->AddEnvironment("X509_VOMS_DIR",voms_dir);
    } else {
      re->RemoveEnvironment("X509_VOMS_DIR");
    };
  };
  re->KeepStdin(true);
  if(errstr) {
    re->KeepStdout(false);
    re->AssignStdout(*errstr, 1024); // Expecting short failure reason here. Rest goes into .errors file.
  } else {
    re->KeepStdout(true);
  };
  re->KeepStderr(true);
  if(!re->Start()) {
    delete re;
    logger.msg(Arc::ERROR,"%s: Failure starting child process",procid?procid:"");
    return false;
  };
  *ere=re;
  return true;
}

void RunParallel::initializer(void* arg) {
  // child
  char const * errlog = (char const *)arg;
  int h;
  // set up stdin,stdout and stderr
  h=::open("/dev/null",O_RDONLY); 
  if(h != 0) { if(dup2(h,0) != 0) { _exit(1); }; close(h); };
  h=::open("/dev/null",O_WRONLY);
  if(h != 1) { if(dup2(h,1) != 1) { _exit(1); }; close(h); };
  if(errlog) { 
    h=::open(errlog,O_WRONLY | O_CREAT | O_APPEND,S_IRUSR | S_IWUSR);
    if(h==-1) { h=::open("/dev/null",O_WRONLY); };
  }
  else { h=::open("/dev/null",O_WRONLY); };
  if(h != 2) { if(dup2(h,2) != 2) { _exit(1); }; close(h); };
}

} // namespace ARex
