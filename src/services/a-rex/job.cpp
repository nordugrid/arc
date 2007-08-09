#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#include <src/libs/common/StringConv.h>

#include "grid-manager/conf/environment.h"
#include "grid-manager/conf/conf_pre.h"
#include "grid-manager/jobs/job.h"
#include "grid-manager/jobs/plugins.h"
#include "grid-manager/jobs/job_request.h"
#include "grid-manager/jobs/commfifo.h"
#include "grid-manager/run/run_plugin.h"
#include "grid-manager/files/info_files.h"
 
#include "job.h"

using namespace ARex;

static bool env_initialized = false;

ARexGMConfig::~ARexGMConfig(void) {
  if(user_) delete user_;
}

ARexGMConfig::ARexGMConfig(const std::string& configfile,const std::string& uname,const std::string& grid_name,const std::string& service_endpoint):user_(NULL),readonly_(false),grid_name_(grid_name),service_endpoint_(service_endpoint) {
  if(!env_initialized) {
    //if(!run.is_initialized()) {
    //  // olog<<"Warning: Initialization of signal environment failed"<<std::endl;
    //  return;
    //};
    if(read_env_vars()) {
      env_initialized=true;
    };
  };
  if(!env_initialized) return;
  if(!configfile.empty()) nordugrid_config_loc=configfile;
  // const char* uname = user_s.get_uname();
  //if((bool)job_map) uname=job_map.unix_name();
  user_=new JobUser(uname);
  if(!user_->is_valid()) { delete user_; user_=NULL; return; };
  if(nordugrid_loc.empty() != 0) { delete user_; user_=NULL; return; };
  /* read configuration */
  std::string session_root;
  std::string control_dir;
  std::string default_lrms;
  std::string default_queue;
  ContinuationPlugins* cont_plugins;
  RunPlugin* cred_plugin = new RunPlugin;
  std::string allowsubmit;
  bool strict_session;
  if(!configure_user_dirs(uname,control_dir,session_root,
                          default_lrms,default_queue,queues_,
                          *cont_plugins,*cred_plugin,
                          allowsubmit,strict_session)) {
    // olog<<"Failed processing grid-manager configuration"<<std::endl;
    delete user_; user_=NULL; return;
  };
  if(default_queue.empty() && (queues_.size() == 1)) {
    default_queue=*(queues_.begin());
  };
  user_->SetControlDir(control_dir);
  user_->SetSessionRoot(session_root);
  user_->SetLRMS(default_lrms,default_queue);
  user_->SetStrictSession(strict_session);
  //for(;allowsubmit.length();) {
  //  std::string group = config_next_arg(allowsubmit);
  //  if(group.length()) readonly=true;
  //  if(user_a.check_group(group)) { readonly=false; break; };
  //};
  //if(readonly) olog<<"This user is denied to submit new jobs."<<std::endl;
  /*
          * link to the class for direct file access *
          std::string direct_config = "mount "+session_root+"\n";
          direct_config+="dir / nouser read cd dirlist delete append overwrite";          direct_config+=" create "+
             inttostring(user->get_uid())+":"+inttostring(user->get_gid())+
             " 600:600";
          direct_config+=" mkdir "+
             inttostring(user->get_uid())+":"+inttostring(user->get_gid())+
             " 700:700\n";
          direct_config+="end\n";
#ifdef HAVE_SSTREAM
          std::stringstream fake_cfile(direct_config);
#else
          std::strstream fake_cfile;
          fake_cfile<<direct_config;
#endif
          direct_fs = new DirectFilePlugin(fake_cfile,user_s);
          if((bool)job_map) {
            olog<<"Job submission user: "<<uname<<
                  " ("<<user->get_uid()<<":"<<user->get_gid()<<")"<<std::endl;
  */
}


bool ARexJob::is_allowed(void) {
  allowed_to_see_=false;
  allowed_to_maintain_=false;
  // Temporarily checking only user's grid name against owner
  if(config_.GridName() == job_.DN) {
    allowed_to_see_=true;
    allowed_to_maintain_=true;
  };
  return true;
}

