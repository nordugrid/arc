#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <limits.h>

#include <arc/StringConv.h>
#include "../misc/canonical_dir.h"
#include "../misc/escaped.h"
#include "info_types.h"

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

void output_escaped_string(std::ostream &o,const std::string &str) {
  std::string::size_type n,nn;
  for(nn=0;;) {
//    if((n=str.find(' ',nn)) == std::string::npos) break;
    if((n=str.find_first_of(" \\",nn)) == std::string::npos) break;
    o.write(str.data()+nn,n-nn); 
    o.put('\\');
    o.put(*(str.data()+n));
    nn=n+1;
  };
  o.write(str.data()+nn,str.length()-nn);
}

std::ostream &operator<< (std::ostream &o,const FileData &fd) {
  output_escaped_string(o,fd.pfn);
  o.put(' ');
  output_escaped_string(o,fd.lfn);
  return o;
}

std::istream &operator>> (std::istream &i,FileData &fd) {
  char buf[1024];
  istream_readline(i,buf,sizeof(buf));
  fd.pfn.resize(0); fd.lfn.resize(0);
  int n=input_escaped_string(buf,fd.pfn);
  input_escaped_string(buf+n,fd.lfn);
  if((fd.pfn.length() == 0) && (fd.lfn.length() == 0)) return i; /* empty st */
  if(canonical_dir(fd.pfn) != 0) {
    logger.msg(Arc::ERROR,"Wrong directory in %s",buf);
    fd.pfn.resize(0); fd.lfn.resize(0);
  };
  return i;
}

FileData::FileData(void) {
}

FileData::FileData(const char *pfn_s,const char *lfn_s) {
  if(pfn_s) { pfn=pfn_s; } else { pfn.resize(0); };
  if(lfn_s) { lfn=lfn_s; } else { lfn.resize(0); };
}

FileData& FileData::operator= (const char *str) {
  pfn.resize(0); lfn.resize(0);
  int n=input_escaped_string(str,pfn);
  input_escaped_string(str+n,lfn);
  return *this; 
}

bool FileData::operator== (const FileData& data) {
  // pfn may contain leading slash. It must be striped
  // before comparison.
  const char* pfn_ = pfn.c_str();
  if(pfn_[0] == '/') ++pfn_;
  const char* dpfn_ = data.pfn.c_str();
  if(dpfn_[0] == '/') ++dpfn_;
  return (strcmp(pfn_,dpfn_) == 0);
  // return (pfn == data.pfn);
}

bool FileData::operator== (const char *name) {
  if(name == NULL) return false;
  if(name[0] == '/') ++name;
  const char* pfn_ = pfn.c_str();
  if(pfn_[0] == '/') ++pfn_;
  return (strcmp(pfn_,name) == 0);
}

bool FileData::has_lfn(void) {
  return (lfn.find(':') != std::string::npos);
}

/** Skou: Currently not used, due to transition in job description parsing.
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
  url_.ChangePath(rc_url_.Path()+url_.Path());
  url=url_.str();
  return true;
}
*/

