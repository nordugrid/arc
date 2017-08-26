#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* write essential information about job started/finished */
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>

#include <arc/ArcLocation.h>
#include <arc/StringConv.h>
#include <arc/DateTime.h>
#include <arc/Run.h>
#include "../files/ControlFileContent.h"
#include "../files/JobLogFile.h"
#include "../conf/GMConfig.h"
#include "JobLog.h"

namespace ARex {

static Arc::Logger& logger = Arc::Logger::getRootLogger();

JobLog::JobLog(void):filename(""),proc(NULL),last_run(0),period(3600) {
}

void JobLog::SetOutput(const char* fname) {
  filename=fname;
}

bool JobLog::SetPeriod(int new_period) {
    if ( new_period < 3600 ) return false;
    period=new_period;
    return true;
}

bool JobLog::open_stream(std::ofstream &o) {
    o.open(filename.c_str(),std::ofstream::app);
    if(!o.is_open()) return false;
    o<<(Arc::Time().str());
    o<<" ";
    return true;
}

bool JobLog::WriteStartInfo(GMJob &job,const GMConfig &config) {
  if(filename.length()==0) return true;
    std::ofstream o;
    if(!open_stream(o)) return false;
    o<<"Started - job id: "<<job.get_id()<<", unix user: "<<job.get_user().get_uid()<<":"<<job.get_user().get_gid()<<", ";
    JobLocalDescription *job_desc = job.GetLocalDescription(config);
    if(job_desc) {
      std::string tmps;
      tmps=job_desc->jobname;
      tmps = Arc::escape_chars(tmps, "\"\\", '\\', false);
      o<<"name: \""<<tmps<<"\", ";
      tmps=job_desc->DN;
      tmps = Arc::escape_chars(tmps, "\"\\", '\\', false);
      o<<"owner: \""<<tmps<<"\", ";
      o<<"lrms: "<<job_desc->lrms<<", queue: "<<job_desc->queue;
    };
    o<<std::endl;
    o.close();
    return true;
}

bool JobLog::WriteFinishInfo(GMJob &job,const GMConfig &config) {
  if(filename.length()==0) return true;
    std::ofstream o;
    if(!open_stream(o)) return false;
    o<<"Finished - job id: "<<job.get_id()<<", unix user: "<<job.get_user().get_uid()<<":"<<job.get_user().get_gid()<<", ";
    std::string tmps;
    JobLocalDescription *job_desc = job.GetLocalDescription(config);
    if(job_desc) {
      tmps=job_desc->jobname;
      tmps = Arc::escape_chars(tmps, "\"\\", '\\', false);
      o<<"name: \""<<tmps<<"\", ";
      tmps=job_desc->DN;
      tmps = Arc::escape_chars(tmps, "\"\\", '\\', false);
      o<<"owner: \""<<tmps<<"\", ";
      o<<"lrms: "<<job_desc->lrms<<", queue: "<<job_desc->queue;
      if(job_desc->localid.length() >0) o<<", lrmsid: "<<job_desc->localid;
    };
    tmps = job.GetFailure(config);
    if(tmps.length()) {
      for(std::string::size_type i=0;;) {
        i=tmps.find('\n',i);
        if(i==std::string::npos) break;
        tmps[i]='.';
      };
      tmps = Arc::escape_chars(tmps, "\"\\", '\\', false);
      o<<", failure: \""<<tmps<<"\"";
    };
    o<<std::endl;
    o.close();
    return true;
} 

bool JobLog::RunReporter(const GMConfig &config) {
  if(proc != NULL) {
    if(proc->Running()) return true; /* running */
    delete proc;
    proc=NULL;
  };
  if(time(NULL) < (last_run+period)) return true; // default: once per hour
  last_run=time(NULL);
  if (logger_name.empty()) {
    logger.msg(Arc::ERROR,": Logger name is not specified");
    return false;
  }
  // Reporter is tun with only argument - configuration file.
  // It is supposed to parse that configuration file to obtain other parameters.
  std::list<std::string> argv;
  argv.push_back(Arc::ArcLocation::GetToolsDir()+"/"+logger_name);
  argv.push_back("-c");
  argv.push_back(config.ConfigFile());
  proc = new Arc::Run(argv);
  if((!proc) || (!(*proc))) {
    delete proc;
    proc = NULL;
    logger.msg(Arc::ERROR,": Failure creating slot for reporter child process");
    return false;
  };
  std::string errlog;
  JobLog* joblog = config.GetJobLog();
  if(joblog) {
    if(!joblog->logfile.empty()) errlog = joblog->logfile;
  };
  proc->AssignInitializer(&initializer,errlog.empty()?NULL:(void*)errlog.c_str());
  logger.msg(Arc::DEBUG, "Running command %s", argv.front());
  if(!proc->Start()) {
    delete proc;
    proc = NULL;
    logger.msg(Arc::ERROR,": Failure starting reporter child process");
    return false;
  };
  return true;
}

bool JobLog::SetLogger(const char* fname) {
  if(fname) logger_name = (std::string(fname));
  return true;
}

bool JobLog::SetLogFile(const char* fname) {
  if(fname) logfile = (std::string(fname));
  return true;
}

bool JobLog::WriteJobRecord(GMJob &job, const GMConfig& config) {
  // Records are written only for first and last states
  if((job.get_state() != JOB_STATE_ACCEPTED) &&
     (job.get_state() != JOB_STATE_FINISHED)) return true;

  // Only one record per job state change is generated now
  bool result = job_log_make_file(job,config,"",report_config); // not assigning urls to records anymore

  return result;
}

void JobLog::SetCredentials(std::string const &key_path,std::string const &certificate_path,std::string const &ca_certificates_dir)
{
  if (!key_path.empty()) 
    report_config.push_back(std::string("key_path=")+key_path);
  if (!certificate_path.empty())
    report_config.push_back(std::string("certificate_path=")+certificate_path);
  if (!ca_certificates_dir.empty())
    report_config.push_back(std::string("ca_certificates_dir=")+ca_certificates_dir);
}

JobLog::~JobLog(void) {
  if(proc != NULL) {
    if(proc->Running()) proc->Kill(0);
    delete proc;
    proc=NULL;
  };
}

void JobLog::initializer(void* arg) {
  char const * errlog = (char const *)arg;
  int h;
  // set up stdin,stdout and stderr
  h=::open("/dev/null",O_RDONLY);
  if(h != 0) { if(dup2(h,0) != 0) { sleep(10); exit(1); }; close(h); };
  h=::open("/dev/null",O_WRONLY);
  if(h != 1) { if(dup2(h,1) != 1) { sleep(10); exit(1); }; close(h); };
  h=errlog ? ::open(errlog,O_WRONLY | O_CREAT | O_APPEND,S_IRUSR | S_IWUSR) : -1;
  if(h==-1) { h=::open("/dev/null",O_WRONLY); };
  if(h != 2) { if(dup2(h,2) != 2) { sleep(10); exit(1); }; close(h); };
}


} // namespace ARex
