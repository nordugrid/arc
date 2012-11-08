#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>

#include <arc/StringConv.h>
#include <arc/FileUtils.h>

#include "../misc/escaped.h"
#include "ControlFileContent.h"

namespace ARex {

static Glib::Mutex local_lock;
static Arc::Logger& logger = Arc::Logger::getRootLogger();

static inline void write_str(int f,const char* buf, std::string::size_type len) {
  for(;len > 0;) {
    ssize_t l = write(f,buf,len);
    if((l < 0) && (errno != EINTR)) break;
    len -= l; buf += l;
  };
}

static inline void write_str(int f,const std::string& str) {
  write_str(f,str.c_str(),str.length());
}

static inline bool read_str(int f,char* buf,int size) {
  char c;
  int pos = 0;
  for(;;) {
    ssize_t l = read(f,&c,1);
    if((l == -1) && (errno == EINTR)) continue;
    if(l < 0) return false;
    if(l == 0) {
      if(!pos) return false;
      break;
    };
    if(c == '\n') break;
    if(pos < (size-1)) {
      buf[pos] = c;
      ++pos;
      buf[pos] = 0;
    } else {
      ++pos;
    };
  };
  return true;
}

std::ostream &operator<< (std::ostream &o,const FileData &fd) {
  std::string escaped_pfn(Arc::escape_chars(fd.pfn, " \\\r\n", '\\', false));
  o.write(escaped_pfn.c_str(), escaped_pfn.size());
  o.put(' ');
  std::string escaped_lfn(Arc::escape_chars(fd.lfn, " \\\r\n", '\\', false));
  o.write(escaped_lfn.c_str(), escaped_lfn.size());
  if(fd.lfn.empty() || fd.cred.empty()) return o;
  o.put(' ');
  std::string escaped_cred(Arc::escape_chars(fd.cred, " \\\r\n", '\\', false));
  o.write(escaped_cred.c_str(), escaped_cred.size());
  return o;
}

std::istream &operator>> (std::istream &i,FileData &fd) {
  std::string buf;
  std::getline(i,buf);
  Arc::trim(buf," \t\r\n");
  fd.pfn.resize(0); fd.lfn.resize(0); fd.cred.resize(0);
  int n=input_escaped_string(buf.c_str(),fd.pfn);
  n+=input_escaped_string(buf.c_str()+n,fd.lfn);
  n+=input_escaped_string(buf.c_str()+n,fd.cred);
  if((fd.pfn.length() == 0) && (fd.lfn.length() == 0)) return i; /* empty st */
  if(!Arc::CanonicalDir(fd.pfn)) {
    logger.msg(Arc::ERROR,"Wrong directory in %s",buf);
    fd.pfn.resize(0); fd.lfn.resize(0);
  };
  return i;
}

FileData::FileData(void) {
  ifsuccess = true;
  ifcancel = false;
  iffailure = false;
}

FileData::FileData(const std::string& pfn_s,const std::string& lfn_s) {
  ifsuccess = true;
  ifcancel = false;
  iffailure = false;
  if(!pfn_s.empty()) { pfn=pfn_s; } else { pfn.resize(0); };
  if(!lfn_s.empty()) { lfn=lfn_s; } else { lfn.resize(0); };
}

//FileData& FileData::operator= (const char *str) {
//  pfn.resize(0); lfn.resize(0);
//  int n=input_escaped_string(str,pfn);
//  input_escaped_string(str+n,lfn);
//  return *this;
//}

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


static char StateToShortcut(const std::string& state) {
  if(state == "ACCEPTED") return 'a'; // not supported
  if(state == "PREPARING") return 'b';
  if(state == "SUBMIT") return 's'; // not supported
  if(state == "INLRMS") return 'q';
  if(state == "FINISHING") return 'f';
  if(state == "FINISHED") return 'e';
  if(state == "DELETED") return 'd';
  if(state == "CANCELING") return 'c';
  return ' ';
}

Exec& Exec::operator=(const Arc::ExecutableType& src) {
  Exec& dest = *this;
  // Order of the following calls matters!
  dest.clear(); dest.successcode = 0;
  dest = src.Argument;
  dest.push_front(src.Path);
  if(src.SuccessExitCode.first) dest.successcode = src.SuccessExitCode.second;
  return dest;
}

JobLocalDescription& JobLocalDescription::operator=(const Arc::JobDescription& arc_job_desc) {
  // TODO: handle errors
  action = "request";
  std::map<std::string, std::string>::const_iterator act_i = arc_job_desc.OtherAttributes.find("nordugrid:xrsl;action");
  if(act_i != arc_job_desc.OtherAttributes.end()) action = act_i->second;
  std::map<std::string, std::string>::const_iterator jid_i = arc_job_desc.OtherAttributes.find("nordugrid:xrsl;jobid");
  if(jid_i != arc_job_desc.OtherAttributes.end()) jobid = jid_i->second;

  dryrun = arc_job_desc.Application.DryRun;

  projectnames.clear();
  std::map<std::string, std::string>::const_iterator jr_i = arc_job_desc.OtherAttributes.find("nordugrid:jsdl;Identification/JobProject");
  if (jr_i != arc_job_desc.OtherAttributes.end()) projectnames.push_back(jr_i->second);

  jobname = arc_job_desc.Identification.JobName;
  downloads = 0;
  uploads = 0;
  freestagein = false;
  outputdata.clear();
  inputdata.clear();
  rte.clear();
  transfershare="_default";

  const std::list<Arc::Software>& sw = arc_job_desc.Resources.RunTimeEnvironment.getSoftwareList();
  for (std::list<Arc::Software>::const_iterator itSW = sw.begin(); itSW != sw.end(); ++itSW)
    rte.push_back(std::string(*itSW));

  for (std::list<Arc::InputFileType>::const_iterator file = arc_job_desc.DataStaging.InputFiles.begin();
       file != arc_job_desc.DataStaging.InputFiles.end(); ++file) {
    std::string fname = file->Name;
    if(fname[0] != '/') fname = "/"+fname; // Just for safety
    inputdata.push_back(FileData(fname, ""));
    if(!file->Sources.empty()) {
      // Only one source per file is used
      if (file->Sources.front() &&
          file->Sources.front().Protocol() != "file") {
        inputdata.back().lfn = file->Sources.front().fullstr();
        // It is not possible to extract credentials path here.
        // So temporarily storing id here.
        inputdata.back().cred = file->Sources.front().DelegationID;
      }
    }

    if(fname == "/") {
      // Unnamed file is used to mark request for free stage in
      freestagein = true;
    }
    if (inputdata.back().has_lfn()) {
      ++downloads;
      Arc::URL u(inputdata.back().lfn);

      if (file->IsExecutable ||
          file->Name == arc_job_desc.Application.Executable.Path) {
        u.AddOption("exec", "yes", true);
      }
      inputdata.back().lfn = u.fullstr();
    } else if (file->FileSize != -1) {
      inputdata.back().lfn = Arc::tostring(file->FileSize);
      if (!file->Checksum.empty()) { // Only set checksum if FileSize is also set.
        inputdata.back().lfn += "."+file->Checksum;
      }
    }
  }
  for (std::list<Arc::OutputFileType>::const_iterator file = arc_job_desc.DataStaging.OutputFiles.begin();
       file != arc_job_desc.DataStaging.OutputFiles.end(); ++file) {
    std::string fname = file->Name;
    if(fname[0] != '/') fname = "/"+fname; // Just for safety
    bool ifsuccess = false;
    bool ifcancel = false;
    bool iffailure = false;
    if (!file->Targets.empty()) { // output file
      for(std::list<Arc::TargetType>::const_iterator target = file->Targets.begin();
                              target != file->Targets.end(); ++target) {
        FileData fdata(fname, target->fullstr());
        fdata.ifsuccess = target->UseIfSuccess;
        fdata.ifcancel = target->UseIfCancel;
        fdata.iffailure = target->UseIfFailure;
        outputdata.push_back(fdata);

        if (outputdata.back().has_lfn()) {
          ++uploads;
          Arc::URL u(outputdata.back().lfn); // really needed?
          if(u.Option("preserve","no") == "yes") {
            outputdata.back().ifcancel = true;
            outputdata.back().iffailure = true;
          };
          switch(target->CreationFlag) {
            case Arc::TargetType::CFE_OVERWRITE: u.AddOption("overwrite","yes",true); break;
            case Arc::TargetType::CFE_DONTOVERWRITE: u.AddOption("overwrite","no",true); break;
            // Rest is not supported in URLs yet.
            default: break;
          };
          u.RemoveOption("preserve");
          u.RemoveOption("mandatory"); // TODO: implement
          outputdata.back().lfn = u.fullstr();
          // It is not possible to extract credentials path here.
          // So temporarily storing id here.
          outputdata.back().cred = target->DelegationID;
        }
        if(outputdata.back().ifsuccess) ifsuccess = true;
        if(outputdata.back().ifcancel) ifcancel = true;
        if(outputdata.back().iffailure) iffailure = true;
      }
      if(ifsuccess && ifcancel && iffailure) {
        // All possible results are covered
      } else {
        // For not covered cases file is treated as user downloadable
        FileData fdata(fname, "");
        fdata.ifsuccess = !ifsuccess;
        fdata.ifcancel = !ifcancel;
        fdata.iffailure = !iffailure;
        outputdata.push_back(fdata);
      }
    }
    else {
      // user downloadable file
      FileData fdata(fname, "");
      // user decides either to use file
      fdata.ifsuccess = true;
      fdata.ifcancel = true;
      fdata.iffailure = true;
      outputdata.push_back(fdata);
    }
  }

  exec = arc_job_desc.Application.Executable;
  for(std::list<Arc::ExecutableType>::const_iterator e = arc_job_desc.Application.PreExecutable.begin();
               e != arc_job_desc.Application.PreExecutable.end(); ++e) {
    Exec pe = *e;
    preexecs.push_back(pe);
  }
  for(std::list<Arc::ExecutableType>::const_iterator e = arc_job_desc.Application.PostExecutable.begin();
               e != arc_job_desc.Application.PostExecutable.end(); ++e) {
    Exec pe = *e;
    postexecs.push_back(pe);
  }

  stdin_ = arc_job_desc.Application.Input;
  stdout_ = arc_job_desc.Application.Output;
  stderr_ = arc_job_desc.Application.Error;

  if (arc_job_desc.Resources.DiskSpaceRequirement.DiskSpace > -1)
    diskspace = (unsigned long long int)(arc_job_desc.Resources.DiskSpaceRequirement.DiskSpace*1024*1024);

  processtime = arc_job_desc.Application.ProcessingStartTime;

  const int lifetimeTemp = (int)arc_job_desc.Resources.SessionLifeTime.GetPeriod();
  if (lifetimeTemp > 0) lifetime = lifetimeTemp;

  activityid = arc_job_desc.Identification.ActivityOldID;

  stdlog = arc_job_desc.Application.LogDir;

  jobreport.clear();
  for (std::list<Arc::RemoteLoggingType>::const_iterator it = arc_job_desc.Application.RemoteLogging.begin();
       it != arc_job_desc.Application.RemoteLogging.end(); ++it) {
    // TODO: Support optional requirement.
    // TODO: Support other types than SGAS.
    if (it->ServiceType == "SGAS") {
      jobreport.push_back(it->Location.str());
    }
  }

  notify.clear();
  {
    int n = 0;
    for (std::list<Arc::NotificationType>::const_iterator it = arc_job_desc.Application.Notification.begin();
         it != arc_job_desc.Application.Notification.end(); it++) {
      if (n >= 3) break; // Only 3 instances are allowed.
      std::string states;
      for (std::list<std::string>::const_iterator s = it->States.begin();
           s != it->States.end(); ++s) {
        char state = StateToShortcut(*s);
        if(state == ' ') continue;
        states+=state;
      }
      if(states.empty()) continue;
      if(it->Email.empty()) continue;
      if (!notify.empty()) notify += " ";
      notify += states + " " + it->Email;
      ++n;
    }
  }

  if (!arc_job_desc.Resources.QueueName.empty()) {
    queue = arc_job_desc.Resources.QueueName;
  }

  if (!arc_job_desc.Application.CredentialService.empty() &&
      arc_job_desc.Application.CredentialService.front())
    credentialserver = arc_job_desc.Application.CredentialService.front().fullstr();

  if (arc_job_desc.Application.Rerun > -1)
    reruns = arc_job_desc.Application.Rerun;

  if ( arc_job_desc.Application.Priority <= 100 && arc_job_desc.Application.Priority > 0 )
    priority = arc_job_desc.Application.Priority;

  return *this;
}

const char* const JobLocalDescription::transfersharedefault = "_default";

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
  std::string buf;
  if(i.eof() || i.fail()) {
  } else {
    std::getline(i,buf);
  };
  r=buf;
  return i;
}

