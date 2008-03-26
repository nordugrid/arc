#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <list>
#include <iostream>
#include <fstream>
#include <globus_common.h>
#include <globus_rsl.h>
#include <globus_symboltable.h>
#include <arc/StringConv.h>
#include <arc/Logger.h>
#include <arc/URL.h>
#include "../../jobs/users.h"
#include "../../files/info_files.h"
#include "../../files/info_types.h"
#include "../../files/info_types.h"
#include "../../misc/canonical_dir.h"
#include "../../conf/environment.h"
#include "../jobdesc_util.h"
#include "subst_rsl.h"
#include "parse_rsl.h"

static Arc::Logger& logger = Arc::Logger::getRootLogger();

static bool stringtoint(const std::string& s,int& i) {
  i=Arc::stringto<int>(s);
  return true;
}

static void rsl_value_to_grami(std::ostream &o,globus_rsl_value_t* value);
static void rsl_print_to_grami(std::ostream &o,globus_rsl_t *cur);
static int globus_rsl_params_get(globus_rsl_t*,const char*,char***);

#define GLOBUS_RSL_PARAM_GET(RSL,TYPE,NAME,PARAM)              \
        if(globus_rsl_param_get(RSL,TYPE,NAME,&PARAM) != 0) {  \
          logger.msg(Arc::ERROR,"Broken RSL in NAME"); goto exit;             \
        }

#define GLOBUS_RSL_PARAMS_GET(RSL,TYPE,NAME,PARAM)              \
        if(globus_rsl_params_get(RSL,NAME,&PARAM) != 0) {  \
          logger.msg(Arc::ERROR,"Broken RSL in NAME"); goto exit;             \
        }

typedef enum {
  version_is_same = 0,
  version_is_higher = 1,
  version_is_lower = 2,
  version_is_unknown = -1
} version_comparison_t;

static version_comparison_t compare_versions(const char *version,int major,int minor,int subminor) {
  const char version_header[] = "nordugrid-";
  if(strncmp(version_header,version,sizeof(version_header)-1) == 0) {
    version = version+sizeof(version_header)-1;
    char* e;
    unsigned long int v[3] = {0,0,0};
    int n = 0;
    for(;n<3;n++) {
      v[n]=strtoul(version,&e,10);
      if(*e == '.') { version=e+1; continue; };
      if(*e == 0) { n++; break; };
      break;
    };
    if(n>0) { // at least something
      if(v[0] > major) return version_is_higher;
      if(v[0] < major) return version_is_lower;
      if(v[1] > minor) return version_is_higher;
      if(v[1] < minor) return version_is_lower;
      if(v[2] > subminor) return version_is_higher;
      if(v[2] < subminor) return version_is_lower;
      return version_is_same;
    } else {
      // Unnumbered version is supposed to be current one
      return version_is_higher;
    };
  };
  return version_is_unknown;
}

static bool use_executable_in_rsl(const char *version) {
  if(compare_versions(version,0,5,26) == version_is_lower) return false;
  return true;
}

static bool use_cputime_in_seconds(const char *version) {
  if(compare_versions(version,0,5,27) == version_is_lower) return false;
  return true;
}

/* function for jobmanager-ng */
bool parse_rsl_for_action(const char* fname,
               std::string &action,std::string &jobid,
               std::string &lrms,std::string &queue) {
  JobLocalDescription job_desc;
  std::string filename(fname);
  if(parse_rsl(filename,job_desc)) {
    action=job_desc.action;
    jobid=job_desc.jobid;
    lrms=job_desc.lrms;
    queue=job_desc.queue;
    return true;
  };
  return false;
}

/* parse RSL and write few informational files */
bool process_rsl(JobUser &user,const JobDescription &desc) {
  JobLocalDescription job_desc;
  return process_rsl(user,desc,job_desc);
}

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

bool process_rsl(JobUser &user,const JobDescription &desc,JobLocalDescription &job_desc) {
  /* read local first to get some additional info pushed here by script */
  job_local_read_file(desc.get_id(),user,job_desc);
  /* some default values */
  job_desc.lrms=user.DefaultLRMS();
  job_desc.queue=user.DefaultQueue();
  job_desc.reruns=user.Reruns();
  std::string filename;
  filename = user.ControlDir() + "/job." + desc.get_id() + ".description";
  if(!parse_rsl(filename,job_desc)) return false;
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
      if(u && value.empty()) {
        u.AddOption("cache",job_desc.cache); i->lfn=u.fullstr();
      };
    };
  };
  if(!job_local_write_file(desc,user,job_desc)) return false;
  if(!job_input_write_file(desc,user,job_desc.inputdata)) return false;
  if(!job_output_write_file(desc,user,job_desc.outputdata)) return false;
  return true;  
}

