#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* write essential information about job started/finished */
#include <fstream>
#include <unistd.h>
#include <arc/ArcLocation.h>
#include <arc/StringConv.h>
#include <arc/DateTime.h>
#include "../files/ControlFileContent.h"
#include "../files/JobLogFile.h"
#include "../conf/GMConfig.h"
#include "../misc/escaped.h"
#include "../run/RunParallel.h"
#include "JobLog.h"

namespace ARex {

JobLog::JobLog(void):filename(""),proc(NULL),last_run(0),ex_period(0) {
}

//JobLog::JobLog(const char* fname):proc(NULL),last_run(0),ex_period(0) {
//  filename=fname;
//}

void JobLog::SetOutput(const char* fname) {
  filename=fname;
}

void JobLog::SetExpiration(time_t period) {
  ex_period=period;
}

bool JobLog::open_stream(std::ofstream &o) {
    o.open(filename.c_str(),std::ofstream::app);
    if(!o.is_open()) return false;
    o<<(Arc::Time().str(Arc::UserTime));
    o<<" ";
    return true;
}

bool JobLog::start_info(GMJob &job,const GMConfig &config) {
  if(filename.length()==0) return true;
    std::ofstream o;
    if(!open_stream(o)) return false;
    o<<"Started - job id: "<<job.get_id()<<", unix user: "<<job.get_user().get_uid()<<":"<<job.get_user().get_gid()<<", ";
    if(job.GetLocalDescription(config)) {
      JobLocalDescription *job_desc = job.get_local();
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

bool JobLog::finish_info(GMJob &job,const GMConfig &config) {
  if(filename.length()==0) return true;
    std::ofstream o;
    if(!open_stream(o)) return false;
    o<<"Finished - job id: "<<job.get_id()<<", unix user: "<<job.get_user().get_uid()<<":"<<job.get_user().get_gid()<<", ";
    std::string tmps;
    if(job.GetLocalDescription(config)) {
      JobLocalDescription *job_desc = job.get_local();
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
  if(time(NULL) < (last_run+3600)) return true; // once per hour
  last_run=time(NULL);
  std::string cmd = Arc::ArcLocation::GetToolsDir()+"/"+logger;
  if(ex_period) cmd += " -E " + Arc::tostring(ex_period);
  if(!vo_filters.empty()) cmd += " -F " + vo_filters;
  cmd += " " + config.ControlDir();
  bool res = RunParallel::run(config,Arc::User(),"logger",cmd,&proc,false,false);
  return res;
}

bool JobLog::SetLogger(const char* fname) {
  if(fname) logger = (std::string(fname));
  return true;
}

bool JobLog::SetVoFilters(const char* filters) {
  if(filters) vo_filters = (std::string(filters));
  return true;
}

bool JobLog::SetReporter(const char* destination) {
  if(destination) urls.push_back(std::string(destination));
  return true;
}

bool JobLog::make_file(GMJob &job, const GMConfig& config) {
  if((job.get_state() != JOB_STATE_ACCEPTED) &&
     (job.get_state() != JOB_STATE_FINISHED)) return true;
  bool result = true;
  // for configured loggers
  for(std::list<std::string>::iterator u = urls.begin();u!=urls.end();u++) {
    if(u->length()) result = job_log_make_file(job,config,*u,report_config) && result;
  };
  // for user's logger
  JobLocalDescription* local;
  if(!job.GetLocalDescription(config)) {
    result=false;
  } else if((local=job.get_local()) == NULL) { 
    result=false;
  } else {
    if(!(local->jobreport.empty())) {
      for (std::list<std::string>::iterator v = local->jobreport.begin();
           v!=local->jobreport.end(); v++) {
        result = job_log_make_file(job,config,*v,report_config) && result;
      }
    };
  };
  return result;
}

void JobLog::set_credentials(std::string &key_path,std::string &certificate_path,std::string &ca_certificates_dir)
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

} // namespace ARex
