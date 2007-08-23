//@ #include "../std.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SSTREAM
#include <sstream>
#else
#include <strstream>
#endif
#include <fstream>

#include "jsdl_soapStub.h"
#include "../../files/info_types.h"
#include "../../files/info_files.h"
//@ #include "../../misc/stringtoint.h"
#include "../../misc/canonical_dir.h"
//@ #include "../../misc/inttostring.h"
#include "../../url/url_options.h"
//@ #include "../../misc/log_time.h"

// For some constants
#include "../rsl/parse_rsl.h"

//@ 
#include <src/libs/common/StringConv.h>
#define olog std::cerr
#define odlog(level) std::cerr
#define inttostring Arc::tostring
//@ 

#include "jsdl_job.h"

//#define SOAP_FMAC5 static
//#include "jsdl_soapC.cpp"

#ifdef GSOAP_ALWAYS_USES_NAMESPACES_FOR_MEMBER_NAMES
#define JobDescription jsdl__JobDescription
#define Application jsdl__Application
#define JobIdentification jsdl__JobIdentification
#define JobName jsdl__JobName
#define DataStaging jsdl__DataStaging
#define Target jsdl__Target
#define Source jsdl__Source
#define FileName jsdl__FileName
#define FilesystemName jsdl__FilesystemName
#define URI jsdl__URI
#define Resources jsdl__Resources
#define IndividualCPUCount jsdl__IndividualCPUCount
#define TotalCPUCount jsdl__TotalCPUCount
#define IndividualCPUTime jsdl__IndividualCPUTime
#define TotalCPUTime jsdl__TotalCPUTime
#define IndividualPhysicalMemory jsdl__IndividualPhysicalMemory
#define TotalPhysicalMemory jsdl__TotalPhysicalMemory

#define Executable jsdlPOSIX__Executable
#define Argument jsdlPOSIX__Argument
#define CPUTimeLimit jsdlPOSIX__CPUTimeLimit
#define WallTimeLimit jsdlPOSIX__WallTimeLimit
#define MemoryLimit jsdlPOSIX__MemoryLimit
#define Input jsdlPOSIX__Input
#define Output jsdlPOSIX__Output
#define Error jsdlPOSIX__Error
#endif

static struct Namespace jsdl_namespaces[] = {
    {"SOAP-ENV", "http://schemas.xmlsoap.org/soap/envelope/", "http://www.w3.org/*/soap-envelope", NULL},
    {"SOAP-ENC", "http://schemas.xmlsoap.org/soap/encoding/", "http://www.w3.org/*/soap-encoding", NULL},
    {"xsi", "http://www.w3.org/2001/XMLSchema-instance", "http://www.w3.org/*/XMLSchema-instance", NULL},
    {"xsd", "http://www.w3.org/2001/XMLSchema", "http://www.w3.org/*/XMLSchema", NULL},
    {"jsdl", "http://schemas.ggf.org/jsdl/2005/11/jsdl", NULL, NULL },
    {"jsdlPOSIX", "http://schemas.ggf.org/jsdl/2005/11/jsdl-posix", NULL, NULL },
    {"jsdlARC", "http://www.nordugrid.org/ws/schemas/jsdl-arc", NULL, NULL },
    {NULL, NULL, NULL, NULL}
};

static void add_non_cache(const char *fname,std::list<FileData> &inputdata) {
  for(std::list<FileData>::iterator i=inputdata.begin();i!=inputdata.end();++i){    if(i->has_lfn()) if((*i) == fname) {
      add_url_option(i->lfn,"cache","no",-1);
      add_url_option(i->lfn,"exec","yes",-1);
    };
  };
}

static void strip_spaces(std::string& s) {
  int n;
  for(n=0;s[n];n++) if(!isspace(s[n])) break;
  if(n) s.erase(0,n);
  for(n=s.length()-1;n>=0;n--) if(!isspace(s[n])) break;
  s.resize(n+1);
}

JSDLJob::~JSDLJob(void) {
  if(sp_) {
    soap_destroy(sp_);
    soap_end(sp_);
    soap_done(sp_);
    delete sp_;
  };
  if(job_) delete job_;
}

