#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>

#include <glibmm.h>

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/client/JobDescription.h>
#include <arc/Utils.h>


#include "users.h"
#include "job.h"
#include "../files/info_files.h"
#include "../run/run_function.h"
#include "../conf/environment.h"

#include "job_request.h"

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


static Arc::Logger& logger = Arc::Logger::getRootLogger();

typedef enum {
  job_req_unknown,
  job_req_rsl,
// #ifndef JSDL_MISSING
  job_req_jsdl
// #endif
} job_req_type_t;


static int filter_rtes(const std::string& rte_root,const std::list<std::string>& rte) {
  int rtes = 0;
  if(rte_root.empty()) return rte.size();
  for(std::list<std::string>::const_iterator r = rte.begin();r!=rte.end();++r) {
    std::string path = Glib::build_filename(rte_root,*r);
    if(Glib::file_test(path,Glib::FILE_TEST_IS_REGULAR)) continue;
    ++rtes;
  }
  return rtes;
}

/* function for jobmanager-ng */
bool parse_job_req_for_action(const char* fname,
               std::string &action,std::string &jobid,
               std::string &lrms,std::string &queue) {
  JobLocalDescription job_desc;
  std::string filename(fname);
  if(parse_job_req(filename,job_desc) == JobReqSuccess) {
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
  if(parse_job_req(filename,job_desc) != JobReqSuccess) return false;
  if(job_desc.reruns>user.Reruns()) job_desc.reruns=user.Reruns();
  if((job_desc.diskspace>user.DiskSpace()) || (job_desc.diskspace==0)) {
    job_desc.diskspace=user.DiskSpace();
  };
  // Adjust number of rtes - exclude existing ones
  job_desc.rtes = filter_rtes(runtime_config_dir(),job_desc.rte);
  if(!job_local_write_file(desc,user,job_desc)) return false;
  if(!job_input_write_file(desc,user,job_desc.inputdata)) return false;
  if(!job_output_write_file(desc,user,job_desc.outputdata)) return false;
  if(!job_rte_write_file(desc,user,job_desc.rte)) return false;
  return true;
}

/* parse job request, fill job description (local) */
JobReqResult parse_job_req(const std::string &fname,JobLocalDescription &job_desc,std::string* acl, std::string* failure) {
  Arc::JobDescription arc_job_desc;
  if (!get_arc_job_description(fname, arc_job_desc)) {
    if (failure) *failure = "Unable to read or parse job description.";
    return JobReqInternalFailure;
  }

  if (!arc_job_desc.Resources.RunTimeEnvironment.isResolved()) {
    if (failure)
      *failure = "Runtime environments have not been resolved.";
    return JobReqInternalFailure;
  }

  job_desc = arc_job_desc;

  if (acl) return get_acl(arc_job_desc, *acl);
  return JobReqSuccess;
}

typedef struct {
  const Arc::JobDescription* arc_job_desc;
  const std::string* session_dir;
} set_execs_t;

static int set_execs_callback(void* arg) {
  const Arc::JobDescription& arc_job_desc = *(((set_execs_t*)arg)->arc_job_desc);
  const std::string& session_dir = *(((set_execs_t*)arg)->session_dir);
  if (set_execs(arc_job_desc, session_dir)) return 0;
  return -1;
};

/* parse job description and set specified file permissions to executable */
bool set_execs(const JobDescription &desc,const JobUser &user,const std::string &session_dir) {
  std::string fname = user.ControlDir() + "/job." + desc.get_id() + ".description";
  Arc::JobDescription arc_job_desc;
  if (!get_arc_job_description(fname, arc_job_desc)) return false;

  if (user.StrictSession()) {
    JobUser tmp_user(user.get_uid()==0?desc.get_uid():user.get_uid());
    set_execs_t arg; arg.arc_job_desc=&arc_job_desc; arg.session_dir=&session_dir;
    return (RunFunction::run(tmp_user, "set_execs", &set_execs_callback, &arg, 20) == 0);
  }
  return set_execs(arc_job_desc, session_dir);
}

bool write_grami(const JobDescription &desc,const JobUser &user,const char *opt_add) {
  const std::string fname = user.ControlDir() + "/job." + desc.get_id() + ".description";

  Arc::JobDescription arc_job_desc;
  if (!get_arc_job_description(fname, arc_job_desc)) return false;

  return write_grami(arc_job_desc, desc, user, opt_add);
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
