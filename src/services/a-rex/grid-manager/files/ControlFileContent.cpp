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
#include <arc/ArcConfigIni.h>

#include "ControlFileContent.h"

namespace ARex {

static Glib::Mutex local_lock;
static Arc::Logger& logger = Arc::Logger::getRootLogger();

class KeyValueFile {
 public:
  enum OpenMode {
    Fetch,
    Create
  };
  KeyValueFile(std::string const& fname, OpenMode mode);
  ~KeyValueFile(void);
  operator bool(void) { return handle_ != -1; };
  bool operator!(void) { return handle_ == -1; };
  bool Write(std::string const& name, std::string const& value);
  bool Read(std::string& name, std::string& value);
 private:
  int handle_;
  char* read_buf_;
  int read_buf_pos_;
  int read_buf_avail_;
  static int const read_buf_size_ = 256; // normally should fit full line
  static int const data_max_ = 1024*1024; // sanity protection
};

KeyValueFile::KeyValueFile(std::string const& fname, OpenMode mode):
          handle_(-1),read_buf_(NULL),read_buf_pos_(0),read_buf_avail_(0) {
  if(mode == Create) {
    handle_ = ::open(fname.c_str(),O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if(handle_==-1) return;
    struct flock lock;
    lock.l_type=F_WRLCK; lock.l_whence=SEEK_SET; lock.l_start=0; lock.l_len=0;
    for(;;) {
      if(::fcntl(handle_,F_SETLKW,&lock) != -1) break;
      if(errno == EINTR) continue;
      ::close(handle_); handle_ = -1;
      return;
    };
    if((::ftruncate(handle_,0) != 0) || (::lseek(handle_,0,SEEK_SET) != 0)) {
      close(handle_); handle_ = -1;
      return;
    };
  } else {
    handle_ = ::open(fname.c_str(),O_RDONLY);
    if(handle_ == -1) return;
    struct flock lock;
    lock.l_type=F_RDLCK; lock.l_whence=SEEK_SET; lock.l_start=0; lock.l_len=0;
    for(;;) {
      if(::fcntl(handle_,F_SETLKW,&lock) != -1) break; // success
      if(errno == EINTR) continue; // retry
      close(handle_); handle_ = -1; // failure
      return;
    };
    read_buf_ = new char[read_buf_size_];
    if(!read_buf_) {
      close(handle_); handle_ = -1;
      return;
    };
  };
}

KeyValueFile::~KeyValueFile(void) {
  if(handle_ != -1) ::close(handle_);
  if(read_buf_) delete[] read_buf_;
}

static inline bool write_str(int f,const char* buf, std::string::size_type len) {
  for(;len > 0;) {
    ssize_t l = write(f,buf,len);
    if(l < 0) {
      if(errno != EINTR) return false;
    } else {
      len -= l; buf += l;
    };
  };
  return true;
}

bool KeyValueFile::Write(std::string const& name, std::string const& value) {
  if(handle_ == -1) return false;
  if(read_buf_) return false;
  if(name.empty()) return false;
  if(name.length() > data_max_) return false;
  if(value.length() > data_max_) return false;
  if(!write_str(handle_, name.c_str(), name.length())) return false;
  if(!write_str(handle_, "=", 1)) return false;
  if(!write_str(handle_, value.c_str(), value.length())) return false;
  if(!write_str(handle_, "\n", 1)) return false;
  return true;
}

bool KeyValueFile::Read(std::string& name, std::string& value) {
  if(handle_ == -1) return false;
  if(!read_buf_) return false;
  name.clear();
  value.clear();
  char c;
  bool key_done = false;
  for(;;) {
    if(read_buf_pos_ >= read_buf_avail_) {
      read_buf_pos_ = 0;
      read_buf_avail_ = 0;
      ssize_t l = ::read(handle_, read_buf_, read_buf_size_);
      if(l < 0) {
        if(errno == EINTR) continue;
        return false;
      };
      if(l == 0) break; // EOF - not error
      read_buf_avail_ = l;
    };
    c = read_buf_[read_buf_pos_++];
    if(c == '\n') break; // EOL
    if(!key_done) {
      if(c == '=') {
        key_done = true;
      } else {
        name += c;
        if(name.length() > data_max_) return false;
      };
    } else {
      value += c;
      if(value.length() > data_max_) return false; 
    };
  };
  return true;
}

std::ostream &operator<< (std::ostream &o,const FileData &fd) {
  // TODO: switch to HEX encoding and drop dependency on ConfigIni in major release
  std::string escaped_pfn(Arc::escape_chars(fd.pfn, " \\\r\n", '\\', false));
  if(!escaped_pfn.empty()) {
    o.write(escaped_pfn.c_str(), escaped_pfn.size());
    std::string escaped_lfn(Arc::escape_chars(fd.lfn, " \\\r\n", '\\', false));
    if(!escaped_lfn.empty()) {
      o.put(' ');
      o.write(escaped_lfn.c_str(), escaped_lfn.size());
      std::string escaped_cred(Arc::escape_chars(fd.cred, " \\\r\n", '\\', false));
      if(!escaped_cred.empty()) {
        o.put(' ');
        o.write(escaped_cred.c_str(), escaped_cred.size());
      };
    };
  };
  return o;
}

std::istream &operator>> (std::istream &i,FileData &fd) {
  std::string buf;
  std::getline(i,buf);
  Arc::trim(buf," \t\r\n");
  fd.pfn.resize(0); fd.lfn.resize(0); fd.cred.resize(0);
  fd.pfn  = Arc::unescape_chars(Arc::extract_escaped_token(buf, ' ', '\\'), '\\');
  fd.lfn  = Arc::unescape_chars(Arc::extract_escaped_token(buf, ' ', '\\'), '\\');
  fd.cred = Arc::unescape_chars(Arc::extract_escaped_token(buf, ' ', '\\'), '\\');
  if((fd.pfn.length() == 0) && (fd.lfn.length() == 0)) return i; /* empty st */
  if(!Arc::CanonicalDir(fd.pfn,true,true)) {
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
  // Pick up per job delegation
  if(!arc_job_desc.DataStaging.DelegationID.empty()) {
    delegationid = arc_job_desc.DataStaging.DelegationID;
  };

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
int const JobLocalDescription::prioritydefault = 50;

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


static inline bool write_pair(KeyValueFile& f,const std::string &name,const std::string &value) {
  if(value.empty()) return true;
  return f.Write(name,value);
}

static inline bool write_pair(KeyValueFile& f,const std::string &name,const Arc::Time &value) {
  if(value == -1) return true;
  return f.Write(name,value.str(Arc::MDSTime));
}

static inline bool write_pair(KeyValueFile& f,const std::string &name,bool value) {
  return f.Write(name,(value?"yes":"no"));
}

static bool write_pair(KeyValueFile& f,const std::string &name,const Exec& value) {
  std::string value_str;
  for(Exec::const_iterator i=value.begin(); i!=value.end(); ++i) {
    value_str += Arc::escape_chars(*i, " \\\r\n", '\\', false);
    value_str += " ";
  }
  if(!f.Write(name,value_str)) return false;
  if(!f.Write(name+"code",Arc::tostring(value.successcode))) return false;
  return true;
}

static bool write_pair(KeyValueFile& f,const std::string &name,const std::list<Exec>& value) {
  for(std::list<Exec>::const_iterator v = value.begin();
                 v != value.end(); ++v) {
    if(!write_pair(f,name,*v)) return false;
  }
  return true;
}

static inline bool parse_boolean(const std::string& buf) {
  if(strncasecmp("yes",buf.c_str(),3) == 0) return true;
  if(strncasecmp("true",buf.c_str(),4) == 0) return true;
  if(strncmp("1",buf.c_str(),1) == 0) return true;
  return false;
}

bool JobLocalDescription::write(const std::string& fname) const {
  Glib::Mutex::Lock lock_(local_lock);
  // *.local file is accessed concurently. To avoid improper readings lock is acquired.
  KeyValueFile f(fname,KeyValueFile::Create);
  if(!f) return false;
  for (std::list<std::string>::const_iterator it=jobreport.begin();
       it!=jobreport.end();
       it++) {
    if(!write_pair(f,"jobreport",*it)) return false;
  };
  if(!write_pair(f,"globalid",globalid)) return false;
  if(!write_pair(f,"headnode",headnode)) return false;
  if(!write_pair(f,"interface",interface)) return false;
  if(!write_pair(f,"lrms",lrms)) return false;
  if(!write_pair(f,"queue",queue)) return false;
  if(!write_pair(f,"localid",localid)) return false;
  if(!write_pair(f,"args",exec)) return false;
  if(!write_pair(f,"pre",preexecs)) return false;
  if(!write_pair(f,"post",postexecs)) return false;
  if(!write_pair(f,"subject",DN)) return false;
  if(!write_pair(f,"starttime",starttime)) return false;
  if(!write_pair(f,"lifetime",lifetime)) return false;
  if(!write_pair(f,"notify",notify)) return false;
  if(!write_pair(f,"processtime",processtime)) return false;
  if(!write_pair(f,"exectime",exectime)) return false;
  if(!write_pair(f,"rerun",Arc::tostring(reruns))) return false;
  if(downloads>=0) if(!write_pair(f,"downloads",Arc::tostring(downloads))) return false;
  if(uploads>=0) if(!write_pair(f,"uploads",Arc::tostring(uploads))) return false;
  if(!write_pair(f,"jobname",jobname)) return false;
  for (std::list<std::string>::const_iterator ppn=projectnames.begin();
       ppn!=projectnames.end();
       ++ppn) {
    if(!write_pair(f,"projectname",*ppn)) return false;
  };
  if(!write_pair(f,"gmlog",stdlog)) return false;
  if(!write_pair(f,"cleanuptime",cleanuptime)) return false;
  if(!write_pair(f,"delegexpiretime",expiretime)) return false;
  if(!write_pair(f,"clientname",clientname)) return false;
  if(!write_pair(f,"clientsoftware",clientsoftware)) return false;
  if(!write_pair(f,"delegationid",delegationid)) return false;
  if(!write_pair(f,"sessiondir",sessiondir)) return false;
  if(!write_pair(f,"diskspace",Arc::tostring(diskspace))) return false;
  if(!write_pair(f,"failedstate",failedstate)) return false;
  if(!write_pair(f,"failedcause",failedcause)) return false;
  if(!write_pair(f,"credentialserver",credentialserver)) return false;
  if(!write_pair(f,"freestagein",freestagein)) return false;
  for(std::list<std::string>::const_iterator lv=localvo.begin();
      lv != localvo.end(); ++lv) {
    if(!write_pair(f,"localvo",(*lv))) return false;
  };
  for(std::list<std::string>::const_iterator vf=voms.begin();
      vf != voms.end(); ++vf) {
    if(!write_pair(f,"voms",(*vf))) return false;
  };
  for(std::list<std::string>::const_iterator act_id=activityid.begin();
      act_id != activityid.end(); ++act_id) {
    if(!write_pair(f,"activityid",(*act_id))) return false;
  };
  if (migrateactivityid != "") {
    if(!write_pair(f,"migrateactivityid",migrateactivityid)) return false;
    if(!write_pair(f,"forcemigration",forcemigration)) return false;
  }
  if(!write_pair(f,"transfershare",transfershare)) return false;
  if(!write_pair(f,"priority",Arc::tostring(priority))) return false;
  return true;
}

bool JobLocalDescription::read(const std::string& fname) {
  Glib::Mutex::Lock lock_(local_lock);
  // *.local file is accessed concurently. To avoid improper readings lock is acquired.
  KeyValueFile f(fname,KeyValueFile::Fetch);
  if(!f) return false;
  activityid.clear();
  localvo.clear();
  voms.clear();
  for(;;) {
    std::string name;
    std::string buf;
    if(!f.Read(name,buf)) return false;
    if(name.empty() && buf.empty()) break; // EOF
    if(name.empty()) continue;
    if(buf.empty()) continue;
    if(name == "lrms") { lrms = buf; }
    else if(name == "headnode") { headnode = buf; }
    else if(name == "interface") { interface = buf; }
    else if(name == "queue") { queue = buf; }
    else if(name == "localid") { localid = buf; }
    else if(name == "subject") { DN = buf; }
    else if(name == "starttime") { starttime = buf; }
//    else if(name == "UI") { UI = buf; }
    else if(name == "lifetime") { lifetime = buf; }
    else if(name == "notify") { notify = buf; }
    else if(name == "processtime") { processtime = buf; }
    else if(name == "exectime") { exectime = buf; }
    else if(name == "jobreport") { jobreport.push_back(std::string(buf)); }
    else if(name == "globalid") { globalid = buf; }
    else if(name == "jobname") { jobname = buf; }
    else if(name == "projectname") { projectnames.push_back(std::string(buf)); }
    else if(name == "gmlog") { stdlog = buf; }
    else if(name == "rerun") {
      int n;
      if(!Arc::stringto(buf,n)) return false;
      reruns = n;
    }
    else if(name == "downloads") {
      int n;
      if(!Arc::stringto(buf,n)) return false;
      downloads = n;
    }
    else if(name == "uploads") {
      int n;
      if(!Arc::stringto(buf,n)) return false;
      uploads = n;
    }
    else if(name == "args") {
      exec.clear(); exec.successcode = 0;
      while(!buf.empty()) {
        std::string arg;
        arg = Arc::unescape_chars(Arc::extract_escaped_token(buf, ' ', '\\'), '\\');
        exec.push_back(arg);
      };
    }
    else if(name == "argscode") {
      int n;
      if(!Arc::stringto(buf,n)) return false;
      exec.successcode = n;
    }
    else if(name == "pre") {
      Exec pe;
      while(!buf.empty()) {
        std::string arg;
        arg = Arc::unescape_chars(Arc::extract_escaped_token(buf, ' ', '\\'), '\\');
        pe.push_back(arg);
      };
      preexecs.push_back(pe);
    }
    else if(name == "precode") {
      if(preexecs.empty()) return false;
      int n;
      if(!Arc::stringto(buf,n)) return false;
      preexecs.back().successcode = n;
    }
    else if(name == "post") {
      Exec pe;
      while(!buf.empty()) {
        std::string arg;
        arg = Arc::unescape_chars(Arc::extract_escaped_token(buf, ' ', '\\'), '\\');
        pe.push_back(arg);
      };
      postexecs.push_back(pe);
    }
    else if(name == "postcode") {
      if(postexecs.empty()) return false;
      int n;
      if(!Arc::stringto(buf,n)) return false;
      postexecs.back().successcode = n;
    }
    else if(name == "cleanuptime") { cleanuptime = buf; }
    else if(name == "delegexpiretime") { expiretime = buf; }
    else if(name == "clientname") { clientname = buf; }
    else if(name == "clientsoftware") { clientsoftware = buf; }
    else if(name == "delegationid") { delegationid = buf; }
    else if(name == "sessiondir") { sessiondir = buf; }
    else if(name == "failedstate") { failedstate = buf; }
    else if(name == "failedcause") { failedcause = buf; }
    else if(name == "credentialserver") { credentialserver = buf; }
    else if(name == "freestagein") { freestagein = parse_boolean(buf); }
    else if(name == "localvo") {
      localvo.push_back(buf);
    }
    else if(name == "voms") {
      voms.push_back(buf);
    }
    else if(name == "diskspace") {
      unsigned long long int n;
      if(!Arc::stringto(buf,n)) return false;
      diskspace = n;
    }
    else if(name == "activityid") {
      activityid.push_back(buf);
    }
    else if(name == "migrateactivityid") { migrateactivityid = buf; }
    else if(name == "forcemigration") { forcemigration = parse_boolean(buf); }
    else if(name == "transfershare") { transfershare = buf; }
    else if(name == "priority") {
      int n;
      if(!Arc::stringto(buf,n)) return false;
      priority = n;
    }
  }
  return true;
}

bool JobLocalDescription::read_var(const std::string &fname,const std::string &vnam,std::string &value) {
  Glib::Mutex::Lock lock_(local_lock);
  // *.local file is accessed concurently. To avoid improper readings lock is acquired.
  KeyValueFile f(fname,KeyValueFile::Fetch);
  if(!f) return false;
  // using iostream for handling file content
  bool found = false;
  for(;;) {
    std::string buf;
    std::string name;
    if(!f.Read(name, buf)) return false;
    if(name.empty() && buf.empty()) break; // EOF
    if(name.empty()) continue;
    if(buf.empty()) continue;
    if(name == vnam) { value = buf; found=true; break; };
  };
  return found;
}

} // namespace ARex