JSDLJob::JSDLJob(void):job_(NULL),job_posix_(NULL) {
  sp_=new soap;
  if(sp_) {
    soap_init(sp_);
    sp_->namespaces=jsdl_namespaces;
    soap_begin(sp_);
  };
}

JSDLJob::JSDLJob(jsdl__JobDefinition_USCOREType* j):sp_(NULL),job_(j) {
  sp_=new soap;
  if(sp_) {
    soap_init(sp_);
    sp_->namespaces=jsdl_namespaces;
    soap_begin(sp_);
    set_posix();
  };
}

void JSDLJob::set(std::istream& f) {
  sp_->is=&f;
  job_=new jsdl__JobDefinition_USCOREType;
  if(job_) {
    job_->soap_default(sp_);
    if(soap_begin_recv(sp_) == SOAP_OK) {
      if(!job_->soap_get(sp_,"jsdl:JobDefinition",NULL)) {
        delete job_; job_=NULL;
      };
    } else { delete job_; job_=NULL; };
    soap_end_recv(sp_);
  };
}

void JSDLJob::set_posix(void) {
  if(job_ && job_->JobDescription &&
     job_->JobDescription->Application &&
     job_->JobDescription->Application->jsdlPOSIX__POSIXApplication) {
    job_posix_=job_->JobDescription->Application->jsdlPOSIX__POSIXApplication;
  };
  /*
  if(job_ && job_->JobDescription &&
     job_->JobDescription->Application &&
     job_->JobDescription->Application->__any) {
#ifdef HAVE_SSTREAM
    std::stringstream f(job_->JobDescription->Application->__any);
#else
    std::strstream f;
    f<<job_->JobDescription->Application->__any;
#endif
std::cout<<"POSIX Application: "<<std::endl<<job_->JobDescription->Application->__any<<std::endl;
    sp_->is=&f;
    job_posix_=new jsdlPOSIX__POSIXApplication_USCOREType;
    if(job_posix_) {
      job_posix_->soap_default(sp_);
      if(soap_begin_recv(sp_) == SOAP_OK) {
        if(!job_posix_->soap_get(sp_,"jsdl-posix:POSIXApplication",NULL)) {
          delete job_posix_; job_posix_=NULL;
        };
      } else { delete job_posix_; job_posix_=NULL; };
      soap_end_recv(sp_);
    };
  };
  */
}

JSDLJob::JSDLJob(std::istream& f):job_(NULL),job_posix_(NULL) {
  sp_=new soap;
  if(sp_) {
    soap_init(sp_);
    sp_->namespaces=jsdl_namespaces;
    soap_begin(sp_);
    set(f);
    set_posix();
  };
}

JSDLJob::JSDLJob(const char* str):job_(NULL) {
#ifdef HAVE_SSTREAM
  std::stringstream f(str);
#else
  std::strstream f;
  f<<str;
#endif
  sp_=new soap;
  if(sp_) {
    soap_init(sp_);
    sp_->namespaces=jsdl_namespaces;
    soap_begin(sp_);
    set(f);
    set_posix();
  };
}

bool JSDLJob::get_jobname(std::string& jobname) {
  jobname.resize(0);
  if(job_->JobDescription->JobIdentification) {
    if(job_->JobDescription->JobIdentification->JobName) {
      jobname=*(job_->JobDescription->JobIdentification->JobName);
    };
  };
  return true;
}

bool JSDLJob::get_arguments(std::list<std::string>& arguments) {
  arguments.clear();
  if(job_posix_->Executable == NULL) {
    odlog(ERROR)<<"ERROR: job description is missing executable"<<std::endl;
    return false;
  };
  strip_spaces(job_posix_->Executable->__item);
  arguments.push_back(job_posix_->Executable->__item);
  for(std::vector<jsdlPOSIX__Argument_USCOREType*>::iterator i = job_posix_->Argument.begin();i!=job_posix_->Argument.end();++i) {
    if(!(*i)) continue;
    strip_spaces((*i)->__item);
    arguments.push_back((*i)->__item);
  };
  return true;
}

