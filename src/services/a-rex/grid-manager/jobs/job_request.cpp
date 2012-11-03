#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>

#include <glibmm.h>

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/compute/JobDescription.h>
#include <arc/Utils.h>


#include "users.h"
#include "job.h"
#include "../files/info_files.h"
#include "../conf/environment.h"
#include "../../delegation/DelegationStore.h"
#include "../../delegation/DelegationStores.h"

#include "job_request.h"

// TODO: move to using process_job_req as much as possible

static Arc::Logger& logger = Arc::Logger::getRootLogger();

typedef enum {
  job_req_unknown,
  job_req_rsl,
// #ifndef JSDL_MISSING
  job_req_jsdl
// #endif
} job_req_type_t;


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
  job_desc.queue=user.DefaultQueue();
  job_desc.lifetime=Arc::tostring(user.KeepFinished());
  std::string filename;
  filename = user.ControlDir() + "/job." + desc.get_id() + ".description";
  if(parse_job_req(filename,job_desc) != JobReqSuccess) return false;
  if(job_desc.reruns>user.Reruns()) job_desc.reruns=user.Reruns();
  if((job_desc.diskspace>user.DiskSpace()) || (job_desc.diskspace==0)) {
    job_desc.diskspace=user.DiskSpace();
  };
  if(!job_local_write_file(desc,user,job_desc)) return false;
  // Convert delegation ids to credential paths.
  // Add default credentials for file which have no own assigned.
  std::string default_cred = user.ControlDir() + "/job." + desc.get_id() + ".proxy";
  for(std::list<FileData>::iterator f = job_desc.inputdata.begin();
                                   f != job_desc.inputdata.end(); ++f) {
    if(f->has_lfn()) {
      if(f->cred.empty()) {
        f->cred = default_cred;
      } else {
        std::string path;
        ARex::DelegationStores* delegs = user.Env().delegations();
        if(delegs) path = (*delegs)[user.DelegationDir()].FindCred(f->cred,job_desc.DN);
        f->cred = path;
      };
    };
  };
  for(std::list<FileData>::iterator f = job_desc.outputdata.begin();
                                   f != job_desc.outputdata.end(); ++f) {
    if(f->has_lfn()) {
      if(f->cred.empty()) {
        f->cred = default_cred;
      } else {
        std::string path;
        ARex::DelegationStores* delegs = user.Env().delegations();
        if(delegs) path = (*delegs)[user.DelegationDir()].FindCred(f->cred,job_desc.DN);
        f->cred = path;
      };
    };
  };
  if(!job_input_write_file(desc,user,job_desc.inputdata)) return false;
  if(!job_output_write_file(desc,user,job_desc.outputdata,job_output_success)) return false;
  return true;
}

/* parse job request, fill job description (local) */
JobReqResult parse_job_req(const std::string &fname,JobLocalDescription &job_desc,std::string* acl, std::string* failure) {
  Arc::JobDescription arc_job_desc;
  return parse_job_req(fname,job_desc,arc_job_desc,acl,failure);
}

JobReqResult parse_job_req(const std::string &fname,JobLocalDescription &job_desc,Arc::JobDescription& arc_job_desc,std::string* acl, std::string* failure) {
  Arc::JobDescriptionResult arc_job_res = get_arc_job_description(fname, arc_job_desc);
  if (!arc_job_res) {
    if (failure) {
      *failure = arc_job_res.str();
      if(failure->empty()) *failure = "Unable to read or parse job description.";
    }
    return JobReqInternalFailure;
  }

  if (!arc_job_desc.Resources.RunTimeEnvironment.isResolved()) {
    if (failure) {
      *failure = "Runtime environments have not been resolved.";
    }
    return JobReqInternalFailure;
  }

  job_desc = arc_job_desc;

  if (acl) return get_acl(arc_job_desc, *acl);
  return JobReqSuccess;
}

/* parse job description and set specified file permissions to executable */
bool set_execs(const JobDescription &desc,const JobUser &user) {
  std::string fname = user.ControlDir() + "/job." + desc.get_id() + ".description";
  Arc::JobDescription arc_job_desc;
  if (!get_arc_job_description(fname, arc_job_desc)) return false;

  return set_execs(arc_job_desc, desc, user);
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
  std::string fgrami = user.ControlDir() + "/job." + job_id + ".grami";
  std::ifstream f(fgrami.c_str());
  if(!f.is_open()) return id;
  for(;!(f.eof() || f.fail());) {
    std::string buf;
    std::getline(f,buf);
    Arc::trim(buf," \t\r\n");
    if(strncmp(local_id_param,buf.c_str(),l)) continue;
    if(buf[l]=='\'') {
      l++; int ll = buf.length();
      if(buf[ll-1]=='\'') buf.resize(ll-1);
    };
    id=buf.substr(l); break;
  };
  f.close();
  return id;
}
