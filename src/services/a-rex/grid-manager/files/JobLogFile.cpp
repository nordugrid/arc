#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>

#include <arc/StringConv.h>
#include <arc/DateTime.h>
#include <arc/FileUtils.h>
#include <arc/compute/JobDescription.h>
#include "../jobs/GMJob.h"
#include "../conf/GMConfig.h"
#include "ControlFileHandling.h"

#include "JobLogFile.h"

namespace ARex {

const char * const sfx_desc        = ".description";
const char * const sfx_diag        = ".diag";
const char * const sfx_statistics  = ".statistics";

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

  // contents of the job record
  std::string job_data;

  // URL to send info to
  if (!url.empty()) job_data += "loggerurl=" + url + '\n';

  // Configuration options for usage reporter tool
  for (std::list<std::string>::iterator sp = report_config.begin();
       sp != report_config.end(); ++sp) {
    job_data += *sp + '\n';
  }

  // Copy job description
  std::string fname_src(config.ControlDir() + "/job." + job.get_id() + sfx_desc);
  std::string desc;
  if (!Arc::FileRead(fname_src, desc)) return false;
  // Remove line breaks
  std::replace( desc.begin(), desc.end(), '\r', ' ');
  std::replace( desc.begin(), desc.end(), '\n', ' ');
  job_data += "description=" + desc + '\n';

  // Find local owner of job from owner of desc file
  struct stat st;
  if (Arc::FileStat(fname_src, &st, false)) {
    struct passwd pw_;
    struct passwd *pw;
    char buf[BUFSIZ];
    getpwuid_r(st.st_uid,&pw_,buf,BUFSIZ,&pw);
    if (pw != NULL && pw->pw_name) {
      job_data += "localuser=" + std::string(pw->pw_name) + '\n';
    }
  }

  // Start time and identifier
  time_t t = job_mark_time(fname_src);
  job_data += "submissiontime=" + Arc::Time(t).str(Arc::MDSTime) + '\n';
  job_data += "ngjobid=" + job.get_id() + '\n';

  // Analyze job.ID.local and store relevant information
  JobLocalDescription local;
  if (!job_local_read_file(job.get_id(), config, local)) return false;
  if (!local.DN.empty()) job_data += "usersn=" + local.DN + '\n';
  if (!local.headnode.empty()) job_data += "headnode=" + local.headnode + '\n';
  if (!local.lrms.empty()) job_data += "lrms=" + local.lrms + '\n';
  if (!local.queue.empty()) job_data += "queue=" + local.queue + '\n';
  if (!local.localid.empty()) job_data += "localid=" + local.localid + '\n';
  if (!local.jobname.empty()) job_data += "jobname=" + local.jobname + '\n';
  if (!local.globalid.empty()) job_data += "globalid=" + local.globalid + '\n';
  for (std::list<std::string>::const_iterator projectname = local.projectnames.begin();
                      projectname != local.projectnames.end(); ++projectname) {
    job_data += "projectname=" + *projectname + '\n';
  }
  if (!local.clientname.empty()) job_data += "clienthost=" + local.clientname + '\n';

  // Copy public part of user certificate chain incl. proxy
  fname_src = job_proxy_filename(job.get_id(), config);
  std::list<std::string> proxy_data;
  if (Arc::FileRead(fname_src, proxy_data)) {
    std::string user_cert;
    bool in_private=false;
    // TODO: remove private key filtering in a future because job.proxy file will soon contain only public part
    for (std::list<std::string>::iterator line = proxy_data.begin();
         line != proxy_data.end(); ++line) {
      if (in_private) { // Skip private key
        if (line->find("-----END") != std::string::npos &&
            line->find("PRIVATE KEY-----") != std::string::npos) { // can be RSA, DSA etc.
          in_private=false;
        }
      }
      else {
        if (line->find("-----BEGIN") != std::string::npos &&
            line->find("PRIVATE KEY-----") != std::string::npos) { // can be RSA, DSA etc.
          in_private=true;
        }
        else {
          user_cert += *line + '\\';
        }
      }
    }
    if (!user_cert.empty()) job_data += "usercert=" + user_cert + '\n';
  }

  // Extract requested resources
  Arc::JobDescription arc_job_desc;
  std::list<Arc::JobDescription> arc_job_desc_list;
  if (!Arc::JobDescription::Parse(desc, arc_job_desc_list, "", "GRIDMANAGER")
      || arc_job_desc_list.size() != 1) return false;
  arc_job_desc = arc_job_desc_list.front();