bool JSDLJob::get_execs(std::list<std::string>& execs) {
  std::vector<jsdl__DataStaging_USCOREType*>* ds;
  std::vector<jsdl__DataStaging_USCOREType*>::iterator i_ds;
  execs.clear();
  ds=&(job_->JobDescription->DataStaging);
  for(i_ds=ds->begin();i_ds!=ds->end();++i_ds) {
    if(!(*i_ds)) continue;
    if((*i_ds)->Source != NULL) { /* input file */
      if(((*i_ds)->jsdlARC__IsExecutable) && 
         (*((*i_ds)->jsdlARC__IsExecutable))) {
        execs.push_back((*i_ds)->FileName);
      };
    };
  };
  return true;
}

bool JSDLJob::get_RTEs(std::list<std::string>& rtes) {
  rtes.clear();
  jsdl__Resources_USCOREType* resources = job_->JobDescription->Resources;
  if(resources == NULL) return true;
  std::vector<jsdlARC__RunTimeEnvironment_USCOREType*>& rte = 
                               resources->jsdlARC__RunTimeEnvironment;
  std::vector<jsdlARC__RunTimeEnvironment_USCOREType*>::iterator i_rte =
                               rte.begin();
  for(;i_rte!=rte.end();++i_rte) {
    std::string s = (*i_rte)->Name;
    if((*i_rte)->Version) {
      if((*i_rte)->Version->UpperExclusive) continue; // not supported
      if((*i_rte)->Version->LowerExclusive) continue; // not supported
      if(((*i_rte)->Version->Exclusive) &&
         (!*((*i_rte)->Version->Exclusive))) continue; // not supported
      if((*i_rte)->Version->Exact.size() > 1) continue; // not supported
      if((*i_rte)->Version->Exact.size()) {
        s+="="; s+=(*i_rte)->Version->Exact[0];
      };
    };
    rtes.push_back(s);
  };
  return true;
}

bool JSDLJob::get_middlewares(std::list<std::string>& mws) {
  mws.clear();
  jsdl__Resources_USCOREType* resources = job_->JobDescription->Resources;
  if(resources == NULL) return true;
  std::vector<jsdlARC__Middleware_USCOREType*>& mw =
                               resources->jsdlARC__Middleware;
  std::vector<jsdlARC__Middleware_USCOREType*>::iterator i_mw = mw.begin();
  for(;i_mw!=mw.end();++i_mw) {
    std::string s = (*i_mw)->Name;
    if((*i_mw)->Version) {
      if((*i_mw)->Version->UpperExclusive) continue; // not supported
      if((*i_mw)->Version->LowerExclusive) continue; // not supported
      if(((*i_mw)->Version->Exclusive) &&
         (!*((*i_mw)->Version->Exclusive))) continue; // not supported
      if((*i_mw)->Version->Exact.size() > 1) continue; // not supported
      if((*i_mw)->Version->Exact.size()) {
        s+="="; s+=(*i_mw)->Version->Exact[0];
      };
    };
    mws.push_back(s);
  };
  return true;
}

bool JSDLJob::get_acl(std::string& acl) {
  acl.resize(0);
  jsdlARC__AccessControl_USCOREType* a = job_->JobDescription->jsdlARC__AccessControl;
  if(a == NULL) return true;
  if((!(a->Type)) ||
     (*(a->Type) == jsdlARC__AccessControlType_USCOREType__GACL)) {
    if(a->Content) acl=*(a->Content);
  } else {
    return false;
  };
  return true;
}

bool JSDLJob::get_notification(std::string& s) {
  s.resize(0);
  std::vector<jsdlARC__Notify_USCOREType*>& nts = 
                                      job_->JobDescription->jsdlARC__Notify;
  std::vector<jsdlARC__Notify_USCOREType*>::iterator i_nts = nts.begin();
  for(;i_nts!=nts.end();++i_nts) {
    if((!((*i_nts)->Type)) || 
       (*((*i_nts)->Type) == jsdlARC__NotificationType_USCOREType__Email)) {
      if((*i_nts)->Endpoint) {
        if((*i_nts)->State.size()) {
          std::string s_;
          for(std::vector<enum jsdlARC__GMState_USCOREType>::iterator i =
                     (*i_nts)->State.begin();i!=(*i_nts)->State.end();++i) {
            switch(*i) {
              case jsdlARC__GMState_USCOREType__PREPARING: s_+="b"; break;
              case jsdlARC__GMState_USCOREType__INLRMS: s_+="q"; break;
              case jsdlARC__GMState_USCOREType__FINISHING: s_+="f"; break;
              case jsdlARC__GMState_USCOREType__FINISHED: s_+="e"; break;
              case jsdlARC__GMState_USCOREType__DELETED: s_+="d"; break;
              case jsdlARC__GMState_USCOREType__CANCELING: s_+="c"; break;
            };
          };
          if(s_.length()) {
            s+=s_; s+=*((*i_nts)->Endpoint); s+=" ";
          };
        };
      };
    };
  };
  return true;
}