ARexJob::ARexJob(const std::string& id,ARexGMConfig& config):config_(config),id_(id) {
  if(id_.empty()) return;
  if(!config_) { id_.clear(); return; };
  // Reading essential information about job
  if(!job_local_read_file(id_,*config_.User(),job_)) { id_.clear(); return; };
  // Checking if user is allowed to do anything with that job
  if(!is_allowed()) { id_.clear(); return; };
  if(!(allowed_to_see_ || allowed_to_maintain_)) { id_.clear(); return; };
}

ARexJob::ARexJob(Arc::XMLNode jsdl,ARexGMConfig& config):config_(config),id_("") {
std::cerr<<"ARexJob - 0"<<std::endl;
  if(!config_) return;
std::cerr<<"ARexJob - 1"<<std::endl;
  // New job is created here
  // First get and acquire new id
  if(!make_job_id()) return;
std::cerr<<"ARexJob - 2"<<std::endl;
  // Turn JSDL into text
  std::string job_desc_str;
  jsdl.GetXML(job_desc_str);
  // Store description
  std::string fname = config_.User()->ControlDir() + "/job." + id_ + ".description";
  if(!job_description_write_file(fname,job_desc_str)) {
std::cerr<<"ARexJob - 3"<<std::endl;
    //@ error_description="Failed to store job RSL description.";
    delete_job_id();
    return;
  };
std::cerr<<"ARexJob - 4"<<std::endl;
  // Analyze JSDL (checking, substituting, etc)
  std::string acl("");
  if(!parse_job_req(fname.c_str(),job_,&acl)) {
std::cerr<<"ARexJob - 5"<<std::endl;
    //@ error_description="Failed to parse job/action description.";
    //@ olog<<error_description<<std::endl;
    delete_job_id();
    return;
  };
std::cerr<<"ARexJob - 6"<<std::endl;
  // Check for proper LRMS name in request. If there is no LRMS name
  // in user configuration that means service is opaque frontend and
  // accepts any LRMS in request.
  if((!job_.lrms.empty()) && (!config_.User()->DefaultLRMS().empty())) {
std::cerr<<"ARexJob - 7"<<std::endl;
    if(job_.lrms != config_.User()->DefaultLRMS()) {
std::cerr<<"ARexJob - 8"<<std::endl;
      //@ error_description="Request for LRMS "+job_desc.lrms+" is not allowed.";
      //@ olog<<error_description<<std::endl;
      delete_job_id();
      return;
    };
  };
std::cerr<<"ARexJob - 9"<<std::endl;
  if(job_.lrms.empty()) job_.lrms=config_.User()->DefaultLRMS();
  // Check for proper queue in request.
  if(job_.queue.empty()) job_.queue=config_.User()->DefaultQueue();
  if(job_.queue.empty()) {
std::cerr<<"ARexJob - 9.5"<<std::endl;
    //@ error_description="Request has no queue defined.";
    //@ olog<<error_description<<std::endl;
    delete_job_id();
    return;
  };
  if(config_.Queues().size() > 0) { // If no queues configured - service takes any
std::cerr<<"ARexJob - 10"<<std::endl;
    for(std::list<std::string>::const_iterator q = config_.Queues().begin();;++q) {
      if(q == config_.Queues().end()) {
std::cerr<<"ARexJob - 11"<<std::endl;
        //@ error_description="Requested queue "+job_desc.queue+" does not match any of available queues.";
        //@ olog<<error_description<<std::endl;
        delete_job_id();
        return;
      };
      if(*q == job_.queue) break;
    };
  };
  if(!preprocess_job_req(fname,config_.User()->SessionRoot()+"/"+id_,id_)) {
std::cerr<<"ARexJob - 12"<<std::endl;
    //@ error_description="Failed to preprocess job description.";
    //@ olog<<error_description<<std::endl;
    delete_job_id();
    return;
  };
  // Start local file 
  /* !!!!! some parameters are unchecked here - rerun,diskspace !!!!! */
  job_.jobid=id_;
  job_.starttime=time(NULL);
  job_.DN=config_.GridName();
  /*@
  if(port != 0) {
    job_desc.clientname=
       inttostring(host[0])+"."+inttostring(host[1])+"."+
       inttostring(host[2])+"."+inttostring(host[3])+":"+
       inttostring(port);
  };
  */
  /*@
  // Try to create proxy
  if(proxy_fname.length() != 0) {
std::cerr<<"ARexJob - 13"<<std::endl;
    std::string fname=user->ControlDir()+"/job."+job_id+".proxy";
    int h=::open(fname.c_str(),O_WRONLY | O_CREAT | O_EXCL,0600);
    if(h == -1) {
std::cerr<<"ARexJob - 14"<<std::endl;
      error_description="Failed to store credentials.";
      return 1;
    };
    int hh=::open(proxy_fname.c_str(),O_RDONLY);
    if(hh == -1) {
std::cerr<<"ARexJob - 15"<<std::endl;
      ::close(h);
      ::remove(fname.c_str());
      error_description="Failed to read credentials.";
      return 1;
    };
    fix_file_owner(fname,*user);
    int l,ll;
    const char* s;
    char buf[256];
    for(;;) {
      ll=::read(hh,buf,sizeof(buf));
      if((ll==0) || (ll==-1)) break;
      for(l=0,s=buf;(ll>0) && (l!=-1);s+=l,ll-=l) l=::write(h,s,ll);
      if(l==-1) break;
    };
    ::close(h);
    ::close(hh);
    try {
      Certificate ci(PROXY,fname);
      job_desc.expiretime = ci.Expires().GetTime();
    } catch (std::exception) {
      job_desc.expiretime = time(NULL);
    };
  };
  */
  // Write local file
  JobDescription job(id_,"",JOB_STATE_ACCEPTED);
  if(!job_local_write_file(job,*config_.User(),job_)) {
std::cerr<<"ARexJob - 16"<<std::endl;
    //@ olog<<"Failed writing local description"<<std::endl;
    delete_job_id();
    //@ error_description="Failed to create job description.";
    return;
  };
  // Write ACL file
  if(!acl.empty()) {
std::cerr<<"ARexJob - 17"<<std::endl;
    if(!job_acl_write_file(id_,*config.User(),acl)) {
std::cerr<<"ARexJob - 18"<<std::endl;
      //@ olog<<"Failed writing ACL"<<std::endl;
      delete_job_id();
      //@ error_description="Failed to process/store job ACL.";
      return;
    };
  };
/*
  // Call authentication/authorization plugin/exec
  int result;
  std::string response;
  // talk to external plugin to ask if we can proceed
  ContinuationPlugins::action_t act =
                 cont_plugins->run(job,*user,result,response);
  // analyze result
  if(act == ContinuationPlugins::act_fail) {
    olog<<"Failed to run external plugin: "<<response<<std::endl;
    delete_job_id();
    error_description="Job is not allowed by external plugin: "+response;
    return 1;
  } else if(act == ContinuationPlugins::act_log) {
    // Scream but go ahead
    olog<<"Failed to run external plugin: "<<response<<std::endl;
  } else if(act == ContinuationPlugins::act_pass) {
    // Just continue
    if(response.length()) olog<<"Plugin response: "<<response<<std::endl;
  } else {
    olog<<"Failed to run external plugin"<<std::endl;
    delete_job_id();
    error_description="Failed to pass external plugin.";
    return 1;
  };
*/ 
/*@
  // Make access to filesystem on behalf of local user
  if(cred_plugin && (*cred_plugin)) {
    job_subst_t subst_arg;
    subst_arg.user=user;
    subst_arg.job=&job_id;
    subst_arg.reason="new";
    // run external plugin to acquire non-unix local credentials
    if(!cred_plugin->run(job_subst,&subst_arg)) {
      olog << "Failed to run plugin" << std::endl;
      delete_job_id();
      error_description="Failed to obtain external credentials.";
      return 1;
    };
    if(cred_plugin->result() != 0) {
      olog << "Plugin failed: " << cred_plugin->result() << std::endl;
      delete_job_id();
      error_description="Failed to obtain external credentials.";
      return 1;
    };
  };
*/
  // Create session directory
  std::string dir=config_.User()->SessionRoot()+"/"+id_;
/*@
  if((getuid()==0) && (user) && (user->StrictSession())) {
    SET_USER_UID;
  };
*/
  if(mkdir(dir.c_str(),0700) != 0) {
std::cerr<<"ARexJob - 19"<<std::endl;
/*@
    if((getuid()==0) && (user) && (user->StrictSession())) {
      RESET_USER_UID;
    };
*/
    //@ olog<<"Failed to create session directory"<<std::endl;
    delete_job_id();
    //@ error_description="Failed to create session directory.";
    return;
  };
/*@
  if((getuid()==0) && (user) && (user->StrictSession())) {
    RESET_USER_UID;
  };
*/
  fix_file_owner(dir,*config_.User());
  // Create status file (do it last so GM picks job up here)
  if(!job_state_write_file(job,*config_.User(),JOB_STATE_ACCEPTED)) {
std::cerr<<"ARexJob - 20"<<std::endl;
    //@ olog<<"Failed writing status"<<std::endl;
    delete_job_id();
    //@ error_description="Failed registering job in grid-manager.";
    return;
  };
  SignalFIFO(*config_.User());
std::cerr<<"ARexJob - 100"<<std::endl;
  return;
}