  if (arc_job_desc.Resources.IndividualPhysicalMemory.max>=0) {
    job_data += "requestedmemory=" + Arc::tostring(arc_job_desc.Resources.IndividualPhysicalMemory.max) + '\n';
  }
  if (arc_job_desc.Resources.TotalCPUTime.range.max>=0) {
    job_data += "requestedcputime=" + Arc::tostring(arc_job_desc.Resources.TotalCPUTime.range.max) + '\n';
  }
  if (arc_job_desc.Resources.TotalWallTime.range.max>=0) {
    job_data += "requestedwalltime=" + Arc::tostring(arc_job_desc.Resources.TotalWallTime.range.max) + '\n';
  }
  if (arc_job_desc.Resources.DiskSpaceRequirement.DiskSpace.max>=0) {
    job_data += "requesteddisk=" + Arc::tostring((arc_job_desc.Resources.DiskSpaceRequirement.DiskSpace.max*1024*1024)) + '\n';
  }
  if (arc_job_desc.Resources.RunTimeEnvironment.getSoftwareList().size()>0) {
    std::string rteStr;
    for (std::list<Arc::Software>::const_iterator itSW = arc_job_desc.Resources.RunTimeEnvironment.getSoftwareList().begin();
         itSW != arc_job_desc.Resources.RunTimeEnvironment.getSoftwareList().end(); itSW++) {
      if (!itSW->empty() && !itSW->getVersion().empty()) {
        if (!rteStr.empty()) rteStr += " ";
        rteStr += *itSW;
      }
    }
    if (!rteStr.empty()) job_data += "runtimeenvironment=" + rteStr + '\n';
  }

  // Analyze diagnostics and store relevant information
  fname_src = config.ControlDir() + "/job." + job.get_id() + sfx_diag;
  std::list<std::string> diag_data;
  if (Arc::FileRead(fname_src, diag_data)) {
    std::string nodenames;
    int nodecount = 0;
    float cputime = 0;
    for (std::list<std::string>::iterator line = diag_data.begin();
         line != diag_data.end(); ++line) {
      std::string::size_type p = line->find('=');
      if (p == std::string::npos) continue;
      std::string key(Arc::lower(line->substr(0, p)));
      std::string value(line->substr(p+1));
      if (key.empty()) continue;
      if (key == "nodename") {
        if (nodecount) nodenames+=":";
        nodenames+=value;
        nodecount++;
      } else if(key == "processors") {
        job_data += "processors=" + value + '\n';
      } else if(key == "walltime") {
        float f;
        if (string_to_number(value,f)) {
          job_data += "usedwalltime=" + Arc::tostring((unsigned int)f) + '\n';
        }
      } else if(key == "kerneltime") {
        float f;
        if(string_to_number(value,f)) {
          job_data += "usedkernelcputime=" + Arc::tostring((unsigned int)f) + '\n';
          cputime += f;
        }
      } else if(key == "usertime") {
        float f;
        if(string_to_number(value,f)) {
          job_data += "usedusercputime=" + Arc::tostring((unsigned int)f) + '\n';
          cputime += f;
        }
      } else if(key == "averagetotalmemory") {
        float f;
        if(string_to_number(value,f)) {
          job_data += "usedmemory=" + Arc::tostring((unsigned int)f) + '\n';
        }
      } else if(key == "averageresidentmemory") {
        float f;
        if(string_to_number(value,f)) {
          job_data += "usedaverageresident=" + Arc::tostring((unsigned int)f) + '\n';
        }
      } else if(key == "maxresidentmemory") {
        float f;
        if(string_to_number(value,f)) {
          job_data += "usedmaxresident=" + Arc::tostring((unsigned int)f) + '\n';
        }
      } else if(key == "exitcode") {
        int n;
        if(Arc::stringto(value,n)) job_data += "exitcode=" + Arc::tostring(n) + '\n';
      }
    }
    if(nodecount) {
      job_data += "nodename=" + nodenames + '\n';
      job_data += "nodecount=" + Arc::tostring(nodecount) + '\n';
    }
    job_data += "usedcputime=" + Arc::tostring((unsigned int)cputime) + '\n';
  }

  // Endtime and failure reason
  std::string status;
  if (job.get_state() == JOB_STATE_FINISHED) {
    status="completed";
    t = job_state_time(job.get_id(),config);
    if (t == 0) t=::time(NULL);
    job_data += "endtime=" + Arc::Time(t).str(Arc::MDSTime) + '\n';
    if (job_failed_mark_check(job.get_id(),config)) {
      std::string failure = job_failed_mark_read(job.get_id(),config);
      job_data += "failurestring=" + failure + '\n';
      status="failed";
    }
  }
  if(!status.empty()) job_data += "status=" + status + '\n';

  // Read and store statistics information
  fname_src = config.ControlDir() + "/job." + job.get_id() + sfx_statistics;
  std::list<std::string> statistics_data;
  if (Arc::FileRead(fname_src, statistics_data)) {
    for (std::list<std::string>::iterator line = statistics_data.begin();
         line != statistics_data.end(); ++line) {
      // statistics file has : as first delimiter, replace with =
      // todo: change DTRGenerator to use = as first delim for next major release
      std::string::size_type p = line->find(':');
      if (p != std::string::npos)
        line->replace(p, 1, "=");
      job_data += *line+"\n";
    }
  }

  // Store in file
  std::string out_file(config.ControlDir()+"/logs/"+job.get_id()+".XXXXXX");
  if (!Arc::TmpFileCreate(out_file, job_data)) return false;
  fix_file_owner(out_file, job);

  return true;
}

} // namespace ARex
