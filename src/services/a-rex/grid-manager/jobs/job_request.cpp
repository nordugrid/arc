#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>

#include <arc/URL.h>
#include <arc/StringConv.h>
#include "../jobs/users.h"
#include "../jobs/job.h"
#include "../files/info_files.h"
#ifdef HAVE_GLOBUS_RSL
#include "../jobdesc/rsl/parse_rsl.h"
#endif
#include "../jobdesc/jsdl/jsdl_job.h"
#include "../run/run_function.h"

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

#include "job_request.h"

static bool insert_RC_to_url(std::string& url,const std::string& rc_url) {
  Arc::URL url_(url);
  if(!url_) return false;
  if(url_.Protocol() != "rc") return false;
  if(!url_.Host().empty()) return false;
  Arc::URL rc_url_(rc_url);
  if(!rc_url_) return false;
  if(rc_url_.Protocol() != "ldap") return false;
  url_.ChangePort(rc_url_.Port());
  url_.ChangeHost(rc_url_.Host());
  url_.ChangePath(rc_url_.Path()+"/"+url_.Path());
  url=url_.str();
  return true;
}

typedef enum {
  job_req_unknown,
#ifdef HAVE_GLOBUS_RSL
  job_req_rsl,
#endif
// #ifndef JSDL_MISSING
  job_req_jsdl
// #endif
} job_req_type_t;

static job_req_type_t detect_job_req_type(const char* fname) {
  // RSL starts from &(
#ifdef HAVE_GLOBUS_RSL
  static char rsl_pattern[] = "&(";
#endif
  // JSDL should start from <?xml. But better use just <
  static char jsdl_pattern[] = "<";
  std::ifstream f(fname);
  if(!f.is_open()) return job_req_unknown;
  unsigned int l = 0;
  char buf[6]; 
  while(!f.eof() && !f.fail()) {
    f.getline(buf+l,sizeof(buf)-l);
    for(unsigned int n = l;buf[n];n++) {
      if(isspace(buf[n])) continue;
      buf[l]=buf[n]; l++;
    };
    if(l >= (sizeof(buf)-1)) break;
  };
  buf[l]=0;
#ifdef HAVE_GLOBUS_RSL
  if(strncasecmp(rsl_pattern,buf,sizeof(rsl_pattern)-1) == 0) return job_req_rsl;
#endif
  if(strncasecmp(jsdl_pattern,buf,sizeof(jsdl_pattern)-1) == 0) return job_req_jsdl;
  return job_req_unknown;
}

/* function for jobmanager-ng */
bool parse_job_req_for_action(const char* fname,
               std::string &action,std::string &jobid,
               std::string &lrms,std::string &queue) {
  JobLocalDescription job_desc;
  std::string filename(fname);
  if(parse_job_req(filename,job_desc)) {
    action=job_desc.action;
    jobid=job_desc.jobid;
    lrms=job_desc.lrms;
    queue=job_desc.queue;
    return true;
  };
  return false;
}

/* parse RSL and write few information files */
bool process_job_req(JobUser &user,const JobDescription &desc) {
  JobLocalDescription job_desc;
  return process_job_req(user,desc,job_desc);
}

bool process_job_req(JobUser &user,const JobDescription &desc,JobLocalDescription &job_desc) {
  /* read local first to get some additional info pushed here by script */
  job_local_read_file(desc.get_id(),user,job_desc);
  /* some default values */
  job_desc.lrms=user.DefaultLRMS();
  job_desc.reruns=user.Reruns();
  job_desc.queue=user.DefaultQueue();
  job_desc.lifetime=Arc::tostring(user.KeepFinished());
  std::string filename;
  filename = user.ControlDir() + "/job." + desc.get_id() + ".description";
  if(!parse_job_req(filename,job_desc)) return false;
  if(job_desc.reruns>user.Reruns()) job_desc.reruns=user.Reruns();
  if((job_desc.diskspace>user.DiskSpace()) || (job_desc.diskspace==0)) {
    job_desc.diskspace=user.DiskSpace();
  };
  if(job_desc.rc.length() != 0) {
    for(FileData::iterator i=job_desc.outputdata.begin();
                         i!=job_desc.outputdata.end();++i) {
      insert_RC_to_url(i->lfn,job_desc.rc);
    };
    for(FileData::iterator i=job_desc.inputdata.begin();
                         i!=job_desc.inputdata.end();++i) {
      insert_RC_to_url(i->lfn,job_desc.rc);
    };
  };
  if(job_desc.gsiftpthreads > 1) {
    std::string v = Arc::tostring(job_desc.gsiftpthreads);
    for(FileData::iterator i=job_desc.outputdata.begin();
                         i!=job_desc.outputdata.end();++i) {
      Arc::URL u(i->lfn);
      if(u) { u.AddOption("threads",v); i->lfn=u.fullstr(); };
    };
    for(FileData::iterator i=job_desc.inputdata.begin();
                         i!=job_desc.inputdata.end();++i) {
      Arc::URL u(i->lfn);
      if(u) { u.AddOption("threads",v); i->lfn=u.fullstr(); };
    };
  };
  if(job_desc.cache.length() != 0) {
    std::string value;
    for(FileData::iterator i=job_desc.outputdata.begin();
                         i!=job_desc.outputdata.end();++i) {
      Arc::URL u(i->lfn);
      value=u.Option("cache");
      if(u && value.empty()) {
        u.AddOption("cache",job_desc.cache); i->lfn=u.fullstr();
      };
    };
    for(FileData::iterator i=job_desc.inputdata.begin();
                         i!=job_desc.inputdata.end();++i) {
      Arc::URL u(i->lfn);
      value=u.Option("cache");
      if(value.empty()) {
        u.AddOption("cache",job_desc.cache); i->lfn=u.fullstr();
      };
    };
  };
  if(!job_local_write_file(desc,user,job_desc)) return false;
  if(!job_input_write_file(desc,user,job_desc.inputdata)) return false;
  if(!job_output_write_file(desc,user,job_desc.outputdata)) return false;
  return true;  
}