void add_non_cache(const char *fname,std::list<FileData> &inputdata) {
  for(std::list<FileData>::iterator i=inputdata.begin();i!=inputdata.end();++i){
    if(i->has_lfn()) if((*i) == fname) {
      Arc::URL u(i->lfn);
      if(u) {
        u.AddOption("cache","no");
        u.AddOption("exec","yes");
        i->lfn=u.fullstr();
      };
    };
  };
}

/* parse rsl and set specified file permissions to executable */
bool set_execs(globus_rsl_t *rsl_tree,const std::string &session_dir) {
  bool res=false;
  char** tmp_param;
  bool use_executable = true;
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
             NG_RSL_SOFTWARE_PARAM,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL in clientsoftware"); goto exit;
  };
  if(tmp_param[0] != NULL) {
    use_executable=use_executable_in_rsl(tmp_param[0]);
  };
  globus_free(tmp_param);
  if(use_executable) {
    if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
                            NG_RSL_EXECUTABLE_PARAM,&tmp_param) != 0) {
      logger.msg(Arc::ERROR,"Broken RSL in executable"); goto exit;
    };
    if(tmp_param[0] == NULL) {
      globus_free(tmp_param);
      logger.msg(Arc::ERROR,"Missing executable in RSL"); goto exit;
    };
  } else {
    if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_MULTI_LITERAL,
                            NG_RSL_ARGUMENTS_PARAM,&tmp_param) != 0) {
      logger.msg(Arc::ERROR,"Broken RSL"); goto exit;
    };
    if(tmp_param[0] == NULL) {
      globus_free(tmp_param);
      logger.msg(Arc::ERROR,"Missing arguments in RSL"); goto exit;
    };
  };
  /* executable can be external, so first check for initial slash */
  if(( tmp_param[0][0] != '/' ) && ( tmp_param[0][0] != '$' )){
    fix_file_permissions(session_dir+"/"+tmp_param[0],true);
  };
  globus_free(tmp_param);
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_MULTI_LITERAL,
                            NG_RSL_EXECUTABLES_PARAM,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL"); goto exit;
  };
  for(int i=0;tmp_param[i]!=NULL;i++) {
    fix_file_permissions(session_dir+"/"+tmp_param[i],true);
  };
  globus_free(tmp_param);
  res=true;
exit:
  if(rsl_tree) globus_rsl_free_recursive(rsl_tree);
  return res;
}

/*
bool set_execs(const JobDescription &desc,const JobUser &user,const std::string &session_dir) {
  std::string frsl = user.ControlDir() + "/job." + desc.get_id() + ".description";
  globus_rsl_t *rsl_tree = NULL;
  rsl_tree=read_rsl(frsl);
  if(rsl_tree == NULL) return false;
  if(user.StrictSession()) {
    JobUser tmp_user(user.get_uid()==0?desc.get_uid():user.get_uid());
    RunElement* re = RunCommands::fork(tmp_user,"set_execs");
    if(re == NULL) return false;
    if(re->get_pid() != 0) return RunCommands::wait(re,20,"set_execs");
    _exit(set_execs(rsl_tree,session_dir));
  };
  return set_execs(rsl_tree,session_dir);
}
*/

/* parse rsl (and local configuration) and write grami file.
   it also returns job local description (for not to parse twice) */

