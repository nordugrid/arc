#ifndef GRID_MANAGER_INFO_TYPES_H
#define GRID_MANAGER_INFO_TYPES_H

#include <string>
#include <list>
#include <iostream>

#include <arc/DateTime.h>
#include <arc/compute/JobDescription.h>

namespace ARex {

/* 
  Defines few data types used by grid-manager to store information
  about jobs.
*/

/*
  Pair of values containing file's path (pfn - physical file name)
  and it's source or destination on the net (lfn - logical file name)
*/
class FileData {
 public:
  typedef std::list<FileData>::iterator iterator;
  FileData(void);
  FileData(const std::string& pfn_s,const std::string& lfn_s);
  std::string pfn;  // path relative to session dir
  std::string lfn;  // input/output url or size.checksum
  std::string cred; // path to file containing credentials
  std::string cred_type; // type of credentials in cred file
  bool ifsuccess;
  bool ifcancel;
  bool iffailure;
  FileData& operator= (const char* str);
  bool operator== (const char* name);
  bool operator== (const FileData& data);
  bool has_lfn(void);
};
std::istream &operator>> (std::istream &i,FileData &fd);
std::ostream &operator<< (std::ostream &o,const FileData &fd);

class Exec: public std::list<std::string> {
 public:
  Exec(void):successcode(0) {};
  Exec(const std::list<std::string>& src):std::list<std::string>(src),successcode(0) {};
  Exec(const Arc::ExecutableType& src):successcode(0) {
    operator=(src);
  };
  Exec& operator=(const std::list<std::string>& src) {
    std::list<std::string>::operator=(src); return *this;
  };
  Exec& operator=(const Arc::ExecutableType& src);
  int successcode;
};
std::istream &operator>> (std::istream &i,Exec &fd);
std::ostream &operator<< (std::ostream &o,const Exec &fd);

/*
  Most important information about job extracted from different sources
  (mostly from job description) and stored in separate file for 
  relatively quick and simple access.
*/
class JobLocalDescription {
 /* all values are public, this class is just for convenience */
 public:
 JobLocalDescription(void):jobid(""),globalid(""),headnode(""),headhost(""),globalurl(""),
                           lrms(""),queue(""),localid(""),
                           DN(""),starttime((time_t)(-1)),lifetime(""),
                           notify(""),processtime((time_t)(-1)),exectime((time_t)(-1)),
                           clientname(""),clientsoftware(""),delegationid(""),
                           reruns(0),priority(prioritydefault),downloads(-1),uploads(-1),
                           jobname(""),jobreport(),
                           cleanuptime((time_t)(-1)),expiretime((time_t)(-1)),
                           failedstate(""),failedcause(""),
                           credentialserver(""),freestagein(false),gsiftpthreads(1),
                           dryrun(false),diskspace(0),
                           transfershare(JobLocalDescription::transfersharedefault)                        
  {}

  JobLocalDescription& operator=(const Arc::JobDescription& arc_job_desc);

  bool read(const std::string& fname);
  bool write(const std::string& fname) const;
  static bool read_var(const std::string &fname,const std::string &vnam,std::string &value);
  
  // All non-static members are safe to copy

  std::string jobid;         /* job's unique identifier */
  /* attributes stored in job.ID.local */
  std::string globalid;      /* job id as seen from outside */
  std::string headnode;      /* URL of the cluster's headnode */
  std::string headhost;      /* hostname of cluster's headnode */
  std::string globalurl;     /* URL roughly representing job on cluster */
  std::string interface;     /* interface type used to submit job */
  std::string lrms;          /* lrms type to use - pbs */
  std::string queue;         /* queue name  - default */
  std::string localid;       /* job's id in lrms */
  std::list<Exec> preexecs;  /* executable + arguments */
  Exec exec;                 /* executable + arguments */
  std::list<Exec> postexecs; /* executable + arguments */
  std::string DN;            /* user's distinguished name aka subject name */
  Arc::Time starttime;       /* job submission time */
  std::string lifetime;      /* time to live for submission directory */
  std::string notify;        /* notification flags used and email address */
  Arc::Time processtime;     /* time to start job processing (downloading) */
  Arc::Time exectime;        /* time to start execution */
  std::string clientname;    /* IP+port of user interface + info given by ui */
  std::string clientsoftware; /* Client's version */
  std::string delegationid;  /* id of deleation assigned to this job (not per file) */
  int    reruns;             /* number of allowed reruns left */
  int    priority;           /* priority the job has */
  int    downloads;          /* number of downloadable files requested */
  int    uploads;            /* number of uploadable files requested */
  std::string jobname;       /* name of job given by user */
  std::list<std::string> projectnames;  /* project names, i.e. "ACIDs" */
  std::list<std::string> jobreport;     /* URLs of user's/VO's loggers */
  Arc::Time cleanuptime;     /* time to remove job completely */
  Arc::Time expiretime;      /* when main delegation expires */
  std::string stdlog;        /* dirname to which log messages will be
                                put after job finishes */
  std::string sessiondir;    /* job's session directory */
  std::string failedstate;   /* state at which job failed, used for rerun */
  std::string failedcause;   /* reason for job failure, client or internal error */
  std::string credentialserver; /* URL of server used to renew credentials - MyProxy */
  bool freestagein;          /* if true, client is allowed to stage in any files */
  std::list<std::string> localvo; /* VO names to which user belongs according to local configuration*/
  std::list<std::string> voms;    /* VOMS FQANs which we matched during authorization process */
  /* attributes stored in other files */
  std::list<FileData> inputdata;  /* input files */
  std::list<FileData> outputdata; /* output files */
  /* attributes taken from job description */
  std::list<std::string> rte; /* runtime environments */
  std::string action;        /* what to do - must be 'request' */
  std::string rc;            /* url to contact replica collection */
  std::string stdin_;         /* file name for stdin handle */
  std::string stdout_;        /* file name for stdout handle */
  std::string stderr_;        /* file name for stderr handle */
  std::string cache;         /* cache default, yes/no */
  int    gsiftpthreads;      /* number of parallel connections to use 
                                during gsiftp down/uploads */
  bool   dryrun;             /* if true, this is test job */
  unsigned long long int diskspace;  /* anount of requested space on disk (unit bytes) */

  std::list<std::string> activityid;     /* ID of activity */

  std::string transfershare; /* share assigned to job for transfer fair share */

  // Default values which are not zero
  static int const         prioritydefault;      /* default priority for the job if not specified */
  static const char* const transfersharedefault; /* default value for transfer share */
  
};

/* Information stored in job.#.lrms_done file */
class LRMSResult {
 private:
  int code_;
  std::string description_;
  bool set(const char* s);
 public:
  LRMSResult(void):code_(-1),description_("") { };
  LRMSResult(const std::string& s) { set(s.c_str()); };
  LRMSResult(int c):code_(c),description_("") { };
  LRMSResult(const char* s) { set(s); };
  LRMSResult& operator=(const std::string& s) { set(s.c_str()); return *this; };
  LRMSResult& operator=(const char* s) { set(s); return *this; };
  int code(void) const { return code_; }; 
  const std::string& description(void) const { return description_; };
};
std::istream& operator>>(std::istream& i,LRMSResult &r);
std::ostream& operator<<(std::ostream& i,const LRMSResult &r);

} // namespace ARex

#endif
