#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <list>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>

#include <arc/StringConv.h>
#include <../jobdesc/job.h>
#include "info_files.h"
#include "../conf/conf.h"
//@ #include <arc/certificate.h>

#ifdef HAVE_GSOAP
#include <../jobdesc/job_jsdl.h>
#endif

#ifdef HAVE_GLOBUS_RSL
#include <../jobdesc/job_xrsl.h>
#endif

#include "info_log.h"

const char * const sfx_local       = ".local";
const char * const sfx_rsl         = ".description";
const char * const sfx_diag        = ".diag";

static void extract_integer(std::string& s,std::string::size_type n = 0) {
  for(;n<s.length();n++) {
    if(isdigit(s[n])) continue;
    s.resize(n); break;
  };
  return;
}

static void extract_float(std::string& s,std::string::size_type n = 0) {
  for(;n<s.length();n++) {
    if(isdigit(s[n])) continue;
    if(s[n] == '.') { extract_integer(s,n+1); return; };
    s.resize(n); break;
  };
  return;
}

static bool string_to_number(std::string& s,float& f) {
  extract_float(s);
  if(s.length() == 0) return false;
  if(!Arc::stringto(s,f)) return false;
  return true;
}

static bool string_to_number(std::string& s,unsigned int& n) {
  extract_integer(s);
  if(s.length() == 0) return false;
  if(!Arc::stringto(s,n)) return false;
  return true;
}