bool write_grami_rsl(const JobDescription &desc,const JobUser &user,const char* opt_add) {
  bool res=false;
  bool use_executable = true;
  bool use_seconds = true;
  int runtime_num = 0;
  /* 'local' parameters are requied */
  if(desc.get_local() == NULL) return false;
  std::string session_dir = desc.SessionDir();
  JobLocalDescription& job_desc = *(desc.get_local());
  std::string fgrami = user.ControlDir() + "/job." + desc.get_id() + ".grami";
  std::string frsl = user.ControlDir() + "/job." + desc.get_id() + ".description";
  /* read RSL and write grami */
  globus_rsl_t *rsl_tree = NULL;
  rsl_tree=read_rsl(frsl);
  if(rsl_tree == NULL) return false;
  std::ofstream f(fgrami.c_str(),std::ios::out | std::ios::trunc);
  if(!f.is_open()) return false;
  if(!fix_file_owner(fgrami,desc,user)) { goto exit; };
  char** tmp_param;
  f<<"joboption_directory='"<<session_dir<<"'"<<std::endl;

  use_executable=use_executable_in_rsl(job_desc.clientsoftware.c_str());
  use_seconds=use_cputime_in_seconds(job_desc.clientsoftware.c_str());

  if(use_executable) {
    GLOBUS_RSL_PARAM_GET(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
                              NG_RSL_EXECUTABLE_PARAM,tmp_param);
    if(tmp_param[0] == NULL) {
      globus_free(tmp_param);
      logger.msg(Arc::ERROR,"Missing executable in RSL"); goto exit;
    };
    f<<"joboption_arg_0='";
    if((tmp_param[0][0] != '/') && (tmp_param[0][0] != '$')) {
      bool found=false;
      for(FileData::iterator i=job_desc.inputdata.begin();
	  i!=job_desc.inputdata.end();++i) {
	if (tmp_param[0] == i->pfn) found=true;
      }
      if (found) f<<"./";
    }
    f<<value_for_shell(tmp_param[0],false)<<"'"<<std::endl;
    globus_free(tmp_param);
    GLOBUS_RSL_PARAM_GET(rsl_tree,GLOBUS_RSL_PARAM_MULTI_LITERAL,
                              NG_RSL_ARGUMENTS_PARAM,tmp_param);
    for(int i=0;tmp_param[i]!=NULL;i++) { 
      f<<"joboption_arg_"<<(i+1)<<"="<<value_for_shell(tmp_param[i],true)<<std::endl;
    };
    globus_free(tmp_param);
  } else {
    GLOBUS_RSL_PARAM_GET(rsl_tree,GLOBUS_RSL_PARAM_MULTI_LITERAL,
                              NG_RSL_ARGUMENTS_PARAM,tmp_param);
    if(tmp_param[0] == NULL) {
      globus_free(tmp_param);
      logger.msg(Arc::ERROR,"Missing arguments in RSL"); goto exit;
    };
    f<<"joboption_arg_0='";
    if((tmp_param[0][0] != '/') && (tmp_param[0][0] != '$')) f<<"./";
    f<<value_for_shell(tmp_param[0],false)<<"'"<<std::endl;
    for(int i=1;tmp_param[i]!=NULL;i++) { 
      f<<"joboption_arg_"<<i<<"="<<value_for_shell(tmp_param[i],true)<<std::endl;
    };
    globus_free(tmp_param);
  };

  GLOBUS_RSL_PARAM_GET(rsl_tree,GLOBUS_RSL_PARAM_SEQUENCE,
                           NG_RSL_ENVIRONMENT_PARAM,tmp_param);
  {
    int i;
    for(i=0;tmp_param[i]!=NULL;i++) { 
      f<<"joboption_env_"<<(i/2)<<"='"<<value_for_shell(tmp_param[i],false)<<"=";
      i++; if(tmp_param[i]==NULL) { f<<"'"<<std::endl; break; };
      f<<value_for_shell(tmp_param[i],false)<<"'"<<std::endl;
    };
/*
    f<<joboption_env_"<<(i/2)<<"='X509_CERT_DIR=/etc/grid-security/certificates'"<<std::endl; i+=2;
    f<<joboption_env_"<<(i/2)<<"='GLOBUS_LOCATION="<<globus_loc<<"'"<<std::endl; i+=2;
    f<<joboption_env_"<<(i/2)<<"='X509_USER_PROXY="<<user.ControlDir()+"/job."+desc.get_id()+".proxy'"<<std::endl;
*/
  };
  globus_free(tmp_param);
  GLOBUS_RSL_PARAMS_GET(rsl_tree,GLOBUS_RSL_PARAM_MULTI_LITERAL,
                                   NG_RSL_RUNTIME_PARAM,tmp_param);
  for(int i=0;tmp_param[i]!=NULL;i++) { 
    for(char* s=tmp_param[i];*s;s++) (*s)=toupper(*s);
    std::string tmp_s = tmp_param[i];
    if(canonical_dir(tmp_s) != 0) {
      logger.msg(Arc::ERROR,"Bad name for runtime environment: %s",tmp_param[i]);
      globus_free(tmp_param); goto exit;
    };
    f<<"joboption_runtime_"<<runtime_num<<"="<<value_for_shell(tmp_param[i],true)<<std::endl;
    runtime_num++;
  };
  globus_free(tmp_param);
  GLOBUS_RSL_PARAM_GET(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
                       NG_RSL_STDIN_PARAM3,tmp_param);
  if(tmp_param[0] == NULL) {
    globus_free(tmp_param);
    GLOBUS_RSL_PARAM_GET(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
                                     NG_RSL_STDIN_PARAM,tmp_param);
    if(tmp_param[0] == NULL) {
      globus_free(tmp_param);
      GLOBUS_RSL_PARAM_GET(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
                                    NG_RSL_STDIN_PARAM2,tmp_param);
    };
  };
  if(tmp_param[0] == NULL) {
    tmp_param[0]=NG_RSL_DEFAULT_STDIN;
  };
  f<<"joboption_stdin="<<value_for_shell(tmp_param[0],true)<<std::endl;
  globus_free(tmp_param);
  GLOBUS_RSL_PARAM_GET(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
                            NG_RSL_STDOUT_PARAM,tmp_param);
  if(tmp_param[0] == NULL) { 
    std::string stdout_=NG_RSL_DEFAULT_STDOUT;
    f<<"joboption_stdout="<<value_for_shell(stdout_,true)<<std::endl;
  }
  else {
    std::string stdout_=tmp_param[0];
    if(canonical_dir(stdout_) != 0) {
      globus_free(tmp_param);
      logger.msg(Arc::ERROR,"Bad name for stdout: %s",stdout_); goto exit;
    };
    stdout_=session_dir+stdout_;
    f<<"joboption_stdout="<<value_for_shell(stdout_,true)<<std::endl;
  };
  globus_free(tmp_param);
  GLOBUS_RSL_PARAM_GET(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
                            NG_RSL_STDERR_PARAM,tmp_param);
  if(tmp_param[0] == NULL) { 
    std::string stderr_=NG_RSL_DEFAULT_STDOUT;
    f<<"joboption_stderr="<<value_for_shell(stderr_,true)<<std::endl;
  }
  else {
    std::string stderr_=tmp_param[0];
    if(canonical_dir(stderr_) != 0) {
      globus_free(tmp_param);
      logger.msg(Arc::ERROR,"Bad name for stderr: %s",stderr_); goto exit;
    };
    stderr_=session_dir+stderr_;
    f<<"joboption_stderr="<<value_for_shell(stderr_,true)<<std::endl;
  };
  globus_free(tmp_param);

  GLOBUS_RSL_PARAM_GET(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
                      NG_RSL_CPUTIME_PARAM,tmp_param);
  if(tmp_param[0] == NULL) { f<<"joboption_cputime="<<std::endl; }
  else {
    if(use_seconds) {
      f<<"joboption_cputime="<<numvalue_for_shell(tmp_param[0])<<std::endl;
    } else {
      f<<"joboption_cputime="<<numvalue_for_shell(tmp_param[0])*60<<std::endl;
    };
  };
  globus_free(tmp_param);
  GLOBUS_RSL_PARAM_GET(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
                      NG_RSL_WALLTIME_PARAM,tmp_param);
  if(tmp_param[0] == NULL) { f<<"joboption_walltime="<<std::endl; }
  else { f<<"joboption_walltime="<<numvalue_for_shell(tmp_param[0])<<std::endl; };
  globus_free(tmp_param);
  GLOBUS_RSL_PARAM_GET(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
                      NG_RSL_MEMORY_PARAM,tmp_param);
  if(tmp_param[0] == NULL) { f<<"joboption_memory="<<std::endl; }
  else { f<<"joboption_memory="<<numvalue_for_shell(tmp_param[0])<<std::endl; };
  globus_free(tmp_param);
  GLOBUS_RSL_PARAM_GET(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
                      NG_RSL_COUNT_PARAM,tmp_param);
  if(tmp_param[0] == NULL) { f<<"joboption_count=1"<<std::endl; }
  else { f<<"joboption_count="<<numvalue_for_shell(tmp_param[0])<<std::endl; };
  globus_free(tmp_param);
//  f<<"joboption_jobtype=1" << std::endl; 
  f<<"joboption_queue="<<value_for_shell(job_desc.queue,true)<<std::endl;
  f<<"joboption_jobname="<<value_for_shell(job_desc.jobname,true)<<std::endl;
  if(job_desc.exectime.defined()) {
    f<<"joboption_starttime="<<job_desc.exectime<<std::endl;
  }
  else {
    f<<"joboption_starttime="<<std::endl;
  };
//  GLOBUS_RSL_PARAM_GET(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
//                        NG_RSL_PROJECT_PARAM,tmp_param);
//  if(tmp_param[0] == NULL) { f << "grami_project=''" << std::endl;  }
//  else { f << "grami_project='" << tmp_param[0] << "'" << std::endl;  };
//  globus_free(tmp_param);
  f<<"joboption_gridid="<<value_for_shell(desc.get_id(),true)<<std::endl;
  if(opt_add) f<<opt_add<<std::endl;
  // put all RSL attributes to grami for super-clever submission script
  f<<"joboption_rsl="<<frsl<<std::endl;
  rsl_print_to_grami(f,rsl_tree);
  res=true;
exit:
  if(rsl_tree) globus_rsl_free_recursive(rsl_tree);
  f.close();
  return res;
}

