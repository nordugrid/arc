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
#include <arc/DateTime.h>
#include <arc/compute/JobDescription.h>
#include "../jobs/GMJob.h"
#include "ControlFileHandling.h"
#include "../conf/GMConfig.h"
#include "../conf/ConfigUtils.h"

#include "JobLogFile.h"

namespace ARex {

const char * const sfx_local       = ".local";
const char * const sfx_rsl         = ".description";
const char * const sfx_diag        = ".diag";
const char * const sfx_proxy       = ".proxy";

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

/*
static bool string_to_number(std::string& s,unsigned int& n) {
  extract_integer(s);
  if(s.length() == 0) return false;
  if(!Arc::stringto(s,n)) return false;
  return true;
}
*/

// Create multiple files for sending to logger
// TODO - make it SOAP XML so that they could be sent directly
bool job_log_make_file(const GMJob &job,const GMConfig& config,const std::string &url,std::list<std::string> &report_config) {
  std::string fname_dst = config.ControlDir()+"/logs/"+job.get_id()+".XXXXXX";
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
  fix_file_owner(fname_dst,job);
  fix_file_permissions(fname_dst,false);
  std::ofstream o_dst(fname_dst.c_str());
  close(h_dst);
  // URL to send info to
  if(url.length()) {
    o_dst<<"loggerurl="<<url<<std::endl; if(o_dst.fail()) goto error;
  };
  // Configuration options for usage reporter tool
  for (std::list<std::string>::iterator sp = report_config.begin();
       sp != report_config.end();
       ++sp)
    {
      o_dst<<*sp<<std::endl;
    }
  // Copy job description
  {
  fname_src = config.ControlDir() + "/job." + job.get_id() + sfx_rsl;
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
  o_dst<<"submissiontime="<<Arc::Time(t).str(Arc::MDSTime)<<std::endl;
  o_dst<<"ngjobid="<<job.get_id()<<std::endl;
  if(o_dst.fail()) goto error;
  // Analyze job.ID.local and store relevant information
  {
  fname_src = config.ControlDir() + "/job." + job.get_id() + sfx_local;
  JobLocalDescription local;
  if(!local.read(fname_src)) goto error;
  if(!local.DN.empty()) { o_dst<<"usersn="<<local.DN<<std::endl; }
  if(!local.headnode.empty()) { o_dst<<"headnode="<<local.headnode<<std::endl; }
  if(!local.lrms.empty()) { o_dst<<"lrms="<<local.lrms<<std::endl; }
  if(!local.queue.empty()) { o_dst<<"queue="<<local.queue<<std::endl; }
  if(!local.localid.empty()) { o_dst<<"localid="<<local.localid<<std::endl; }
  if(!local.jobname.empty()) { o_dst<<"jobname="<<local.jobname<<std::endl; }
  if(!local.globalid.empty()) { o_dst<<"globalid="<<local.globalid<<std::endl; }
  for(std::list<std::string>::const_iterator projectname = local.projectnames.begin();
                      projectname != local.projectnames.end(); ++projectname) {
    o_dst<<"projectname="<<*projectname<<std::endl;
  };
  if(!local.clientname.empty()) { o_dst<<"clienthost="<<local.clientname<<std::endl; }
  };

  // Copy public part of user certificate chain incl. proxy
  {
    std::string user_cert;
    fname_src = config.ControlDir() + "/job." + job.get_id() + sfx_proxy;
    std::ifstream proxy_src(fname_src.c_str());
    bool in_private=false;
    for(;;) {
      if(proxy_src.bad()) goto error;
      if(proxy_src.eof()) break;
      std::string line;
      std::getline(proxy_src,line);
      if(in_private)
  { // Skip private key
    if (line.find("-----END") != std::string::npos &&
        line.find("PRIVATE KEY-----") != std::string::npos
        )           // can be RSA, DSA etc.
      in_private=false;
  }
      else
  {
    if (line.find("-----BEGIN") != std::string::npos &&
        line.find("PRIVATE KEY-----") != std::string::npos
        )           // can be RSA, DSA etc.
      in_private=true;
    else
      {
        user_cert+=line;
        if (!proxy_src.eof()) user_cert+='\\';
      }
  }
    }
    if(user_cert.length()) {
      o_dst<<"usercert="<<user_cert<<std::endl; if(o_dst.fail()) goto error;
    }
  }

  // Extract requested resources
  {
    fname_src = config.ControlDir() + "/job." + job.get_id() + sfx_rsl;
    std::string job_desc_str;
    if (!job_description_read_file(fname_src, job_desc_str)) goto error;
    Arc::JobDescription arc_job_desc;
    {
      std::list<Arc::JobDescription> arc_job_desc_list;
      if (!Arc::JobDescription::Parse(job_desc_str, arc_job_desc_list, "", "GRIDMANAGER") || arc_job_desc_list.size() != 1) goto error;
      arc_job_desc = arc_job_desc_list.front();
    }
//    if(!get_arc_job_description(fname_src, arc_job_desc)) goto error;
    if(arc_job_desc.Resources.IndividualPhysicalMemory.max>=0) o_dst<<"requestedmemory="<<arc_job_desc.Resources.IndividualPhysicalMemory.max<<std::endl;
    if(arc_job_desc.Resources.TotalCPUTime.range.max>=0) o_dst<<"requestedcputime="<<arc_job_desc.Resources.TotalCPUTime.range.max<<std::endl;
    if(arc_job_desc.Resources.TotalWallTime.range.max>=0) o_dst<<"requestedwalltime="<<arc_job_desc.Resources.TotalWallTime.range.max<<std::endl;
    if(arc_job_desc.Resources.DiskSpaceRequirement.DiskSpace.max>=0) o_dst<<"requesteddisk="<<(arc_job_desc.Resources.DiskSpaceRequirement.DiskSpace.max*1024*1024)<<std::endl;
    if(arc_job_desc.Resources.RunTimeEnvironment.getSoftwareList().size()>0) {
      std::string rteStr;
      for (std::list<Arc::Software>::const_iterator itSW = arc_job_desc.Resources.RunTimeEnvironment.getSoftwareList().begin();
           itSW != arc_job_desc.Resources.RunTimeEnvironment.getSoftwareList().end(); itSW++) {
        if (!itSW->empty() && !itSW->getVersion().empty()) {
          if (!rteStr.empty()) rteStr += " ";
          rteStr += *itSW;
        }
      }
      if (!rteStr.empty()) o_dst<<"runtimeenvironment="<<rteStr<<std::endl;
    }
  };
  // Analyze diagnostics and store relevant information
  {
  fname_src = config.ControlDir() + "/job." + job.get_id() + sfx_diag;
  std::ifstream i_src(fname_src.c_str());
  if(!i_src.fail()) {
    std::string nodenames;
    int nodecount = 0;
    float cputime = 0;
    for(;;) {
      // if(i_src.fail()) goto error; - it is better to provide something than nothing
      if(o_dst.fail()) goto error;
      // if(i_src.eof()) break; - config_read_line handles that
      std::string value;
      std::string key = config_read_line(i_src,value,'=');
      if(key.empty()) break; // only case config_read_line returns "" is then can't read anymore
      if(key=="nodename") {
        if(nodecount) nodenames+=":"; nodenames+=value;
        nodecount++;
      } else if(strcasecmp(key.c_str(),"walltime") == 0) {
        float f;
        if(string_to_number(value,f))
          o_dst<<"usedwalltime="<<(unsigned int)f<<std::endl;
      } else if(strcasecmp(key.c_str(),"kerneltime") == 0) {
        float f;
        if(string_to_number(value,f)) {
          o_dst<<"usedkernelcputime="<<(unsigned int)f<<std::endl;
          cputime+=f;
        }
      } else if(strcasecmp(key.c_str(),"usertime") == 0) {
        float f;
        if(string_to_number(value,f)) {
          o_dst<<"usedusercputime="<<(unsigned int)f<<std::endl;
          cputime+=f;
        }
      } else if(strcasecmp(key.c_str(),"averagetotalmemory") == 0) {
        float f;
        if(string_to_number(value,f))
          o_dst<<"usedmemory="<<(unsigned int)f<<std::endl;
      } else if(strcasecmp(key.c_str(),"averageresidentmemory") == 0) {
        float f;
        if(string_to_number(value,f))
          o_dst<<"usedaverageresident="<<(unsigned int)f<<std::endl;
      } else if(strcasecmp(key.c_str(),"maxresidentmemory") == 0) {
        float f;
        if(string_to_number(value,f))
          o_dst<<"usedmaxresident="<<(unsigned int)f<<std::endl;
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
  if(job.get_state() == JOB_STATE_FINISHED) {
    status="completed";
    t = job_state_time(job.get_id(),config);
    if(t == 0) t=::time(NULL);
    o_dst<<"endtime="<<Arc::Time(t).str(Arc::MDSTime)<<std::endl;
    if(job_failed_mark_check(job.get_id(),config)) {
      std::string failure = job_failed_mark_read(job.get_id(),config);
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

} // namespace ARex