/* parse job request, fill job description (local) */
bool parse_job_req(const std::string &fname,JobLocalDescription &job_desc,std::string* acl,std::string* failure) {
  switch(detect_job_req_type(fname.c_str())) {
#ifdef HAVE_GLOBUS_RSL
    case job_req_rsl: {
      return parse_rsl(fname,job_desc,acl,failure);
    }; break;
#endif
    case job_req_jsdl: {
      std::ifstream f(fname.c_str());
      if(!f.is_open()) return false;
      JSDLJob j(f);
      if(!j) return false;
      bool res = j.parse(job_desc,acl);
      if((!res) && (failure)) *failure=j.get_failure();
      return res;
    }; break;
    default: break;
  };
  return false;
}

/* do rsl substitution and nordugrid specific stuff */
/* then write file back */
bool preprocess_job_req(const std::string &fname,const std::string& session_dir,const std::string& jobid) {
  switch(detect_job_req_type(fname.c_str())) {
#ifdef HAVE_GLOBUS_RSL
    case job_req_rsl: {
      return preprocess_rsl(fname,session_dir,jobid);
    }; break;
#endif
    case job_req_jsdl: {
      // JSDL does not support substitutions,
      // so here we just parse it for testing
      std::ifstream f(fname.c_str());
      if(!f.is_open()) return false;
      JSDLJob j(f);
      if(!j) return false;
      return true;
    }; break;
    default: break;
  };
  // This is to avoid compiler warnings
  std::string tmp;
  tmp=session_dir;
  tmp=jobid;
  return false;
}

typedef struct {
#ifdef HAVE_GLOBUS_RSL
  globus_rsl_t* rsl_tree;
#endif
  const std::string* session_dir;
} set_execs_t;

typedef struct {
  JSDLJob* j;
  const std::string* session_dir;
} job_set_execs_t;

static int set_execs_callback(void* arg) {
#ifdef HAVE_GLOBUS_RSL
  globus_rsl_t* rsl_tree = ((set_execs_t*)arg)->rsl_tree;
  const std::string& session_dir = *(((set_execs_t*)arg)->session_dir);
  if(set_execs(rsl_tree,session_dir)) return 0;
  return -1;
#endif
  return 0;
}

static int job_set_execs_callback(void* arg) {
  JSDLJob& j = *(((job_set_execs_t*)arg)->j);
  const std::string& session_dir = *(((job_set_execs_t*)arg)->session_dir);
  if(j.set_execs(session_dir)) return 0;
  return -1;
}

/* parse job description and set specified file permissions to executable */
bool set_execs(const JobDescription &desc,const JobUser &user,const std::string &session_dir) {
  std::string fname = user.ControlDir() + "/job." + desc.get_id() + ".description";
  switch(detect_job_req_type(fname.c_str())) {
#ifdef HAVE_GLOBUS_RSL
    case job_req_rsl: {
      globus_rsl_t *rsl_tree = NULL;
      rsl_tree=read_rsl(fname);
      if(rsl_tree == NULL) return false;
      if(user.StrictSession()) {
        JobUser tmp_user(user.get_uid()==0?desc.get_uid():user.get_uid());
        set_execs_t arg; arg.rsl_tree=rsl_tree; arg.session_dir=&session_dir;
        return (RunFunction::run(tmp_user,"set_execs",&set_execs_callback,&arg,20) == 0);
      };
      return set_execs(rsl_tree,session_dir);
    }; break;
#endif
    case job_req_jsdl: {
      std::ifstream f(fname.c_str());
      if(!f.is_open()) return false;
      JSDLJob j(f);
      if(!j) return false;
      if(user.StrictSession()) {
        JobUser tmp_user(user.get_uid()==0?desc.get_uid():user.get_uid());
        job_set_execs_t arg; arg.j=&j; arg.session_dir=&session_dir;
        return (RunFunction::run(tmp_user,"set_execs",&job_set_execs_callback,&arg,20) == 0);
      };
      return j.set_execs(session_dir);
    }; break;
    default: break;
  };
  return false;
}

bool write_grami(const JobDescription &desc,const JobUser &user,const char *opt_add) { 
  std::string fname = user.ControlDir() + "/job." + desc.get_id() + ".description"; 
  switch(detect_job_req_type(fname.c_str())) { 
#ifdef HAVE_GLOBUS_RSL
        case job_req_rsl: {
          return write_grami_rsl(desc,user,opt_add);
        }; break;
#endif
        case job_req_jsdl: {
            std::ifstream f(fname.c_str());
            if(!f.is_open()) return false;
            JSDLJob j(f);
            if(!j) return false;
            return j.write_grami(desc,user,opt_add);
        }; break;
        default: break;
      };
  return false;
}

/* extract joboption_jobid from grami file */
std::string read_grami(const JobId &job_id,const JobUser &user) {
  const char* local_id_param = "joboption_jobid=";
  int l = strlen(local_id_param);
  std::string id = "";
  char buf[256];
  std::string fgrami = user.ControlDir() + "/job." + job_id + ".grami";
  std::ifstream f(fgrami.c_str());
  if(!f.is_open()) return id;
  for(;!f.eof();) {
    istream_readline(f,buf,sizeof(buf));
    if(strncmp(local_id_param,buf,l)) continue;
    if(buf[0]=='\'') {
      l++; int ll = strlen(buf);
      if(buf[ll-1]=='\'') buf[ll-1]=0;
    };
    id=buf+l; break;
  };
  f.close();
  return id;
}