/* extract joboption_jobid from grami file *
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
exit:
  f.close();
  return id;
}
*/

globus_rsl_t* read_rsl(const std::string &fname) {
  globus_rsl_t *rsl_tree = NULL;
  std::string rsl;
  char* rsl_spec;
  if(!job_description_read_file(fname,rsl)) {
    logger.msg(Arc::ERROR,"Failed reading RSL"); return NULL; 
  };
  rsl_spec=strdup(rsl.c_str());
  rsl.erase();
  /* parse RSL */
  rsl_tree=globus_rsl_parse(rsl_spec);
  globus_free(rsl_spec); rsl_spec = GLOBUS_NULL;
  return rsl_tree;
}

bool write_rsl(const std::string &fname,globus_rsl_t *rsl) {
  char* rsl_spec = NULL;
  if((rsl_spec = globus_rsl_unparse(rsl)) == GLOBUS_NULL) return false;
  if(!job_description_write_file(fname,rsl_spec)) {
    globus_free(rsl_spec);
    logger.msg(Arc::ERROR,"Failed writing RSL"); return false; 
  };
  globus_free(rsl_spec);
  return true;
}

/* parse rsl, fill job description (local) */
bool parse_rsl(const std::string &fname,JobLocalDescription &job_desc,std::string* acl) {
  char** tmp_param;
  globus_rsl_t *rsl_tree = NULL;
  bool use_executable=true;

  bool res = false;

  /* read RSL */
  rsl_tree=read_rsl(fname);
  if (!rsl_tree) {
    logger.msg(Arc::ERROR,"Failed parsing RSL"); goto exit;
  };
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
             NG_RSL_SOFTWARE_PARAM,&tmp_param) != 0) {
   logger.msg(Arc::ERROR,"Broken RSL in clientsoftware"); goto exit;
  };
  if(tmp_param[0] != NULL) {
    job_desc.clientsoftware=tmp_param[0];
    use_executable=use_executable_in_rsl(job_desc.clientsoftware.c_str());
  };
  globus_free(tmp_param);
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
                            NG_RSL_JOB_ID_PARAM,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL in jobid"); goto exit;
  };
  if(tmp_param[0]) job_desc.jobid=tmp_param[0];
  if(job_desc.jobid.find('/') != std::string::npos) {
    globus_free(tmp_param);
    logger.msg(Arc::ERROR,"slashes are not allowed in jobid"); goto exit;
  };
  globus_free(tmp_param);
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
                            NG_RSL_ACTION_PARAM,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL in action"); goto exit;
  };
  if(tmp_param[0] == NULL) {
    logger.msg(Arc::ERROR,"Missing action in RSL - using default");
    job_desc.action="request";
  } else {
    job_desc.action=tmp_param[0];
  };
  if(strcasecmp(job_desc.action.c_str(),"request")) {
     /* not a job request, need nothing more */
    globus_free(tmp_param);
    res=true; goto exit;
  };
  globus_free(tmp_param);