bool JSDLJob::get_lifetime(int& l) {
  jsdl__Resources_USCOREType* resources = job_->JobDescription->Resources;
  if(resources == NULL) return true;
  if(resources->jsdlARC__SessionLifeTime) {
    l=*(resources->jsdlARC__SessionLifeTime);
  };
  return true;
}

bool JSDLJob::get_fullaccess(bool& b) {
  jsdl__Resources_USCOREType* resources = job_->JobDescription->Resources;
  if(resources == NULL) return true;
  if(resources->jsdlARC__SessionType) {
    switch(*(resources->jsdlARC__SessionType)) {
      case jsdlARC__SessionType_USCOREType__FULL: b=true; break;
      default: b=false; break;
    };
  };
  return true;
}

bool JSDLJob::get_gmlog(std::string& s) {
  s.resize(0);
  jsdlARC__LocalLogging_USCOREType* log = 
                job_->JobDescription->jsdlARC__LocalLogging;
  if(log == NULL) return true;
  s=log->Directory;
  return true;
}

bool JSDLJob::get_loggers(std::list<std::string>& urls) {
  urls.clear();
  std::vector<jsdlARC__RemoteLogging_USCOREType*>& logs =
                             job_->JobDescription->jsdlARC__RemoteLogging;
  std::vector<jsdlARC__RemoteLogging_USCOREType*>::iterator i_logs=logs.begin();
  for(;i_logs!=logs.end();++i_logs) {
    urls.push_back((*i_logs)->URL);
  };
  return true;
}

bool JSDLJob::get_credentialserver(std::string& url) {
  url.resize(0);
  if(job_->JobDescription->jsdlARC__CredentialServer) {
    url=job_->JobDescription->jsdlARC__CredentialServer->URL;
  };
  return true;
}

bool JSDLJob::get_queue(std::string& s) {
  s.resize(0);
  jsdl__Resources_USCOREType* resources = job_->JobDescription->Resources;
  if(resources == NULL) return true;
  if(resources->jsdlARC__CandidateTarget == NULL) return true;
  if(resources->jsdlARC__CandidateTarget->QueueName == NULL) return true;
  s=*(resources->jsdlARC__CandidateTarget->QueueName);
  return true;
}

bool JSDLJob::get_stdin(std::string& s) {
  if(job_posix_->Input)  {
    strip_spaces(job_posix_->Input->__item);
    s =job_posix_->Input->__item;
  } else {
    s.resize(0);
  };
  return true;
}

bool JSDLJob::get_stdout(std::string& s) {
  if(job_posix_->Output)  {
    strip_spaces(job_posix_->Output->__item);
    s =job_posix_->Output->__item;
  } else {
    s.resize(0);
  };
  return true;
}

bool JSDLJob::get_stderr(std::string& s) {
  if(job_posix_->Error)  {
    strip_spaces(job_posix_->Error->__item);
    s =job_posix_->Error->__item;
  } else {
    s.resize(0);
  };
  return true;
}