// Create multiple files for sending to logger
// TODO - make it SOAP XML so that they could be sent directly
bool job_log_make_file(const JobDescription &desc,JobUser &user,const std::string &url) {
  std::string fname_dst = user.ControlDir()+"/logs/"+desc.get_id()+".XXXXXX";
  std::string fname_src;
  std::string status;
  int h_dst;
  int l;
  time_t t;
  char buf[256];
  if((h_dst=mkstemp((char*)(fname_dst.c_str()))) == -1) {
    return false;
  };
  (void)chmod(fname_dst.c_str(),S_IRUSR | S_IWUSR);
  fix_file_owner(fname_dst,desc,user);
  fix_file_permissions(fname_dst,false);
  std::ofstream o_dst(fname_dst.c_str());
  close(h_dst);
  // URL to send info to
  if(url.length()) {
    o_dst<<"loggerurl="<<url<<std::endl; if(o_dst.fail()) goto error;
  };
  // Copy job description
  {
  fname_src = user.ControlDir() + "/job." + desc.get_id() + sfx_rsl;
  int h_src=open(fname_src.c_str(),O_RDONLY);
  if(h_src==-1) goto error; 
  o_dst<<"description=";
  for(;;) {
    l=read(h_src,buf,sizeof(buf));
    if(l==0) break;
    if(l==-1) goto error;
    for(char* p=buf;p;) { p=(char*)memchr(buf,'\r',l); if(p) (*p)=' '; };
    for(char* p=buf;p;) { p=(char*)memchr(buf,'\n',l); if(p) (*p)=' '; };
    o_dst.write(buf,l);
    if(o_dst.fail()) goto error;
  };
  o_dst<<std::endl;
  struct stat st;
  if(fstat(h_src,&st) == 0) {
    struct passwd pw_;
    struct passwd *pw;
    char buf[BUFSIZ];
    getpwuid_r(st.st_uid,&pw_,buf,BUFSIZ,&pw);
    if(pw != NULL) {
      if(pw->pw_name) o_dst<<"localuser="<<pw->pw_name<<std::endl;
    };
  };
  close(h_src);
  };
  // Start time and identifier
  t = job_mark_time(fname_src);
  o_dst<<"submissiontime="<<mds_time(t)<<std::endl;
  o_dst<<"ngjobid="<<desc.get_id()<<std::endl;
  if(o_dst.fail()) goto error;
  // Analyze job.ID.local and store relevant information
  {
  fname_src = user.ControlDir() + "/job." + desc.get_id() + sfx_local;
  std::ifstream i_src(fname_src.c_str());
  for(;;) {
    if(i_src.fail()) goto error;
    if(o_dst.fail()) goto error;
    if(i_src.eof()) break;
    std::string value;
    std::string key = config_read_line(i_src,value,'=');
    if(key=="subject") { o_dst<<"usersn="<<value<<std::endl; }
    else if(key=="lrms") { o_dst<<"lrms="<<value<<std::endl; }
    else if(key=="queue") { o_dst<<"queue="<<value<<std::endl; }
    else if(key=="localid") { o_dst<<"localjobid="<<value<<std::endl; }
    else if(key=="jobname") { o_dst<<"jobname="<<value<<std::endl; }
    else if(key=="clientname") { o_dst<<"clienthost="<<value<<std::endl; }
  };
  };
  // Extract requested resources
  {
  fname_src = user.ControlDir() + "/job." + desc.get_id() + sfx_rsl;
  JobRequest* job = NULL;
#ifdef HAVE_GSOAP
  if(job == NULL) try {
    std::ifstream i_src(fname_src.c_str());
    job = new JobRequestJSDL(i_src); 
  } catch (std::exception e) { };
#endif
#ifdef HAVE_GLOBUS_RSL
  if(job == NULL) try {
      std::ifstream i_src(fname_src.c_str());
      job = new JobRequestXRSL(i_src); 
  } catch (std::exception e) { };
#endif
  if(job == NULL) goto error;
  if(job->Memory()>=0) o_dst<<"requestedmemory="<<job->Memory()<<std::endl;
  if(job->CPUTime()>=0) o_dst<<"requestedcputime="<<job->CPUTime()<<std::endl;
  if(job->WallTime()>=0) o_dst<<"requestedwalltime="<<job->WallTime()<<std::endl;
  if(job->Disk()>=0) o_dst<<"requesteddisk="<<job->Disk()<<std::endl;
  std::list<RuntimeEnvironment> re = job->RuntimeEnvironments();
  if (re.size()>0)
  {
    std::string runtimeenvironments;
    for(std::list<RuntimeEnvironment>::const_iterator it=re.begin(); it!=re.end(); ++it)
    {
      runtimeenvironments+=" ";
	runtimeenvironments+=it->str();
    }
    o_dst<<"runtimeenvironment="<<runtimeenvironments<<std::endl;
  }

  delete job;
  };
  // Analyze diagnostics and store relevant information
  {
  fname_src = user.ControlDir() + "/job." + desc.get_id() + sfx_diag;
  std::ifstream i_src(fname_src.c_str());
  if(!i_src.fail()) {
    std::string nodenames;
    int nodecount = 0;
    float cputime = 0;
    for(;;) {
      if(i_src.fail()) goto error;
      if(o_dst.fail()) goto error;
      if(i_src.eof()) break;
      std::string value;
      std::string key = config_read_line(i_src,value,'=');
      if(key=="nodename") {
        if(nodecount) nodenames+=":"; nodenames+=value;
        nodecount++;
      } else if(strcasecmp(key.c_str(),"walltime") == 0) {
        float f;
        if(string_to_number(value,f))
          o_dst<<"usedwalltime="<<(unsigned int)f<<std::endl;
      } else if(strcasecmp(key.c_str(),"kerneltime") == 0) {
        float f;
        if(string_to_number(value,f)) cputime+=f;
      } else if(strcasecmp(key.c_str(),"usertime") == 0) {
        float f;
        if(string_to_number(value,f)) cputime+=f;
      } else if(strcasecmp(key.c_str(),"averagetotalmemory") == 0) {
        float f;
        if(string_to_number(value,f))
          o_dst<<"usedmemory="<<(unsigned int)f<<std::endl;
      } else if(strcasecmp(key.c_str(),"exitcode") == 0) {
        int n;
        if(Arc::stringto(value,n)) o_dst<<"exitcode="<<n<<std::endl;
      };
    };
    if(nodecount) {
      o_dst<<"nodename="<<nodenames<<std::endl;
      o_dst<<"nodecount="<<nodecount<<std::endl;
    };
    o_dst<<"usedcputime="<<(unsigned int)cputime<<std::endl;
  };
  };
  // Endtime and failure reason
  if(desc.get_state() == JOB_STATE_FINISHED) {
    status="completed";
    t = job_state_time(desc.get_id(),user);
    if(t == 0) t=::time(NULL);
    o_dst<<"endtime="<<mds_time(t)<<std::endl;
    if(job_failed_mark_check(desc.get_id(),user)) {
      std::string failure = job_failed_mark_read(desc.get_id(),user);
      o_dst<<"failurestring="<<failure<<std::endl;
      status="failed";
    };
  };
  if(status.length()) o_dst<<"status="<<status<<std::endl;
  // Identity of cluster
//@   try {
//@     Certificate cert(HOSTCERT);
//@     o_dst<<"cluster="<<cert.GetSN()<<std::endl;
//@   } catch (std::exception e) { };
  if(o_dst.fail()) goto error;
  o_dst.close();
  return true;
error:
  o_dst.close();
  unlink(fname_dst.c_str());
  return false;
}