/*
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
                            NG_RSL_LRMS_PARAM,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL in lrmstype"); goto exit;
  };
  if(tmp_param[0] != NULL) { job_desc.lrms=tmp_param[0]; };
  globus_free(tmp_param);
*/
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
                            NG_RSL_QUEUE_PARAM,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL in queue"); goto exit;
  };
  if(tmp_param[0] != NULL) { job_desc.queue=tmp_param[0]; }
  globus_free(tmp_param);
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
                            NG_RSL_REPLICA_PARAM,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL in replicacollection"); goto exit;
  };
  if(tmp_param[0] != NULL) { job_desc.rc=tmp_param[0]; };
  globus_free(tmp_param);
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
                            NG_RSL_LIFETIME_PARAM,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL in lifetime"); goto exit;
  };
  if(tmp_param[0] != NULL) { job_desc.lifetime=tmp_param[0]; };
  globus_free(tmp_param);
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
                            NG_RSL_STARTTIME_PARAM,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL in starttime"); goto exit;
  };
  if(tmp_param[0] != NULL) { job_desc.processtime=tmp_param[0]; };
  globus_free(tmp_param);
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
                            NG_RSL_JOBNAME_PARAM,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL in jobname"); goto exit;
  };
  if(tmp_param[0] != NULL) { job_desc.jobname=tmp_param[0]; };
  globus_free(tmp_param);
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
                            NG_RSL_JOBREPORT_PARAM,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL in jobreport"); goto exit;
  };
  if(tmp_param[0] != NULL) { job_desc.jobreport=tmp_param[0]; };
  globus_free(tmp_param);
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
                            NG_RSL_RERUN_PARAM,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL in rerun"); goto exit;
  };
  if(tmp_param[0] != NULL) {
    if(!stringtoint(std::string(tmp_param[0]),job_desc.reruns)) {
      globus_free(tmp_param);
      logger.msg(Arc::ERROR,"Bad integer in rerun"); goto exit;
    };
  };
  globus_free(tmp_param);
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
                            NG_RSL_DISKSPACE_PARAM,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL in disk"); goto exit;
  };
  if(tmp_param[0] != NULL) {
    double ds = 0.0;
    if((sscanf(tmp_param[0],"%lf",&ds) != 1) || (ds < 0)) {
      globus_free(tmp_param);
      logger.msg(Arc::ERROR,"disk value is bad"); goto exit;
    };
    job_desc.diskspace=(unsigned long long int)(ds*1024*1024*1024); /* unit - GB */
  };
  globus_free(tmp_param);
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
                            NG_RSL_NOTIFY_PARAM,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL in notify"); goto exit;
  };
  if(tmp_param[0] != NULL) { job_desc.notify=tmp_param[0]; };
  globus_free(tmp_param);
  job_desc.arguments.clear();
  if(use_executable) {
    if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
                            NG_RSL_EXECUTABLE_PARAM,&tmp_param) != 0) {
      logger.msg(Arc::ERROR,"Broken RSL in executable"); goto exit;
    };
    if(tmp_param[0] == NULL) {
      globus_free(tmp_param);
      logger.msg(Arc::ERROR,"Missing executable in RSL"); goto exit;
    };
    job_desc.arguments.push_back(std::string(tmp_param[0]));
    globus_free(tmp_param);
  };
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_MULTI_LITERAL,
                            NG_RSL_ARGUMENTS_PARAM,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL in arguments"); goto exit;
  };
  if((!use_executable) && (tmp_param[0] == NULL)) {
    globus_free(tmp_param);
    logger.msg(Arc::ERROR,"Missing arguments in RSL"); goto exit;
  };
  for(int i=0;tmp_param[i]!=NULL;i++) {
    job_desc.arguments.push_back(std::string(tmp_param[i]));
  };
  globus_free(tmp_param);
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SEQUENCE,
                            NG_RSL_INPUT_DATA_PARAM,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL in inputdata"); goto exit;
  };
  job_desc.inputdata.clear();
  job_desc.downloads=0;
  for(int i=0;tmp_param[i]!=NULL;i+=2) {
    FileData fdata(tmp_param[i],tmp_param[i+1]);
    job_desc.inputdata.push_back(fdata);
    if(fdata.has_lfn()) job_desc.downloads++;
    if(tmp_param[i+1]==NULL) break;
  };
  globus_free(tmp_param);
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SEQUENCE,
                            NG_RSL_OUTPUT_DATA_PARAM,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL in outputdata"); goto exit;
  };
  job_desc.outputdata.clear();
  job_desc.uploads=0;
  for(int i=0;tmp_param[i]!=NULL;i+=2) {
    FileData fdata(tmp_param[i],tmp_param[i+1]);
    job_desc.outputdata.push_back(fdata);
    if(fdata.has_lfn()) job_desc.uploads++;
    if(tmp_param[i+1]==NULL) break;
  };
  globus_free(tmp_param);
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
             NG_RSL_STDLOG_PARAM,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL in gmlog"); goto exit;
  };
  if(tmp_param[0] != NULL) { job_desc.stdlog=tmp_param[0]; };
  globus_free(tmp_param);
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
             NG_RSL_STDOUT_PARAM,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL in stdout"); goto exit;
  };
  if(tmp_param[0] != NULL) { job_desc.stdout_=tmp_param[0]; };
  globus_free(tmp_param);
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
             NG_RSL_STDERR_PARAM,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL in stderr"); goto exit;
  };
  if(tmp_param[0] != NULL) { job_desc.stderr_=tmp_param[0]; };
  globus_free(tmp_param);
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
             NG_RSL_STDIN_PARAM3,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL in stdin"); goto exit;
  };
  if(tmp_param[0] == NULL) {
    globus_free(tmp_param);
    if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
             NG_RSL_STDIN_PARAM,&tmp_param) != 0) {
      logger.msg(Arc::ERROR,"Broken RSL"); goto exit;
    };
    if(tmp_param[0] == NULL) {
      globus_free(tmp_param);
      if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
               NG_RSL_STDIN_PARAM2,&tmp_param) != 0) {
        logger.msg(Arc::ERROR,"Broken RSL"); goto exit;
      };
    };
  };
  if(tmp_param[0] != NULL) { job_desc.stdin_=tmp_param[0]; };
  globus_free(tmp_param);
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
             NG_RSL_FTPTHREADS_PARAM,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL in ftpthreads"); goto exit;
  };
  if(tmp_param[0] != NULL) { 
    if(!stringtoint(std::string(tmp_param[0]),job_desc.gsiftpthreads)) {
      globus_free(tmp_param);
      logger.msg(Arc::ERROR,"Bad integer in ftpthreads"); goto exit;
    };
  };
  globus_free(tmp_param);
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
             NG_RSL_CACHE_PARAM,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL in cache"); goto exit;
  };
  if(tmp_param[0] != NULL) { 
    if( strcmp(tmp_param[0],"yes") && strcmp(tmp_param[0],"no") ) {
      globus_free(tmp_param);
      logger.msg(Arc::ERROR,"Bad value in cache"); goto exit;
    };
    job_desc.cache = std::string(tmp_param[0]);
  };
  globus_free(tmp_param);
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
             NG_RSL_HOSTNAME_PARAM,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL in hostname"); goto exit;
  };
  if(tmp_param[0] != NULL) {
    if(job_desc.clientname.length()!=0) { /* keep predefined value */
      if(job_desc.clientname.find(';') == std::string::npos) {
        job_desc.clientname+=";"; job_desc.clientname+=tmp_param[0];
      };
    }
    else {
      job_desc.clientname=tmp_param[0];
    };
  };
  globus_free(tmp_param);
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
             NG_RSL_DRY_RUN_PARAM,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL in dryrun"); goto exit;
  };
  if(tmp_param[0] != NULL) { 
    if(!strcasecmp(tmp_param[0],"yes")) job_desc.dryrun=true;
  };
  globus_free(tmp_param);
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
             NG_RSL_SESSION_TYPE_PARAM,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL in sessiondirectorytype"); goto exit;
  };
  if(tmp_param[0] != NULL) {
    if(!strcasecmp(tmp_param[0],"full")) { job_desc.fullaccess=true; }
    else if((!strcasecmp(tmp_param[0],"readonly")) ||
            (!strcasecmp(tmp_param[0],"limited")) ||
            (!strcasecmp(tmp_param[0],"internal"))) {
      job_desc.fullaccess=false;
    } else {
      globus_free(tmp_param);
      logger.msg(Arc::ERROR,"session_directory_type value is bad"); goto exit;
    };
  };
  globus_free(tmp_param);
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
             NG_RSL_CRED_SERVER_PARAM,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL in credentialserver"); goto exit;
  };
  if(tmp_param[0] != NULL) {
    job_desc.credentialserver=tmp_param[0];
  };
  globus_free(tmp_param);
  /* add cache=no to executable input files */
  if(job_desc.arguments.size() > 0) {
    if(( job_desc.arguments.begin()->c_str()[0] != '/' ) && 
       ( job_desc.arguments.begin()->c_str()[0] != '$' )){
      add_non_cache(job_desc.arguments.begin()->c_str(),job_desc.inputdata);
    };
  };
  if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_MULTI_LITERAL,
                            NG_RSL_EXECUTABLES_PARAM,&tmp_param) != 0) {
    logger.msg(Arc::ERROR,"Broken RSL"); goto exit;
  };
  for(int i=0;tmp_param[i]!=NULL;i++) {
    add_non_cache(tmp_param[i],job_desc.inputdata);
  };
  globus_free(tmp_param);
  if(acl) {
    if (globus_rsl_param_get(rsl_tree,GLOBUS_RSL_PARAM_SINGLE_LITERAL,
               NG_RSL_ACL_PARAM,&tmp_param) != 0) {
      logger.msg(Arc::ERROR,"Broken RSL in acl"); goto exit;
    };
    if(tmp_param[0] != NULL) {
      (*acl)=tmp_param[0];
    };
    globus_free(tmp_param);
  };
  res=true;