void ARexJob::GetDescription(Arc::XMLNode& jsdl) {
}

bool ARexJob::Cancel(void) {
  if(id_.empty()) return false;
  return false;
}

bool ARexJob::Resume(void) {
  if(id_.empty()) return false;
  return false;
}

std::string ARexJob::State(void) {
  if(id_.empty()) return "";
  return "";
}
















/*
bool ARexJob::make_job_id(const std::string &id) {
  if((id.find('/') != std::string::npos) || (id.find('\n') != std::string::npos)) {
    olog<<"ID contains forbidden characters"<<std::endl;
    return false;
  };
  if((id == "new") || (id == "info")) return false;
  // claim id by creating empty description file
  std::string fname=user->ControlDir()+"/job."+id+".description";
  struct stat st;
  if(stat(fname.c_str(),&st) == 0) return false;
  int h = ::open(fname.c_str(),O_RDWR | O_CREAT | O_EXCL,S_IRWXU);
  // So far assume control directory is on local fs.
  // TODO: add locks or links for NFS
  if(h == -1) return false;
  fix_file_owner(fname,*user);
  close(h);
  delete_job_id();
  job_id=id;
  return true;
}
*/

bool ARexJob::make_job_id(void) {
  if(!config_) return false;
  int i;
  //@ delete_job_id();
  for(i=0;i<100;i++) {
    id_=Arc::tostring((unsigned int)getpid())+
        Arc::tostring((unsigned int)time(NULL))+
        Arc::tostring(rand(),1);
    std::string fname=config_.User()->ControlDir()+"/job."+id_+".description";
    struct stat st;
    if(stat(fname.c_str(),&st) == 0) continue;
    int h = ::open(fname.c_str(),O_RDWR | O_CREAT | O_EXCL,0600);
    // So far assume control directory is on local fs.
    // TODO: add locks or links for NFS
    if(h == -1) {
      if(errno == EEXIST) continue;
      //@ olog << "Failed to create file in " << user->ControlDir()<< std::endl;
      return false;
    };
    fix_file_owner(fname,*config_.User());
    close(h);
    return true;
  };
  //@ olog << "Out of tries while allocating new job id in " << user->ControlDir() << std:endl;
  id_="";
  return false;
}

bool ARexJob::delete_job_id(void) {
  if(!config_) return true;
  if(!id_.empty()) {
    job_clean_final(JobDescription(id_,
                config_.User()->SessionRoot()+"/"+id_),*config_.User());
    id_="";
  };
  return true;
}

