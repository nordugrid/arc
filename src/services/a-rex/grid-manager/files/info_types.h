#ifndef GRID_MANAGER_INFO_TYPES_H
#define GRID_MANAGER_INFO_TYPES_H

#include <string>
#include <list>
#include <iostream>

#include <arc/DateTime.h>
#include <arc/client/JobDescription.h>

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
  FileData(const char *pfn_s,const char *lfn_s);
  std::string pfn;  /* path relative to session dir */
  std::string lfn;  /* input/output url or size.checksum */
  FileData& operator= (const char* str);
  bool operator== (const char* name);
  bool operator== (const FileData& data);
  bool has_lfn(void);
};
std::istream &operator>> (std::istream &i,FileData &fd);
std::ostream &operator<< (std::ostream &o,const FileData &fd);


/*
  Most important information about job extracted from different sources
  (mostly from job description) and stored in separate file for 
  relatively quick and simple access.
*/
class JobLocalDescription {
 /* all values are public, this class is just for convenience */
 public:
 JobLocalDescription(void):jobid(""),globalid(""),lrms(""),queue(""),localid(""),
                           DN(""),starttime((time_t)(-1)),lifetime(""),
                           notify(""),processtime((time_t)(-1)),exectime((time_t)(-1)),
                           clientname(""),clientsoftware(""),
                           reruns(0),downloads(-1),uploads(-1),
                           jobname(""),jobreport(),
                           cleanuptime((time_t)(-1)),expiretime((time_t)(-1)),
                           failedstate(""),
                           credentialserver(""),gsiftpthreads(1),
                           dryrun(false),diskspace(0), migrateactivityid(""),
                           forcemigration(false)
  {}

  JobLocalDescription& operator=(const Arc::JobDescription& arc_job_desc);
  
  std::string jobid;         /* job's unique identificator */
  /* attributes stored in job.ID.local */
  std::string globalid;      /* BES ActivityIdentifier, i.e. globally unique id */
  std::string lrms;          /* lrms type to use - pbs */
  std::string queue;         /* queue name  - default */
  std::string localid;       /* job's id in lrms */
  std::list<std::string> arguments;    /* executable + arguments */
  std::string DN;            /* user's distinguished name aka subject name */
  Arc::Time starttime;      /* job submission time */
  std::string lifetime;      /* time to live for submission directory */
  std::string notify;        /* notification flags used and email address */
  Arc::Time processtime;      /* time to start job processing (downloading) */
  Arc::Time exectime;         /* time to start execution */
  std::string clientname;    /* IP+port of user interface + info given by ui */
  std::string clientsoftware; /* Client's version */
  int    reruns;             /* number of allowed reruns left */
  int    downloads;          /* number of downloadable files requested */
  int    uploads;            /* number of uploadable files requested */
  std::string jobname;       /* name of job given by user */
  std::list<std::string> projectnames;  /* project names, i.e. "ACIDs" */
  std::list<std::string> jobreport;     /* URLs of user's/VO's loggers */
  Arc::Time cleanuptime;      /* time to remove job completely */
  Arc::Time expiretime;       /* when delegation expires */
  std::string stdlog;        /* dirname to which log messages will be
                                put after job finishes */
  std::string sessiondir;    /* job's session directory */
  std::string failedstate;   /* state at which job failed, used for rerun */
  std::string credentialserver; /* URL of server used to renew credentials - MyProxy */
  /* attributes stored in other files */
  std::list<FileData> inputdata;  /* input files */
  std::list<FileData> outputdata; /* output files */
  /* attributes taken from RSL */
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
  std::string migrateactivityid;     /* ID of activity that is being migrated*/
  bool forcemigration;      /* Ignore if killing of old job fails */
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
  void operator=(const std::string& s) { set(s.c_str()); };
  void operator=(const char* s) { set(s); };
  int code(void) const { return code_; }; 
  const std::string& description(void) const { return description_; };
};
std::istream& operator>>(std::istream& i,LRMSResult &r);
std::ostream& operator<<(std::ostream& i,const LRMSResult &r);

/*
  Writes to 'o' string 'str' prepending each blank space with \ .
*/
void output_escaped_string(std::ostream &o,const std::string &str);

#endif