bool JSDLJob::get_data(std::list<FileData>& inputdata,int& downloads,
                       std::list<FileData>& outputdata,int& uploads) {
  std::vector<jsdl__DataStaging_USCOREType*>* ds;
  std::vector<jsdl__DataStaging_USCOREType*>::iterator i_ds;
  inputdata.clear(); downloads=0;
  outputdata.clear(); uploads=0;
  ds=&(job_->JobDescription->DataStaging);
  for(i_ds=ds->begin();i_ds!=ds->end();++i_ds) {
    if(!(*i_ds)) continue;
    if((*i_ds)->FilesystemName) {
      odlog(ERROR)<<"ERROR: FilesystemName defined in job description - all files must be relative to session directory"<<std::endl;
      return false;
    };
    // CreationFlag - ignore
    // DeleteOnTermination - ignore
    if(((*i_ds)->Source == NULL) && ((*i_ds)->Target == NULL)) {
      // Neither in nor out - must be file generated by job
      FileData fdata((*i_ds)->FileName.c_str(),"");
      outputdata.push_back(fdata);
    } else {
      if((*i_ds)->Source != NULL) {
        if((*i_ds)->Source->URI == NULL) {
          // Source without URL - uploaded by client
          FileData fdata((*i_ds)->FileName.c_str(),"");
          inputdata.push_back(fdata);
        } else {
          FileData fdata((*i_ds)->FileName.c_str(),
                         (*i_ds)->Source->URI->c_str());
          inputdata.push_back(fdata);
          if(fdata.has_lfn()) downloads++;
        };
      };
      if((*i_ds)->Target != NULL) {
        if((*i_ds)->Target->URI == NULL) {
          // Destination without URL - keep after execution
          // TODO: check for DeleteOnTermination
          FileData fdata((*i_ds)->FileName.c_str(),"");
          outputdata.push_back(fdata);
        } else {
          FileData fdata((*i_ds)->FileName.c_str(),
                         (*i_ds)->Target->URI->c_str());
          outputdata.push_back(fdata);
          if(fdata.has_lfn()) uploads++;
        };
      };
    };
  };
  return true;
}


static double get_limit(jsdl__RangeValue_USCOREType* range) {
  if(range == NULL) return 0.0;
  if(range->UpperBoundedRange) return range->UpperBoundedRange->__item;
  if(range->LowerBoundedRange) return range->LowerBoundedRange->__item;
  return 0.0;
}

bool JSDLJob::get_memory(int& memory) {
  memory=0;
  if(job_posix_->MemoryLimit) {
    //stringtoint(job_posix_->MemoryLimit->__item,memory);
    memory=job_posix_->MemoryLimit->__item;
    return true;
  };
  jsdl__Resources_USCOREType* resources = job_->JobDescription->Resources;
  if(resources == NULL) return true;
  if(resources->TotalPhysicalMemory) {
    memory=(int)(get_limit(resources->TotalPhysicalMemory)+0.5);
    return true;
  };
  if(resources->IndividualPhysicalMemory) {
    memory=(int)(get_limit(resources->IndividualPhysicalMemory)+0.5);
    return true;
  };
  return true;
}

bool JSDLJob::get_cputime(int& t) {
  t=0;
  if(job_posix_->CPUTimeLimit) {
    //stringtoint(job_posix_->CPUTimeLimit->__item,t);
    t=job_posix_->CPUTimeLimit->__item;
    return true;
  };
  jsdl__Resources_USCOREType* resources = job_->JobDescription->Resources;
  if(resources == NULL) return true;
  if(resources->TotalCPUTime) {
    t=(int)(get_limit(resources->TotalCPUTime)+0.5);
    return true;
  };
  if(resources->IndividualCPUTime) {
    t=(int)(get_limit(resources->IndividualCPUTime)+0.5);
    return true;
  };
  return true;
}

bool JSDLJob::get_walltime(int& t) {
  t=0;
  if(job_posix_->WallTimeLimit) {
    //stringtoint(job_posix_->WallTimeLimit->__item,t);
    t=job_posix_->WallTimeLimit->__item;
    return true;
  };
  return get_cputime(t);
}

bool JSDLJob::get_count(int& n) {
  jsdl__Resources_USCOREType* resources = job_->JobDescription->Resources;
  n=1;
  if(resources == NULL) return true;
  if(resources->TotalCPUCount) {
    n=(int)(get_limit(resources->TotalCPUCount)+0.5);
    return true;
  };
  if(resources->IndividualCPUCount) {
    n=(int)(get_limit(resources->IndividualCPUCount)+0.5);
    return true;
  };
  return true;
}

bool JSDLJob::get_reruns(int& n) {
  if(job_->JobDescription->jsdlARC__Reruns) {
    n=*(job_->JobDescription->jsdlARC__Reruns);
  };
  return true;
}