exit:
  if(rsl_tree) globus_rsl_free_recursive(rsl_tree);
  return res;
}

/* do rsl substitution and nordugrid specific stuff */
/* then write file back */
bool preprocess_rsl(const std::string &fname,const std::string &session_dir,const std::string &jobid) {
  globus_rsl_t *rsl_tree = NULL;
  rsl_subst_table_t *symbol_table = NULL;

  bool res = false;

  rsl_tree=read_rsl(fname);
  if (!rsl_tree) {
    logger.msg(Arc::ERROR,"Failed parsing RSL"); goto exit;
  };

  symbol_table = (rsl_subst_table_t *) globus_libc_malloc
                            (sizeof(rsl_subst_table_t));
  rsl_subst_table_init(symbol_table);
  rsl_subst_table_insert(symbol_table,
                           strdup("ARC_LOCATION"),
                           strdup(nordugrid_loc.c_str()));
  rsl_subst_table_insert(symbol_table,
                           strdup("ARC_SESSION_DIR"),
                           strdup(session_dir.c_str()));
  rsl_subst_table_insert(symbol_table,
                           strdup("ARC_JOB_ID"),
                           strdup(jobid.c_str()));
  rsl_subst_table_insert(symbol_table,
                           strdup("GLOBUS_LOCATION"),
                           strdup(globus_loc.c_str()));
//  if(globus_rsl_eval(rsl_tree, symbol_table) != 0) {
//    logger.msg(Arc::ERROR,"Failed evaluating RSL"); goto exit;
//  };
  if(rsl_subst(rsl_tree, symbol_table) != 0) {
    logger.msg(Arc::ERROR,"Failed evaluating RSL"); goto exit;
  };
  if(!write_rsl(fname,rsl_tree)) goto exit;
  res=true;
exit:
  if(rsl_tree) globus_rsl_free_recursive(rsl_tree);
  if(symbol_table) rsl_subst_table_destroy(symbol_table);
  return res;
}