JobLocalDescription& JobLocalDescription::operator=(const Arc::JobDescription& arc_job_desc)
{
  action = "request";

  projectnames.clear();
  projectnames.push_back(arc_job_desc.Identification.JobVOName);

  jobname = arc_job_desc.Identification.JobName;
  downloads = 0;
  uploads = 0;
  rtes = 0;
  outputdata.clear();
  inputdata.clear();
  rte.clear();

  std::list<Arc::SoftwareVersion> rte_names = arc_job_desc.Resources.RunTimeEnvironment.getVersions();
  for (std::list<Arc::SoftwareVersion>::const_iterator rte_name = rte_names.begin(); rte_name != rte_names.end(); ++rte_name) {
    rte.push_back(std::string(*rte_name));
    ++rtes;
  }

  for (std::list<Arc::FileType>::const_iterator file = arc_job_desc.DataStaging.File.begin();
       file != arc_job_desc.DataStaging.File.end(); ++file) {
    std::string fname = file->Name;
    if(fname.empty()) continue; // Can handle only named files
    if(fname[0] != '/') fname = "/"+fname; // Just for safety
    // Because ARC job description does not keep enough information
    // about initial JSDL description we have to make some guesses here.
    if(!file->Source.empty()) { // input file
      // Only one source per file supported
      inputdata.push_back(FileData(fname.c_str(), ""));
      if (file->Source.front().URI &&
          file->Source.front().URI.Protocol() != "file") {
        inputdata.back().lfn = file->Source.front().URI.fullstr();
        ++downloads;
      }

      if (inputdata.back().has_lfn()) {
        Arc::URL u(inputdata.back().lfn);
        
        if (file->IsExecutable ||
            file->Name == arc_job_desc.Application.Executable.Name) {
          u.AddOption("exec", "yes");
        }
        if (u.Option("cache").empty())
          u.AddOption("cache", (file->DownloadToCache ? "yes" : "no"));
        if (u.Option("threads").empty() && file->Source.front().Threads > 1)
          u.AddOption("threads", Arc::tostring(file->Source.front().Threads));
        inputdata.back().lfn = u.fullstr();
      }
    }
    if (!file->Target.empty()) { // output file
      FileData fdata(fname.c_str(), file->Target.front().URI.fullstr().c_str());
      outputdata.push_back(fdata);
      ++uploads;

      if (outputdata.back().has_lfn()) {
        Arc::URL u(outputdata.back().lfn);
        if (u.Option("threads").empty() && file->Target.front().Threads > 1)
          u.AddOption("threads", Arc::tostring(file->Source.front().Threads));
        outputdata.back().lfn = u.fullstr();
      }      
    }
    if (file->Source.empty() && file->Target.empty() && file->KeepData) {
      // user downloadable file
      FileData fdata(fname.c_str(), NULL);
      outputdata.push_back(fdata);
    }
  }
  
/** Skou: Currently not supported, due to transition in job description parsing.
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
*/

  // Order of the following calls matters!
  arguments.clear();
  arguments = arc_job_desc.Application.Executable.Argument;
  arguments.push_front(arc_job_desc.Application.Executable.Name);

  stdin_ = arc_job_desc.Application.Input;
  stdout_ = arc_job_desc.Application.Output;
  stderr_ = arc_job_desc.Application.Error;

  if (arc_job_desc.Resources.IndividualDiskSpace > -1)
    diskspace = (unsigned long long int)arc_job_desc.Resources.IndividualDiskSpace;

  processtime = arc_job_desc.Application.ProcessingStartTime;
  
  const int lifetimeTemp = (int)arc_job_desc.Application.SessionLifeTime.GetPeriod();
  if (lifetimeTemp > 0) lifetime = lifetimeTemp;

  activityid = arc_job_desc.Identification.ActivityOldId;

  stdlog = arc_job_desc.Application.LogDir;

  jobreport.clear();
  for (std::list<Arc::URL>::const_iterator it = arc_job_desc.Application.RemoteLogging.begin();
       it != arc_job_desc.Application.RemoteLogging.end(); it++) {
    jobreport.push_back(it->str());
  }
  
  /** Skou: Currently not supported, due to transition in job description parsing.
   * New notification structure need to be defined.
   * Old parsing structure follows...
  // Notification.
  for( int j=0; (bool)(stateNode[j]); j++ ) {
    std::string value = (std::string) stateNode[i];
    if ( value == "PREPARING" ) {
      s_+="b";
    } else if ( value == "INLRMS" ) {
      s_+="q";
    } else if ( value == "FINISHING" ) {
      s_+="f";
    } else if ( value == "FINISHED" ) {
      s_+="e";
    } else if ( value == "DELETED" ) {
      s_+="d";
    } else if ( value == "CANCELING" ) {
      s_+="c";
    };
  };

  if(s_.length()) {
    s+=s_; s+= (std::string) endpointNode; s+=" ";
  };
  if(!get_notification(job_desc.notify)) return false;
  */
  
  if (!arc_job_desc.Resources.CandidateTarget.empty() &&
      !arc_job_desc.Resources.CandidateTarget.front().QueueName.empty())
    queue = arc_job_desc.Resources.CandidateTarget.front().QueueName;

  if (!arc_job_desc.Application.CredentialService.empty() &&
      arc_job_desc.Application.CredentialService.front())
    credentialserver = arc_job_desc.Application.CredentialService.front().str();

  if (arc_job_desc.Application.Rerun > -1)
    reruns = arc_job_desc.Application.Rerun;
  
  return *this;
};

bool LRMSResult::set(const char* s) {
  // 1. Empty string = exit code 0
  if(s == NULL) s="";
  for(;*s;++s) { if(!isspace(*s)) break; };
  if(!*s) { code_=0; description_=""; };
  // Try to read first word as number
  char* e;
  code_=strtol(s,&e,0);
  if((!*e) || (isspace(*e))) { 
    for(;*e;++e) { if(!isspace(*e)) break; };
    description_=e;
    return true;
  };
  // If there is no number that means some "uncoded" failure
  code_=-1;
  description_=s;
  return true;
}

std::istream& operator>>(std::istream& i,LRMSResult &r) {
  char buf[1025]; // description must have reasonable size
  if(i.eof()) { buf[0]=0; } else { istream_readline(i,buf,sizeof(buf)); };
  r=buf;
  return i;  
}

std::ostream& operator<<(std::ostream& o,const LRMSResult &r) {
  o<<r.code()<<" "<<r.description();
  return o;
}