bool JSDLJob::check(void) {
  if(!job_) {
    odlog(ERROR)<<"ERROR: job description is missing"<<std::endl;
    return false;
  };
  if(!job_->JobDescription) {
    odlog(ERROR)<<"ERROR: job description is missing"<<std::endl;
    return false;
  };
  if(!job_posix_) {
    odlog(ERROR)<<"ERROR: job description is missing POSIX application"<<std::endl;
    return false;
  };
  return true;
}

bool JSDLJob::parse(JobLocalDescription &job_desc,std::string* acl) {
  std::list<std::string> l;
  std::string s;
  int n;
  char c;
  if(!check()) return false;
  if(!get_jobname(job_desc.jobname)) return false;
  if(!get_data(job_desc.inputdata,job_desc.downloads,job_desc.outputdata,job_desc.uploads)) return false;
  if(!get_arguments(job_desc.arguments)) return false;
  if(!get_stdin(job_desc.stdin_)) return false;
  if(!get_stdout(job_desc.stdout_)) return false;
  if(!get_stderr(job_desc.stderr_)) return false;
  n=-1; if(!get_lifetime(n)) return false; if(n != -1) job_desc.lifetime=inttostring(n);
  if(!get_fullaccess(job_desc.fullaccess)) return false;
  if(acl) if(!get_acl(*acl)) return false;
  if(!get_arguments(l)) return false;
  if(l.size() == 0) return false;
  c=l.begin()->c_str()[0];
  if((c != '/') && (c != '$')) {
    add_non_cache(l.begin()->c_str(),job_desc.inputdata);
  };
  if(!get_execs(l)) return false;
  for(std::list<std::string>::iterator i = l.begin();i!=l.end();++i) {
    add_non_cache(i->c_str(),job_desc.inputdata);
  };
  if(!get_gmlog(job_desc.stdlog)) return false;
  if(!get_loggers(l)) return false; if(l.size())job_desc.jobreport=*(l.begin());
  if(!get_notification(job_desc.notify)) return false;
  if(!get_queue(job_desc.queue)) return false;
  if(!get_credentialserver(job_desc.credentialserver)) return false;

/* 

  bool get_jobname(std::string& jobname);
  bool get_arguments(std::list<std::string>& arguments);
  bool get_middlewares(std::list<std::string>& mws);
  bool get_stdin(std::string& s);
  bool get_stdout(std::string& s);
  bool get_stderr(std::string& s);
  bool get_data(std::list<FileData>& inputdata,int& downloads,
                std::list<FileData>& outputdata,int& uploads);
  bool get_memory(int& memory);
  bool get_cputime(int& t);
  bool get_walltime(int& t);
  bool get_count(int& n);

  bool get_execs(std::list<std::string>& execs);
  bool get_acl(std::string& acl);
  bool get_gmlog(std::string& s);
  bool get_loggers(std::list<std::string>& urls);
  bool get_lifetime(int& l);

 Out of scope of job description
NG_RSL_SOFTWARE_PARAM job_desc.clientsoftware=tmp_param[0];
NG_RSL_HOSTNAME_PARAM if(job_desc.clientname.length()!=0) { * keep predefined value * if(job_desc.clientname.find(';') == std::string::npos) { job_desc.clientname+=";"; job_desc.clientname+=tmp_param[0]; job_desc.clientname=tmp_param[0];

 Not needed for JSDL interface
NG_RSL_JOB_ID_PARAM job_desc.jobid=tmp_param[0];
NG_RSL_ACTION_PARAM job_desc.action=tmp_param[0];

 Unnecessary or unused
NG_RSL_REPLICA_PARAM job_desc.rc=tmp_param[0];
NG_RSL_STARTTIME_PARAM job_desc.processtime=tmp_param[0]; };
NG_RSL_DISKSPACE_PARAM sscanf(tmp_param[0],"%lf",&ds); job_desc.diskspace=(unsigned long long int)(ds*1024*1024*1024); * unit - GB *
NG_RSL_FTPTHREADS_PARAM stringtoint(std::string(tmp_param[0]),job_desc.gsiftpthreads);
NG_RSL_CACHE_PARAM if( strcmp(tmp_param[0],"yes") && strcmp(tmp_param[0],"no") ) job_desc.cache = std::string(tmp_param[0]);
NG_RSL_DRY_RUN_PARAM if(!strcasecmp(tmp_param[0],"yes")) job_desc.dryrun=true;

*/

/*
  std::string jobid;         /* job's unique identificator *
  * attributes stored in job.ID.local *
  std::string queue;         /* queue name  - default *
  std::string localid;       /* job's id in lrms *
  std::list<std::string> arguments;    /* executable + arguments *
  std::string DN;            /* user's distinguished name aka subject name *
  mds_time starttime;        /* job submission time *
  std::string lifetime;      /* time to live for submission directory *
  std::string notify;        /* notification flags used and email address *
  mds_time processtime;      /* time to start job processing (downloading) *
  mds_time exectime;         /* time to start execution *
  std::string clientname;    /* IP+port of user interface + info given by ui *
  std::string clientsoftware; /* Client's version *
  int    reruns;             /* number of allowed reruns left *
  int    downloads;          /* number of downloadable files requested *
  int    uploads;            /* number of uploadable files requested *
  std::string jobname;       /* name of job given by user *
  std::string jobreport;     /* URL of user's/VO's logger *
  mds_time cleanuptime;      /* time to remove job completely *
  mds_time expiretime;       /* when delegation expires *
  std::string stdlog;        /* dirname to which log messages will be
                                put after job finishes *
  std::string sessiondir;    /* job's session directory *
  std::string failedstate;   /* state at which job failed, used for rerun *
  /* attributes stored in other files *
  std::list<FileData> inputdata;  /* input files *
  std::list<FileData> outputdata; /* output files *
  /* attributes taken from RSL *
  std::string action;        /* what to do - must be 'request' *
  std::string rc;            /* url to contact replica collection *
  std::string stdin_;         /* file name for stdin handle *
  std::string stdout_;        /* file name for stdout handle *
  std::string stderr_;        /* file name for stderr handle *
  std::string cache;         /* cache default, yes/no *
  int    gsiftpthreads;      /* number of parallel connections to use
                                during gsiftp down/uploads *
  bool   dryrun;             /* if true, this is test job *
  unsigned long long int diskspace;  /* anount of requested space on disk *
*/
  return true;
}

