#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* write essential information about job started/finished */
#include <fstream>
#include <arc/StringConv.h>
#include <arc/DateTime.h>
#include "../files/info_types.h"
#include "../files/info_log.h"
#include "../conf/environment.h"
#include "job_log.h"
#include <unistd.h>

#if defined __GNUC__ && __GNUC__ >= 3

#include <limits>
#define istream_readline(__f,__s,__n) {      \
   __f.get(__s,__n,__f.widen('\n'));         \
   if(__f.fail()) __f.clear();               \
   __f.ignore(std::numeric_limits<std::streamsize>::max(), __f.widen('\n')); \
}

#else

#define istream_readline(__f,__s,__n) {      \
   __f.get(__s,__n,'\n');         \
   if(__f.fail()) __f.clear();               \
   __f.ignore(INT_MAX,'\n'); \
}

#endif

JobLog::JobLog(void):filename(""),proc(NULL),last_run(0),ex_period(0) {
}

JobLog::JobLog(const char* fname):proc(NULL),last_run(0),ex_period(0) {
  filename=fname;
}

void JobLog::SetOutput(const char* fname) {
  filename=fname;
}

void JobLog::SetExpiration(time_t period) {
  ex_period=period;
}

bool JobLog::open_stream(std::ofstream &o) {
    o.open(filename.c_str(),std::ofstream::app);
    if(!o.is_open()) return false;
    o<<" ";
    o<<(Arc::Time().str(Arc::UserTime));
    return true;
}

bool JobLog::start_info(JobDescription &job,const JobUser &user) {
  if(filename.length()==0) return true;
    std::ofstream o;
    if(!open_stream(o)) return false;
    o<<"Started - job id: "<<job.get_id()<<", unix user: "<<job.get_uid()<<":"<<job.get_gid()<<", ";
    if(job.GetLocalDescription(user)) {
      JobLocalDescription *job_desc = job.get_local();
      std::string tmps;
      tmps=job_desc->jobname; make_escaped_string(tmps,'"');
      o<<"name: \""<<tmps<<"\", ";
      tmps=job_desc->DN; make_escaped_string(tmps,'"');
      o<<"owner: \""<<tmps<<"\", ";
      o<<"lrms: "<<job_desc->lrms<<", queue: "<<job_desc->queue;
    };
    o<<std::endl;
    o.close();
    return true;
}

bool JobLog::finish_info(JobDescription &job,const JobUser &user) {
  if(filename.length()==0) return true;
    std::ofstream o;
    if(!open_stream(o)) return false;
    o<<"Finished - job id: "<<job.get_id()<<", unix user: "<<job.get_uid()<<":"<<job.get_gid()<<", ";
    std::string tmps;
    if(job.GetLocalDescription(user)) {
      JobLocalDescription *job_desc = job.get_local();
      tmps=job_desc->jobname; make_escaped_string(tmps,'"');
      o<<"name: \""<<tmps<<"\", ";
      tmps=job_desc->DN; make_escaped_string(tmps,'"');
      o<<"owner: \""<<tmps<<"\", ";
      o<<"lrms: "<<job_desc->lrms<<", queue: "<<job_desc->queue;
      if(job_desc->localid.length() >0) o<<", lrmsid: "<<job_desc->localid;
    };
    tmps = job.GetFailure();
    if(tmps.length()) {
      for(std::string::size_type i=0;;) {
        i=tmps.find('\n',i);
        if(i==std::string::npos) break;
        tmps[i]='.';
      };
      make_escaped_string(tmps,'"');
      o<<", failure: \""<<tmps<<"\"";
    };
    o<<std::endl;
    o.close();
    return true;
} 


