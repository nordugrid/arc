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
#include "../accounting/AAR.h"
#include "../accounting/AccountingDBSQLite.h"
#include "JobLog.h"

#define ACCOUNTING_DB_FILE "accounting.db"

namespace ARex {

static Arc::Logger& logger = Arc::Logger::getRootLogger();

JobLog::JobLog(void):filename(""),reporter_proc(NULL),reporter_last_run(0),reporter_period(3600),
                     archive_mgmt_proc(NULL),archive_mgmt_last_run(0),archive_mgmt_period(3600){
}

void JobLog::SetOutput(const char* fname) {
  filename=fname;
}

bool JobLog::SetReporterPeriod(int new_period) {
    if ( new_period < 3600 ) return false;
    reporter_period=new_period;
    // it does not make sense to run archive manager faster than reporter
    if (archive_mgmt_period < reporter_period ) {
        archive_mgmt_period=reporter_period;
    }
    return true;
}

bool JobLog::SetArchiveManagerPeriod(int new_period) {
    if ( new_period < reporter_period ) return false;
    archive_mgmt_period=new_period;
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
  if(reporter_proc != NULL) {
    if(reporter_proc->Running()) return true; /* running */
    delete reporter_proc;
    reporter_proc=NULL;
  };
  // Check tool exits
  if (reporter_tool.empty()) {
    logger.msg(Arc::ERROR,": Accounting records reporter tool is not specified");
    return false;
  }
  // Record the start time
  if(time(NULL) < (reporter_last_run+reporter_period)) return true; // default: once per hour
  reporter_last_run=time(NULL);
  // Reporter is run with only argument - configuration file.
  // It is supposed to parse that configuration file to obtain other parameters.
  std::list<std::string> argv;
  argv.push_back(Arc::ArcLocation::GetToolsDir()+"/"+reporter_tool);
  argv.push_back("-c");
  argv.push_back(config.ConfigFile());
  reporter_proc = new Arc::Run(argv);
  if((!reporter_proc) || (!(*reporter_proc))) {
    delete reporter_proc;
    reporter_proc = NULL;
    logger.msg(Arc::ERROR,": Failure creating slot for accounting reporter child process");
    return false;
  };
  std::string errlog;
  JobLog* joblog = config.GetJobLog();
  if(joblog) {
    if(!joblog->reporter_logfile.empty()) errlog = joblog->reporter_logfile;
  };
  reporter_proc->AssignInitializer(&initializer,errlog.empty()?NULL:(void*)errlog.c_str());
  logger.msg(Arc::DEBUG, "Running command %s", argv.front());
  if(!reporter_proc->Start()) {
    delete reporter_proc;
    reporter_proc = NULL;
    logger.msg(Arc::ERROR,": Failure starting accounting reporter child process");
    return false;
  };
  return true;
}

bool JobLog::ReporterEnabled(void) {
  if (reporter_tool.empty()) return false;
  return true;
}

bool JobLog::SetReporter(const char* fname) {
  if(fname) reporter_tool = (std::string(fname));
  return true;
}

bool JobLog::SetReporterLogFile(const char* fname) {
  if(fname) reporter_logfile = (std::string(fname));
  return true;
}

bool JobLog::RunArchiveManager(const GMConfig &config) {
  if(reporter_proc != NULL) {
    if(reporter_proc->Running()) return true; /* reporter is running */
  }
  if(archive_mgmt_proc != NULL) {
    if(archive_mgmt_proc->Running()) return true; /* archive manager is running */
    delete archive_mgmt_proc;
    archive_mgmt_proc=NULL;
  };
  // Check tool exists
  if (archive_mgmt_tool.empty()) {
    logger.msg(Arc::ERROR,": Accounting archive management tool is not specified");
    return false;
  }
  // Record the start time
  if(time(NULL) < (archive_mgmt_last_run+archive_mgmt_period)) return true; // default: once per hour
  archive_mgmt_last_run=time(NULL);
  // Pass only runtime config to archive manager
  std::list<std::string> argv;
  argv.push_back(Arc::ArcLocation::GetToolsDir()+"/"+archive_mgmt_tool);
  argv.push_back("-c");
  argv.push_back(config.ConfigFile());
  archive_mgmt_proc = new Arc::Run(argv);
  if((!archive_mgmt_proc) || (!(*archive_mgmt_proc))) {
    delete archive_mgmt_proc;
    archive_mgmt_proc = NULL;
    logger.msg(Arc::ERROR,": Failure creating slot for accounting archive manager child process");
    return false;
  };
  // Logfile is handled by A-REX
  std::string errlog;
  JobLog* joblog = config.GetJobLog();
  if(joblog) {
    if(!joblog->archive_mgmt_logfile.empty()) errlog = joblog->archive_mgmt_logfile;
  };
  archive_mgmt_proc->AssignInitializer(&initializer,errlog.empty()?NULL:(void*)errlog.c_str());
  logger.msg(Arc::DEBUG, "Running command %s", argv.front());
  if(!archive_mgmt_proc->Start()) {
    delete archive_mgmt_proc;
    archive_mgmt_proc = NULL;
    logger.msg(Arc::ERROR,": Failure starting accounting archive manager child process");
    return false;
  };
  return true;
}

bool JobLog::ArchiveManagerEnabled(void) {
  if (archive_mgmt_tool.empty()) return false;
  return true;
}

bool JobLog::SetArchiveManager(const char* fname) {
  if(fname) archive_mgmt_tool = (std::string(fname));
  return true;
}

bool JobLog::SetArchiveManagerLogFile(const char* fname) {
  if(fname) archive_mgmt_logfile = (std::string(fname));
  return true;
}

bool JobLog::WriteJobRecord(GMJob &job, const GMConfig& config) {
  // Legacy joblog files generation
  // TODO: add configuration option
  if((job.get_state() == JOB_STATE_ACCEPTED) ||
     (job.get_state() == JOB_STATE_FINISHED)) {
        // don't care about legacy method success
        job_log_make_file(job,config,"",report_config); // not assigning urls to records anymore
  }

  // Create accounting DB connection
  std::string accounting_db_path = config.ControlDir() + G_DIR_SEPARATOR_S + ACCOUNTING_DB_FILE;
  AccountingDBSQLite adb(accounting_db_path);

  if (!adb.IsValid()) {
    logger.msg(Arc::ERROR,": Failure creating accounting database connection");
    return false;
  }
  
  // create initial AAR record in the accounting database on ACCEPTED
  if(job.get_state() == JOB_STATE_ACCEPTED) {
    AAR aar;
    aar.FetchJobData(job, config);
    return adb.createAAR(aar);
  }

  // update all job metrics when job FINISHED
  if (job.get_state() == JOB_STATE_FINISHED) {
    AAR aar;
    aar.FetchJobData(job, config);
    return adb.updateAAR(aar);
  }

  // record job state change event in the accounting database
  aar_jobevent_t jobevent(job.get_state_name(), Arc::Time());
  return adb.addJobEvent(jobevent, job.get_id());
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
  if(reporter_proc != NULL) {
    if(reporter_proc->Running()) reporter_proc->Kill(0);
    delete reporter_proc;
    reporter_proc=NULL;
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