std::ostream& operator<<(std::ostream& o,const LRMSResult &r) {
  o<<r.code()<<" "<<r.description();
  return o;
}


static inline void write_pair(int f,const std::string &name,const std::string &value) {
  if(value.length() <= 0) return;
  write_str(f,name);
  write_str(f,"=");
  write_str(f,value);
  write_str(f,"\n");
}

static inline void write_pair(int f,const std::string &name,const Arc::Time &value) {
  if(value == -1) return;
  write_str(f,name);
  write_str(f,"=");
  write_str(f,value.str(Arc::MDSTime));
  write_str(f,"\n");
}

static inline void write_pair(int f,const std::string &name,bool value) {
  write_str(f,name);
  write_str(f,"=");
  write_str(f,(value?"yes":"no"));
  write_str(f,"\n");
}

static void write_pair(int f,const std::string &name,const Exec& value) {
  write_str(f,name);
  write_str(f,"=");
  for(Exec::const_iterator i=value.begin(); i!=value.end(); ++i) {
    write_str(f, Arc::escape_chars(*i, " \\\r\n", '\\', false));
    write_str(f," ");
  }
  write_str(f,"\n");
  write_str(f,name+"code");
  write_str(f,"=");
  write_str(f,Arc::tostring(value.successcode));
  write_str(f,"\n");
}