bool JSDLJob::set_execs(const std::string &session_dir) {
  if(!check()) return false;
  std::list<std::string> arguments;
  if(!get_arguments(arguments)) return false;
  if(arguments.size() == 0) return false;
  /* executable can be external, so first check for initial slash */
  char c = arguments.begin()->c_str()[0];
  if((c != '/') && (c != '$')){
    if(canonical_dir(*(arguments.begin())) != 0) {
      olog<<"Bad name for executable: "<<*(arguments.begin())<<std::endl;
      return false;
    };
    fix_file_permissions(session_dir+"/"+(*(arguments.begin())),true);
  };
  std::list<std::string> execs;
  if(!get_execs(execs)) return false;
  for(std::list<std::string>::iterator i = execs.begin();i!=execs.end();++i) {
    if(canonical_dir(*i) != 0) {
      olog<<"Bad name for executable: "<<*i<<std::endl; return false;
    };
    fix_file_permissions(session_dir+"/"+(*i));
  };
  return true;
}

#ifdef JobDescription
#undef JobDescription
bool JSDLJob::write_grami(const JobDescription &desc,const JobUser &user,const char* opt_add) {
#define JobDescription jsdl__JobDescription
#else
bool JSDLJob::write_grami(const JobDescription &desc,const JobUser &user,const char* opt_add) {
#endif
  if(!check()) return false;
  if(desc.get_local() == NULL) return false;
  std::string session_dir = desc.SessionDir();
  JobLocalDescription& job_desc = *(desc.get_local());
  std::string fgrami = user.ControlDir() + "/job." + desc.get_id() + ".grami";
  std::ofstream f(fgrami.c_str(),std::ios::out | std::ios::trunc);
  if(!f.is_open()) return false;
  if(!fix_file_owner(fgrami,desc,user)) return false;
  f<<"joboption_directory='"<<session_dir<<"'"<<std::endl;
  int n;
  std::list<std::string> arguments;
  if(!get_arguments(arguments)) return false;
  n=0;
  for(std::list<std::string>::iterator i = arguments.begin();
                                         i!=arguments.end();++i) {
    f<<"joboption_arg_"<<n<<"="<<value_for_shell(i->c_str(),true)<<std::endl;
    n++;
  };
  std::string s;
  if(!get_stdin(s)) return false;
  if(s.length() == 0) s=NG_RSL_DEFAULT_STDIN;
  f<<"joboption_stdin="<<value_for_shell(s.c_str(),true)<<std::endl;
  if(!get_stdout(s)) return false;
  if(s.length() == 0) { s=NG_RSL_DEFAULT_STDOUT; }
  else {
    if(canonical_dir(s) != 0) {
      olog<<"Bad name for stdout: "<<s<<std::endl; return false;
    };
    s=session_dir+s;
  };
  f<<"joboption_stdout="<<value_for_shell(s.c_str(),true)<<std::endl;
  if(!get_stderr(s)) return false;
  if(s.length() == 0) { s=NG_RSL_DEFAULT_STDERR; }
  else {
    if(canonical_dir(s) != 0) {
      olog<<"Bad name for stderr: "<<s<<std::endl; return false;
    };
    s=session_dir+s;
  };
  f<<"joboption_stderr="<<value_for_shell(s.c_str(),true)<<std::endl;
  if(!get_walltime(n)) return false;
  if(n<=0) { f<<"joboption_walltime="<<std::endl; }
  else { f<<"joboption_walltime="<<numvalue_for_shell(n)<<std::endl; };
  if(!get_cputime(n)) return false;
  if(n<=0) { f<<"joboption_cputime="<<std::endl; }
  else { f<<"joboption_cputime="<<numvalue_for_shell(n)<<std::endl; };
  if(!get_memory(n)) return false;
  if(n<=0) { f<<"joboption_memory="<<std::endl; }
  else { f<<"joboption_memory="<<numvalue_for_shell(n)<<std::endl; };
  if(!get_count(n)) return false;
  if(n<=0) { f<<"joboption_count=1"<<std::endl; }
  else { f<<"joboption_count="<<numvalue_for_shell(n)<<std::endl; };

  // Pre-parsed
  f<<"joboption_jobname="<<value_for_shell(job_desc.jobname,true)<<std::endl;
  f<<"joboption_gridid="<<value_for_shell(desc.get_id(),true)<<std::endl;
  f<<"joboption_queue="<<value_for_shell(job_desc.queue,true)<<std::endl;
  if(job_desc.exectime.defined()) {
    f<<"joboption_starttime="<<job_desc.exectime<<std::endl;
  } else {
    f<<"joboption_starttime="<<std::endl;
  };
/*
  GLOBUS_RSL_PARAM_GET(rsl_tree,GLOBUS_RSL_PARAM_SEQUENCE,
                           NG_RSL_ENVIRONMENT_PARAM,tmp_param);
  {
    int i;
    for(i=0;tmp_param[i]!=NULL;i++) { 
      f<<"joboption_env_"<<(i/2)<<"='"<<value_for_shell(tmp_param[i],false)<<"=";
      i++; if(tmp_param[i]==NULL) { f<<"'"<<std::endl; break; };
      f<<value_for_shell(tmp_param[i],false)<<"'"<<std::endl;
    };
  };


*/


  std::list<std::string> rtes;
  if(!get_RTEs(rtes)) return false;
  n=0;
  for(std::list<std::string>::iterator i = rtes.begin();i!=rtes.end();++i) {
    std::string tmp_s = *i;
    for(int ii=0;ii<tmp_s.length();++ii) tmp_s[ii]=toupper(tmp_s[ii]);
    if(canonical_dir(tmp_s) != 0) {
      olog<<"Bad name for runtime environemnt: "<<*i<<std::endl; return false;
    };
    f<<"joboption_runtime_"<<n<<"="<<value_for_shell(i->c_str(),true)<<std::endl;
    n++;
  };
  //f<<"joboption_rsl="<<frsl<<std::endl;
  print_to_grami(f);
  if(opt_add) f<<opt_add<<std::endl;
  return true;
}

void JSDLJob::print_to_grami(std::ostream &o) {
}