/*
static char* rsl_operators [11] = {
  "?0?","=","!=",">",">=","<","<=","?7?","&","|","multi"
};
*/

static void rsl_print_to_grami(std::ostream &o,globus_rsl_t *cur) {
  if(globus_rsl_is_boolean(cur)) {
    globus_rsl_t* cur_;
    globus_list_t *list = cur->req.boolean.operand_list;
    for(;;) {
      if(globus_list_empty(list)) break;
      cur_=(globus_rsl_t*)globus_list_first(list);
      rsl_print_to_grami(o,cur_);
      list=globus_list_rest(list);
    };
  }
  else if(globus_rsl_is_relation(cur)) {
    if(cur->req.relation.my_operator==GLOBUS_RSL_EQ) {
      std::string attribute_name = cur->req.relation.attribute_name;
      for(int n = 0;n<attribute_name.length();++n) {
        attribute_name[n]=tolower(attribute_name[n]);
      };
      o<<"joboption_rsl_"<<attribute_name<<"='";
      rsl_value_to_grami(o,cur->req.relation.value_sequence);
      o<<"'"<<std::endl;
    };
  }
  else {
  };
}

static void rsl_value_to_grami(std::ostream &o,globus_rsl_value_t* cur) {
  if(globus_rsl_value_is_literal(cur)) {
    o<<value_for_shell(cur->value.literal.string,false); 
  }
  else if(globus_rsl_value_is_variable(cur)) {
    rsl_value_to_grami(o,cur->value.variable.sequence);
  }
  else if(globus_rsl_value_is_concatenation(cur)) {
    rsl_value_to_grami(o,cur->value.concatenation.left_value);
    rsl_value_to_grami(o,cur->value.concatenation.right_value);
  }
  else if(globus_rsl_value_is_sequence(cur)) {
    globus_rsl_value_t* cur_;
    globus_list_t *list = cur->value.sequence.value_list;
    bool first = true;
    for(;;) {
      if(globus_list_empty(list)) break;
      if(!first) { o<<" "; } else { first=false; };
      cur_=(globus_rsl_value_t*)globus_list_first(list);
      rsl_value_to_grami(o,cur_);
      list=globus_list_rest(list);
    };
  }
  else {
  };
}