bool JobLog::read_info(std::fstream &i,bool &processed,bool &jobstart,struct tm &t,JobId &jobid,JobLocalDescription &job_desc,std::string &failure) {
  processed=false;
  if(!i.is_open()) return false;
  char line[4096];
  std::streampos start_p=i.tellp();
  istream_readline(i,line,sizeof(line));
  std::streampos end_p=i.tellp();
  if((line[0] == 0) || (line[0] == '*')) { processed=true; return true; };
  char* p = line;
  if((*p) == ' ') p++;
  // struct tm t;
  /* read time */
  if(sscanf(p,"%d-%d-%d %d:%d:%d ",
       &t.tm_mday,&t.tm_mon,&t.tm_year,&t.tm_hour,&t.tm_min,&t.tm_sec) != 6) {
    return false;
  };
  t.tm_year-=1900;
  t.tm_mon-=1;
  /* skip time */
  for(;(*p) && ((*p)==' ');p++); if(!(*p)) return false;
  for(;(*p) && ((*p)!=' ');p++); if(!(*p)) return false;
  for(;(*p) && ((*p)==' ');p++); if(!(*p)) return false;
  for(;(*p) && ((*p)!=' ');p++); if(!(*p)) return false;
  for(;(*p) && ((*p)==' ');p++); if(!(*p)) return false;
  // bool jobstart;
  if(strncmp("Finished - ",p,11) == 0) {
    jobstart=false; p+=11;
  } else if(strncmp("Started - ",p,10) == 0) {
    jobstart=true; p+=10;
  } else {
    return false;
  };
  /* read values */
  char* name;
  char* value;
  char* pp;
  for(;;) {
    for(;(*p) && ((*p)==' ');p++); if(!(*p)) break;
    if((pp=strchr(p,':')) == NULL) break;
    name=p; (*pp)=0; pp++;
    for(;(*pp) && ((*pp)==' ');pp++);
    value=pp;
    if((*value) == '"') {
      value++;
      pp=make_unescaped_string(value,'"');
      for(;(*pp) && ((*pp) != ',');pp++);
      if((*pp)) pp++;
    } else {
      for(;(*pp) && ((*pp) != ',');pp++);
      if((*pp)) { (*pp)=0; pp++; };
    };
    p=pp;
    /* use name:value pair */
    if(strcasecmp("job id",name) == 0) {
      jobid=value;
    } else if(strcasecmp("name",name) == 0) {
      job_desc.jobname=value;
    } else if(strcasecmp("unix user",name) == 0) {

    } else if(strcasecmp("owner",name) == 0) {
      job_desc.DN=value;
    } else if(strcasecmp("lrms",name) == 0) {
      job_desc.lrms=value;
    } else if(strcasecmp("queue",name) == 0) {
      job_desc.queue=value;
    } else if(strcasecmp("lrmsid",name) == 0) {
      job_desc.localid=value;
    } else if(strcasecmp("failure",name) == 0) {
      failure=value;
    } else {

    };
  };
  i.seekp(start_p); i<<"*"; i.seekp(end_p);
  return true;
}

#ifndef NO_GLOBUS_CODE

bool JobLog::RunReporter(JobUsers &users) {
  //if(!is_reporting()) return true;
  if(proc != NULL) {
    if(proc->Running()) return true; /* running */
    delete proc;
    proc=NULL;
  };
  if(time(NULL) < (last_run+3600)) return true; // once per hour
  last_run=time(NULL);
  if(users.size() <= 0) return true; // no users to report
  char** args = (char**)malloc(sizeof(char*)*(users.size()+6)); 
  if(args == NULL) return false;
  std::string cmd = nordugrid_libexec_loc+"/logger";
  int argc=0; args[argc++]=(char*)cmd.c_str();
  std::string ex_str = Arc::tostring(ex_period);
  if(ex_period) {
    args[argc++]="-E";
    args[argc++]=(char*)ex_str.c_str();
  };
  for(JobUsers::iterator i = users.begin();i != users.end();++i) {
    args[argc++]=(char*)(i->ControlDir().c_str());
  };
  args[argc]=NULL;
  JobUser user(getuid());
  user.SetControlDir(users.begin()->ControlDir());
  bool res = RunParallel::run(user,"logger",args,&proc,false,false);
  free(args);
  return res;
}

#endif // NO_GLOBUS_CODE

bool JobLog::SetReporter(const char* destination) {
  if(destination) urls.push_back(std::string(destination));
  return true;
}

bool JobLog::make_file(JobDescription &job,JobUser &user) {
  //if(!is_reporting()) return true;
  if((job.get_state() != JOB_STATE_ACCEPTED) &&
     (job.get_state() != JOB_STATE_FINISHED)) return true;
  bool result = true;
  // for configured loggers
  for(std::list<std::string>::iterator u = urls.begin();u!=urls.end();u++) {
    if(u->length()) result = job_log_make_file(job,user,*u) && result;
  };
  // for user's logger
  JobLocalDescription* local;
  if(!job.GetLocalDescription(user)) {
    result=false;
  } else if((local=job.get_local()) == NULL) { 
    result=false;
  } else {
    if(local->jobreport.length()) {
      result = job_log_make_file(job,user,local->jobreport) && result;
    };
  };
  return result;
}

JobLog::~JobLog(void) {
#ifndef NO_GLOBUS_CODE
  if(proc != NULL) {
    if(proc->Running()) proc->Kill(0);
    delete proc;
    proc=NULL;
  };
#endif // NO_GLOBUS_CODE
}

