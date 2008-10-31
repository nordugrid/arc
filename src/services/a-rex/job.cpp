#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
// NOTE: On Solaris errno is not working properly if cerrno is included first
#include <cerrno>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <glibmm/thread.h>
#include <arc/StringConv.h>
#include <arc/security/ArcPDP/Evaluator.h>
#include <arc/security/ArcPDP/EvaluatorLoader.h>
#include <arc/message/SecAttr.h>
#include <arc/credential/Credential.h>

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

#define JOB_POLICY_OPERATION_URN "http://www.nordugrid.org/schemas/policy-arc/types/a-rex/joboperation"

static bool env_initialized = false;
Glib::StaticMutex env_lock = GLIBMM_STATIC_MUTEX_INIT;

bool ARexGMConfig::InitEnvironment(const std::string& configfile) {
  if(env_initialized) return true;
  env_lock.lock();
  if(!env_initialized) {
    if(!configfile.empty()) nordugrid_config_loc=configfile;
    env_initialized=read_env_vars();
  };
  env_lock.unlock();
  return env_initialized;
}

ARexGMConfig::~ARexGMConfig(void) {
  if(user_) delete user_;
}

ARexGMConfig::ARexGMConfig(const std::string& configfile,const std::string& uname,const std::string& grid_name,const std::string& service_endpoint):user_(NULL),readonly_(false),grid_name_(grid_name),service_endpoint_(service_endpoint) {
  if(!InitEnvironment(configfile)) return;
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
  ContinuationPlugins* cont_plugins = NULL;
  RunPlugin* cred_plugin = new RunPlugin;
  std::string allowsubmit;
  bool strict_session;
  if(!configure_user_dirs(uname,control_dir,session_root,
                          default_lrms,default_queue,queues_,
                          *cont_plugins,*cred_plugin,
                          allowsubmit,strict_session)) {
    // olog<<"Failed processing grid-manager configuration"<<std::endl;
    delete user_; user_=NULL; delete cred_plugin; return;
  };
  delete cred_plugin;
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

template <typename T> class AutoPointer {
 private:
  T* object;
  void operator=(const AutoPointer<T>&) { };
  void operator=(T*) { };
  AutoPointer(const AutoPointer&):object(NULL) { };
 public:
  AutoPointer(void):object(NULL) { };
  AutoPointer(T* o):object(o) { }
  ~AutoPointer(void) { if(object) delete object; };
  T& operator*(void) const { return *object; };
  T* operator->(void) const { return object; };
  operator bool(void) const { return (object!=NULL); };
  bool operator!(void) const { return (object==NULL); };
  operator T*(void) const { return object; };
};

static ARexJobFailure setfail(JobReqResult res) {
  switch(res) {
    case JobReqSuccess: return ARexJobNoError;
    case JobReqInternalFailure: return ARexJobInternalError;
    case JobReqSyntaxFailure: return ARexJobDescriptionSyntaxError;
    case JobReqUnsupportedFailure: return ARexJobDescriptionUnsupportedError;
    case JobReqMissingFailure: return ARexJobDescriptionMissingError;
    case JobReqLogicalFailure: return ARexJobDescriptionLogicalError;
  };
  return ARexJobInternalError;
}

bool ARexJob::is_allowed(bool fast) {
  allowed_to_see_=false;
  allowed_to_maintain_=false;
  // Checking user's grid name against owner
  if(config_.GridName() == job_.DN) {
    allowed_to_see_=true;
    allowed_to_maintain_=true;
    return true;
  };
  if(fast) return true;
  // Do fine-grained authorization requested by job's owner
  if(config_.beginAuth() == config_.endAuth()) return true;
  std::string acl;
  if(!job_acl_read_file(id_,*config_.User(),acl)) return true; // safe to ignore
  if(acl.empty()) return true; // No policy defiled - only owner allowed
  // Identify and parse policy
  ArcSec::EvaluatorLoader eval_loader;
  AutoPointer<ArcSec::Policy> policy(eval_loader.getPolicy(ArcSec::Source(acl)));
  if(!policy) {
    logger_.msg(Arc::DEBUG, "Failed to parse user policy for job %s", id_);
    return true;
  };
  AutoPointer<ArcSec::Evaluator> eval(eval_loader.getEvaluator(policy));
  if(!eval) {
    logger_.msg(Arc::DEBUG, "Failed to load evaluator for user policy for job %s", id_);
    return true;
  };
  std::string policyname = policy->getName();
  if((policyname.length() > 7) && 
     (policyname.substr(policyname.length()-7) == ".policy")) {
    policyname.resize(policyname.length()-7);
  };
  if(policyname == "arc") {
    // Creating request - directly with XML
    // Creating top of request document
    Arc::NS ns;
    ns["ra"]="http://www.nordugrid.org/schemas/request-arc";
    Arc::XMLNode request(ns,"ra:Request");
    // Collect all security attributes
    for(std::list<Arc::MessageAuth*>::iterator a = config_.beginAuth();a!=config_.endAuth();++a) {
      if(*a) (*a)->Export(Arc::SecAttr::ARCAuth,request);
    };
    // Leave only client identities
    for(Arc::XMLNode item = request["RequestItem"];(bool)item;++item) {
      for(Arc::XMLNode a = item["Action"];(bool)a;a=item["Action"]) a.Destroy();
      for(Arc::XMLNode r = item["Resource"];(bool)r;r=item["Resource"]) r.Destroy();
    };
    // Fix namespace
    request.Namespaces(ns);
    // Create A-Rex specific action
    // TODO: make helper classes for such operations
    Arc::XMLNode item = request["ra:RequestItem"];
    if(!item) item=request.NewChild("ra:RequestItem");
    // Possible operations are Modify and Read
    Arc::XMLNode action;
    action=item.NewChild("ra:Action");
    action="Read"; action.NewAttribute("Type")="string";
    action.NewAttribute("AttributeId")=JOB_POLICY_OPERATION_URN;
    action=item.NewChild("ra:Action");
    action="Modify"; action.NewAttribute("Type")="string";
    action.NewAttribute("AttributeId")=JOB_POLICY_OPERATION_URN;
    // Evaluating policy
    ArcSec::Response *resp = eval->evaluate(request,policy);
    // Analyzing response in order to understand which operations are allowed
    if(!resp) return true; // Not authorized
    // Following should be somehow made easier
    ArcSec::ResponseList& rlist = resp->getResponseItems();
    for(int n = 0; n<rlist.size(); ++n) {
      ArcSec::ResponseItem* ritem = rlist[n];
      if(!ritem) continue;
      if(ritem->res != ArcSec::DECISION_PERMIT) continue;
      if(!(ritem->reqtp)) continue;
      for(ArcSec::Action::iterator a = ritem->reqtp->act.begin();a!=ritem->reqtp->act.end();++a) {
        ArcSec::RequestAttribute* attr = *a;
        if(!attr) continue;
        ArcSec::AttributeValue* value = attr->getAttributeValue();
        if(!value) continue;
        std::string action = value->encode();
        if(action == "Read") allowed_to_see_=true;
        if(action == "Modify") allowed_to_maintain_=true;
      };
    };
  } else if(policyname == "gacl") {
    // Creating request - directly with XML
    Arc::NS ns;
    Arc::XMLNode request(ns,"gacl");
    // Collect all security attributes
    for(std::list<Arc::MessageAuth*>::iterator a = config_.beginAuth();a!=config_.endAuth();++a) {
      if(*a) (*a)->Export(Arc::SecAttr::GACL,request);
    };
    // Leave only client identities
    int entries = 0;
    for(Arc::XMLNode entry = request["entry"];(bool)entry;++entry) {
      for(Arc::XMLNode a = entry["allow"];(bool)a;a=entry["allow"]) a.Destroy();
      for(Arc::XMLNode a = entry["deny"];(bool)a;a=entry["deny"]) a.Destroy();
      ++entries;
    };
    if(!entries) request.NewChild("entry");
    // Evaluate every action separately
    for(Arc::XMLNode entry = request["entry"];(bool)entry;++entry) {
      entry.NewChild("allow").NewChild("read");
    };
    ArcSec::Response *resp;
    resp=eval->evaluate(request,policy);
    if(resp) {
      ArcSec::ResponseList& rlist = resp->getResponseItems();
      for(int n = 0; n<rlist.size(); ++n) {
        ArcSec::ResponseItem* ritem = rlist[n];
        if(!ritem) continue;
        if(ritem->res != ArcSec::DECISION_PERMIT) continue;
        allowed_to_see_=true; break;
      };
    };
    for(Arc::XMLNode entry = request["entry"];(bool)entry;++entry) {
      entry["allow"].Destroy();
      entry.NewChild("allow").NewChild("write");
    };
    resp=eval->evaluate(request,policy);
    if(resp) {
      ArcSec::ResponseList& rlist = resp->getResponseItems();
      for(int n = 0; n<rlist.size(); ++n) {
        ArcSec::ResponseItem* ritem = rlist[n];
        if(!ritem) continue;
        if(ritem->res != ArcSec::DECISION_PERMIT) continue;
        allowed_to_maintain_=true; break;
      };
    };
    // TODO: <list/>, <admin/>
  } else {
    logger_.msg(Arc::DEBUG, "Unknown user policy '%s' for job %s", policyname, id_);
  };
  return true;
}

ARexJob::ARexJob(const std::string& id,ARexGMConfig& config,Arc::Logger& logger,bool fast_auth_check):id_(id),config_(config),logger_(logger) {
  if(id_.empty()) return;
  if(!config_) { id_.clear(); return; };
  // Reading essential information about job
  if(!job_local_read_file(id_,*config_.User(),job_)) { id_.clear(); return; };
  // Checking if user is allowed to do anything with that job
  if(!is_allowed(fast_auth_check)) { id_.clear(); return; };
  if(!(allowed_to_see_ || allowed_to_maintain_)) { id_.clear(); return; };
}

ARexJob::ARexJob(Arc::XMLNode jsdl,ARexGMConfig& config,const std::string& credentials,const std::string& clientid,Arc::Logger& logger):id_(""),config_(config),logger_(logger) {
  if(!config_) return;
  // New job is created here
  // First get and acquire new id
  if(!make_job_id()) return;
  // Turn JSDL into text
  std::string job_desc_str;
  // Make full XML doc out of subtree
  {
    Arc::XMLNode jsdldoc;
    jsdl.New(jsdldoc);
    jsdldoc.GetDoc(job_desc_str);
  };
  // Store description
  std::string fname = config_.User()->ControlDir() + "/job." + id_ + ".description";
  if(!job_description_write_file(fname,job_desc_str)) {
    delete_job_id();
    failure_="Failed to store job RSL description.";
    failure_type_=ARexJobInternalError;
    return;
  };
  // Analyze JSDL (checking, substituting, etc)
  std::string acl("");
  if((failure_type_=setfail(parse_job_req(fname.c_str(),job_,&acl,&failure_))) != ARexJobNoError) {
    if(failure_.empty()) {
      failure_="Failed to parse job/action description.";
      failure_type_=ARexJobInternalError;
    };
    delete_job_id();
    return;
  };
  // Check for proper LRMS name in request. If there is no LRMS name
  // in user configuration that means service is opaque frontend and
  // accepts any LRMS in request.
  if((!job_.lrms.empty()) && (!config_.User()->DefaultLRMS().empty())) {
    if(job_.lrms != config_.User()->DefaultLRMS()) {
      failure_="Requested LRMS is not supported by this service";
      failure_type_=ARexJobInternalError;
      //failure_type_=ARexJobDescriptionLogicalError;
      delete_job_id();
      return;
    };
  };
  if(job_.lrms.empty()) job_.lrms=config_.User()->DefaultLRMS();
  // Check for proper queue in request.
  if(job_.queue.empty()) job_.queue=config_.User()->DefaultQueue();
  if(job_.queue.empty()) {
    failure_="Request has no queue defined.";
    failure_type_=ARexJobDescriptionMissingError;
    delete_job_id();
    return;
  };
  if(config_.Queues().size() > 0) { // If no queues configured - service takes any
    for(std::list<std::string>::const_iterator q = config_.Queues().begin();;++q) {
      if(q == config_.Queues().end()) {
        failure_="Requested queue "+job_.queue+" does not match any of available queues.";
        //failure_type_=ARexJobDescriptionLogicalError;
        failure_type_=ARexJobInternalError;
        delete_job_id();
        return;
      };
      if(*q == job_.queue) break;
    };
  };
  if(!preprocess_job_req(fname,config_.User()->SessionRoot()+"/"+id_,id_)) {
    failure_="Failed to preprocess job description.";
    failure_type_=ARexJobInternalError;
    delete_job_id();
    return;
  };
  // Start local file 
  /* !!!!! some parameters are unchecked here - rerun,diskspace !!!!! */
  job_.jobid=id_;
  job_.starttime=time(NULL);
  job_.DN=config_.GridName();
  job_.clientname=clientid;
  // Try to create proxy
  if(!credentials.empty()) {
    std::string fname=config.User()->ControlDir()+"/job."+id_+".proxy";
    int h=::open(fname.c_str(),O_WRONLY | O_CREAT | O_EXCL,0600);
    if(h == -1) {
      failure_="Failed to store credentials.";
      failure_type_=ARexJobInternalError;
      delete_job_id();
      return;
    };
    fix_file_owner(fname,*config.User());
    const char* s = credentials.c_str();
    int ll = credentials.length();
    int l = 0;
    for(;(ll>0) && (l!=-1);s+=l,ll-=l) l=::write(h,s,ll);
    if(l==-1) {
      ::close(h);
      failure_="Failed to store credentials.";
      failure_type_=ARexJobInternalError;
      delete_job_id();
      return;
    };
    ::close(h);
    //@try {
    //////Credential(const std::string& cert, const std::string& key, const std::string& cadir, const std::string& cafile);
    //@   Certificate ci(PROXY,fname);
    //@   job_desc.expiretime = ci.Expires().GetTime();
    //@ } catch (std::exception) {
    //@   job_desc.expiretime = time(NULL);
    //@ };
  };
  // Write local file
  JobDescription job(id_,"",JOB_STATE_ACCEPTED);
  if(!job_local_write_file(job,*config_.User(),job_)) {
    delete_job_id();
    failure_="Failed to create job description.";
    failure_type_=ARexJobInternalError;
    return;
  };
  // Write ACL file
  if(!acl.empty()) {
    if(!job_acl_write_file(id_,*config.User(),acl)) {
      delete_job_id();
      failure_="Failed to process/store job ACL.";
      failure_type_=ARexJobInternalError;
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
    failure_type_=ARexJobInternalError;
    return 1;
  } else if(act == ContinuationPlugins::act_log) {
    // Scream but go ahead
    olog<<"Failed to run external plugin: "<<response<<std::endl;
    failure_type_=ARexJobInternalError;
  } else if(act == ContinuationPlugins::act_pass) {
    // Just continue
    if(response.length()) olog<<"Plugin response: "<<response<<std::endl;
  } else {
    olog<<"Failed to run external plugin"<<std::endl;
    delete_job_id();
    error_description="Failed to pass external plugin.";
    failure_type_=ARexJobInternalError;
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
      failure_type_=ARexJobInternalError;
      error_description="Failed to obtain external credentials.";
      return 1;
    };
    if(cred_plugin->result() != 0) {
      olog << "Plugin failed: " << cred_plugin->result() << std::endl;
      delete_job_id();
      error_description="Failed to obtain external credentials.";
      failure_type_=ARexJobInternalError;
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
/*@
    if((getuid()==0) && (user) && (user->StrictSession())) {
      RESET_USER_UID;
    };
*/
    delete_job_id();
    failure_="Failed to create session directory.";
    failure_type_=ARexJobInternalError;
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
    delete_job_id();
    failure_="Failed registering job in grid-manager.";
    failure_type_=ARexJobInternalError;
    return;
  };
  SignalFIFO(*config_.User());
  return;
}

bool ARexJob::GetDescription(Arc::XMLNode& jsdl) {
  if(id_.empty()) return false;
  std::string sdesc;
  if(!job_description_read_file(id_,*config_.User(),sdesc)) return false;
  Arc::XMLNode xdesc(sdesc);
  if(!xdesc) return false;
  jsdl.Replace(xdesc);
  return true;
}

bool ARexJob::Cancel(void) {
  if(id_.empty()) return false;
  JobDescription job_desc(id_,"");
  if(!job_cancel_mark_put(job_desc,*config_.User())) return false;
  return true;
}

bool ARexJob::Clean(void) {
  if(id_.empty()) return false;
  JobDescription job_desc(id_,"");
  if(!job_clean_mark_put(job_desc,*config_.User())) return false;
  return true;
}

bool ARexJob::Resume(void) {
  if(id_.empty()) return false;
  return false;
}

std::string ARexJob::State(void) {
  bool job_pending;
  return State(job_pending);
}

std::string ARexJob::State(bool& job_pending) {
  if(id_.empty()) return "";
  job_state_t state = job_state_read_file(id_,*config_.User(),job_pending);
  if(state > JOB_STATE_UNDEFINED) state=JOB_STATE_UNDEFINED;
  return states_all[state].name;
}

bool ARexJob::Failed(void) {
  if(id_.empty()) return false;
  return job_failed_mark_check(id_,*config_.User());
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
    int err = errno;
    if(h == -1) {
      if(err == EEXIST) continue;
      logger_.msg(Arc::ERROR, "Failed to create file in %s", config_.User()->ControlDir());
      id_="";
      return false;
    };
    fix_file_owner(fname,*config_.User());
    close(h);
    return true;
  };
  logger_.msg(Arc::ERROR, "Out of tries while allocating new job id in %s", config_.User()->ControlDir());
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

int ARexJob::TotalJobs(ARexGMConfig& config,Arc::Logger& logger) {
  ContinuationPlugins plugins;
  JobsList jobs(*config.User(),plugins);
  jobs.ScanNewJobs();
  return jobs.size();
}

std::list<std::string> ARexJob::Jobs(ARexGMConfig& config,Arc::Logger& logger) {
  std::list<std::string> jlist;
  ContinuationPlugins plugins;
  JobsList jobs(*config.User(),plugins);
  jobs.ScanNewJobs();
  JobsList::iterator i = jobs.begin();
  for(;i!=jobs.end();++i) {
    ARexJob job(i->get_id(),config,logger,true);
    if(job) jlist.push_back(i->get_id());
  };
  return jlist;
}

std::string ARexJob::SessionDir(void) {
  if(id_.empty()) return "";
  return config_.User()->SessionRoot()+"/"+id_;
}

int ARexJob::CreateFile(const std::string& filename) {
  if(id_.empty()) return -1;
  std::string fname = config_.User()->SessionRoot()+"/"+id_+"/"+filename;
  struct stat st;
  if(lstat(fname.c_str(),&st) == 0) {
    if(!S_ISREG(st.st_mode)) return -1;
  } else {
    // Create sudirectories
    std::string::size_type n = fname.length()-filename.length();
    for(;;) {
      if(strncmp("../",fname.c_str()+n,3) == 0) return -1;
      n=fname.find('/',n);
      if(n == std::string::npos) break;
      std::string dname = fname.substr(0,n);
      ++n;
      if(lstat(dname.c_str(),&st) == 0) {
        if(!S_ISDIR(st.st_mode)) return -1;
      } else {
        if(mkdir(dname.c_str(),S_IRUSR | S_IWUSR) != 0) return -1;
        fix_file_owner(dname,*config_.User());
      };
    };
  };
  int h = open(fname.c_str(),O_WRONLY | O_CREAT,S_IRUSR | S_IWUSR);
  if(h != -1) fix_file_owner(fname,*config_.User());
  return h;
}