static int globus_rsl_params_get(globus_rsl_t *cur,const char* name,char*** tmp_param) { 
  int params=0;
  *tmp_param=(char**)globus_malloc(sizeof(char*));
  if(*tmp_param == NULL) return 1;
  (*tmp_param)[0]=0;
  if(globus_rsl_is_boolean(cur)) {
    globus_rsl_t* cur_; 
    globus_list_t *list = cur->req.boolean.operand_list;
    for(;;) {
      char** tmp_param_ = NULL;
      if(globus_list_empty(list)) break;
      cur_=(globus_rsl_t*)globus_list_first(list);
      globus_rsl_params_get(cur_,name,&tmp_param_);
      if(tmp_param_) {
        int n = 0;
        for(;tmp_param_[n];n++) { };
        if(n > 0) {
          char** tmp_param__ = (char**)globus_realloc(*tmp_param,sizeof(char*)*(params+n+1));
          if(tmp_param__) {
            memcpy(tmp_param__+params,tmp_param_,sizeof(char*)*n);
            params+=n;
            tmp_param__[params]=0;
            (*tmp_param)=tmp_param__;
          };
        };
        globus_free(tmp_param_);
      };
      list=globus_list_rest(list);
    };
  } else if(globus_rsl_is_relation(cur)) {
    if(strcasecmp(cur->req.relation.attribute_name,name) == 0) {
      return globus_rsl_param_get(cur,GLOBUS_RSL_PARAM_MULTI_LITERAL,(char*)name,tmp_param);
    };
  };
  return 0;
}