static void write_pair(int f,const std::string &name,const std::list<Exec>& value) {
  for(std::list<Exec>::const_iterator v = value.begin();
                 v != value.end(); ++v) {
    write_pair(f,name,*v);
  }
}

static inline bool read_boolean(const char* buf) {
  if(strncasecmp("yes",buf,3) == 0) return true;
  if(strncasecmp("true",buf,4) == 0) return true;
  if(strncmp("1",buf,1) == 0) return true;
  return false;
}

bool JobLocalDescription::write(const std::string& fname) const {
  Glib::Mutex::Lock lock_(local_lock);
  // *.local file is accessed concurently. To avoid improper readings lock is acquired.
  int f=open(fname.c_str(),O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
  if(f==-1) return false;
  struct flock lock;
  lock.l_type=F_WRLCK; lock.l_whence=SEEK_SET; lock.l_start=0; lock.l_len=0;
  for(;;) {
    if(fcntl(f,F_SETLKW,&lock) != -1) break;
    if(errno == EINTR) continue;
    close(f); return false;
  };
  if(ftruncate(f,0) != 0) { close(f); return false; };
  if(lseek(f,0,SEEK_SET) != 0) { close(f); return false; };
  for (std::list<std::string>::const_iterator it=jobreport.begin();
       it!=jobreport.end();
       it++) {
    write_pair(f,"jobreport",*it);
  };
  write_pair(f,"globalid",globalid);
  write_pair(f,"headnode",headnode);
  write_pair(f,"interface",interface);
  write_pair(f,"lrms",lrms);
  write_pair(f,"queue",queue);
  write_pair(f,"localid",localid);
  write_pair(f,"args",exec);
  write_pair(f,"pre",preexecs);
  write_pair(f,"post",postexecs);
  write_pair(f,"subject",DN);
  write_pair(f,"starttime",starttime);
  write_pair(f,"lifetime",lifetime);
  write_pair(f,"notify",notify);
  write_pair(f,"processtime",processtime);
  write_pair(f,"exectime",exectime);
  write_pair(f,"rerun",Arc::tostring(reruns));
  if(downloads>=0) write_pair(f,"downloads",Arc::tostring(downloads));
  if(uploads>=0) write_pair(f,"uploads",Arc::tostring(uploads));
  write_pair(f,"jobname",jobname);
  for (std::list<std::string>::const_iterator ppn=projectnames.begin();
       ppn!=projectnames.end();
       ++ppn) {
    write_pair(f,"projectname",*ppn);
  };
  write_pair(f,"gmlog",stdlog);
  write_pair(f,"cleanuptime",cleanuptime);
  write_pair(f,"delegexpiretime",expiretime);
  write_pair(f,"clientname",clientname);
  write_pair(f,"clientsoftware",clientsoftware);
  write_pair(f,"sessiondir",sessiondir);
  write_pair(f,"diskspace",Arc::tostring(diskspace));
  write_pair(f,"failedstate",failedstate);
  write_pair(f,"failedcause",failedcause);
  write_pair(f,"credentialserver",credentialserver);
  write_pair(f,"freestagein",freestagein);
  for(std::list<std::string>::const_iterator act_id=activityid.begin();
      act_id != activityid.end(); ++act_id) {
    write_pair(f,"activityid",(*act_id));
  };
  if (migrateactivityid != "") {
    write_pair(f,"migrateactivityid",migrateactivityid);
    write_pair(f,"forcemigration",forcemigration);
  }
  write_pair(f,"transfershare",transfershare);
  write_pair(f,"priority",Arc::tostring(priority));
  close(f);
  return true;
}

bool JobLocalDescription::read(const std::string& fname) {
  Glib::Mutex::Lock lock_(local_lock);
  // *.local file is accessed concurently. To avoid improper readings lock is acquired.
  int f=open(fname.c_str(),O_RDONLY);
  if(f==-1) return false;
  struct flock lock;
  lock.l_type=F_RDLCK; lock.l_whence=SEEK_SET; lock.l_start=0; lock.l_len=0;
  for(;;) {
    if(fcntl(f,F_SETLKW,&lock) != -1) break;
    if(errno == EINTR) continue;
    close(f); return false;
  };
  // using iostream for handling file content
  char buf[4096];
  std::string name;
  activityid.clear();
  for(;;) {
    if(!read_str(f,buf,sizeof(buf))) break;;
    name.erase();
    int p=input_escaped_string(buf,name,'=');
    if(name.length() == 0) continue;
    if(buf[p] == 0) continue;
    if(name == "lrms") { lrms = buf+p; }
    else if(name == "headnode") { headnode = buf+p; }
    else if(name == "interface") { interface = buf+p; }
    else if(name == "queue") { queue = buf+p; }
    else if(name == "localid") { localid = buf+p; }
    else if(name == "subject") { DN = buf+p; }
    else if(name == "starttime") { starttime = buf+p; }
//    else if(name == "UI") { UI = buf+p; }
    else if(name == "lifetime") { lifetime = buf+p; }
    else if(name == "notify") { notify = buf+p; }
    else if(name == "processtime") { processtime = buf+p; }
    else if(name == "exectime") { exectime = buf+p; }
    else if(name == "jobreport") { jobreport.push_back(std::string(buf+p)); }
    else if(name == "globalid") { globalid = buf+p; }
    else if(name == "jobname") { jobname = buf+p; }
    else if(name == "projectname") { projectnames.push_back(std::string(buf+p)); }
    else if(name == "gmlog") { stdlog = buf+p; }
    else if(name == "rerun") {
      std::string temp_s(buf+p); int n;
      if(!Arc::stringto(temp_s,n)) { close(f); return false; };
      reruns = n;
    }
    else if(name == "downloads") {
      std::string temp_s(buf+p); int n;
      if(!Arc::stringto(temp_s,n)) { close(f); return false; };
      downloads = n;
    }
    else if(name == "uploads") {
      std::string temp_s(buf+p); int n;
      if(!Arc::stringto(temp_s,n)) { close(f); return false; };
      uploads = n;
    }
    else if(name == "args") {
      exec.clear(); exec.successcode = 0;
      for(int n=p;buf[n] != 0;) {
        std::string arg;
        n+=input_escaped_string(buf+n,arg);
        exec.push_back(arg);
      };
    }
    else if(name == "argscode") {
      std::string temp_s(buf+p); int n;
      if(!Arc::stringto(temp_s,n)) { close(f); return false; };
      exec.successcode = n;
    }
    else if(name == "pre") {
      Exec pe;
      for(int n=p;buf[n] != 0;) {
        std::string arg;
        n+=input_escaped_string(buf+n,arg);
        pe.push_back(arg);
      };
      preexecs.push_back(pe);
    }
    else if(name == "precode") {
      if(preexecs.empty()) { close(f); return false; };
      std::string temp_s(buf+p); int n;
      if(!Arc::stringto(temp_s,n)) { close(f); return false; };
      preexecs.back().successcode = n;
    }
    else if(name == "post") {
      Exec pe;
      for(int n=p;buf[n] != 0;) {
        std::string arg;
        n+=input_escaped_string(buf+n,arg);
        pe.push_back(arg);
      };
      postexecs.push_back(pe);
    }
    else if(name == "postcode") {
      if(postexecs.empty()) { close(f); return false; };
      std::string temp_s(buf+p); int n;
      if(!Arc::stringto(temp_s,n)) { close(f); return false; };
      postexecs.back().successcode = n;
    }
    else if(name == "cleanuptime") { cleanuptime = buf+p; }
    else if(name == "delegexpiretime") { expiretime = buf+p; }
    else if(name == "clientname") { clientname = buf+p; }
    else if(name == "clientsoftware") { clientsoftware = buf+p; }
    else if(name == "sessiondir") { sessiondir = buf+p; }
    else if(name == "failedstate") { failedstate = buf+p; }
    else if(name == "failedcause") { failedcause = buf+p; }
    else if(name == "credentialserver") { credentialserver = buf+p; }
    else if(name == "freestagein") { freestagein = read_boolean(buf+p); }
    else if(name == "diskspace") {
      std::string temp_s(buf+p);
      unsigned long long int n;
      if(!Arc::stringto(temp_s,n)) { close(f); return false; };
      diskspace = n;
    }
    else if(name == "activityid") {
      std::string temp_s(buf+p);
      activityid.push_back(temp_s);
    }
    else if(name == "migrateactivityid") { migrateactivityid = buf+p; }
    else if(name == "forcemigration") { forcemigration = read_boolean(buf+p); }
    else if(name == "transfershare") { transfershare = buf+p; }
    else if(name == "priority") {
      std::string temp_s(buf+p); int n;
      if(!Arc::stringto(temp_s,n)) { close(f); return false; };
      priority = n;
    }
  }
  close(f);
  return true;
}

bool JobLocalDescription::read_var(const std::string &fname,const std::string &vnam,std::string &value) {
  Glib::Mutex::Lock lock_(local_lock);
  // *.local file is accessed concurently. To avoid improper readings lock is acquired.
  int f=open(fname.c_str(),O_RDONLY);
  if(f==-1) return false;
  struct flock lock;
  lock.l_type=F_RDLCK; lock.l_whence=SEEK_SET; lock.l_start=0; lock.l_len=0;
  for(;;) {
    if(fcntl(f,F_SETLKW,&lock) != -1) break;
    if(errno == EINTR) continue;
    close(f); return false;
  };
  // using iostream for handling file content
  char buf[1024];
  std::string name;
  bool found = false;
  for(;;) {
    if(!read_str(f,buf,sizeof(buf))) break;
    name.erase();
    int p=input_escaped_string(buf,name,'=');
    if(name.length() == 0) continue;
    if(buf[p] == 0) continue;
    if(name == vnam) { value = buf+p; found=true; break; };
  };
  close(f);
  return found;
}

} // namespace ARex
