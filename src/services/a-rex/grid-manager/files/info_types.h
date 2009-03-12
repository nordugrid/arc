#ifndef GRID_MANAGER_INFO_TYPES_H
#define GRID_MANAGER_INFO_TYPES_H

#include <string>
#include <list>
#include <iostream>

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
  Time with visual representation suitable for puting into
  MDS fields Mds-Valid-from and Mds-Valid-to. It looks like
  YYYYMMDDhhmmssZ . Here Z stands for Z. All values are GMT.
  Internally it's represented by Unix timestamp.
*/
class mds_time {
 private:
  time_t t;
 public:
  mds_time(void) { t=(time_t)(-1); };
  mds_time(time_t tm) { t=tm; };
  mds_time& operator= (time_t tm) { t=tm; return (*this); };
  mds_time& operator= (std::string s);
  mds_time& operator= (const char* s);
  operator time_t (void) const { return t; };
  bool defined(void) const { return (t != -1); };
  bool operator>  (time_t tm) { return (t>tm);  };
  bool operator>= (time_t tm) { return (t>=tm); };
  bool operator<= (time_t tm) { return (t<=tm); };
  bool operator<  (time_t tm) { return (t<tm);  };
  bool operator== (time_t tm) { return (t==tm); };
  std::string str(void) const;
};

std::istream &operator>> (std::istream &i,mds_time &t);
std::ostream &operator<< (std::ostream &o,const mds_time &t);

/*
  Most important information about job extracted from different sources
  (mostly from job description) and stored in separate file for 
  relatively quick and simple access.
*/
class JobLocalDescription {
 /* all values are public, this class is just for convenience */
 public:
  JobLocalDescription(void):jobid(""),lrms(""),queue(""),localid(""),
                           DN(""),starttime(),lifetime(""),
                           notify(""),processtime(),exectime(),
                           clientname(""),clientsoftware(""),
                           reruns(0),downloads(-1),uploads(-1),
                           jobname(""),jobreport(""),
                           cleanuptime(),expiretime(),
                           failedstate(""),fullaccess(false),
                           credentialserver(""),gsiftpthreads(1),
                           dryrun(false),diskspace(0), migrateactivityid(""),
                           forcemigration(false)
  {
  };
  std::string jobid;         /* job's unique identificator */
  /* attributes stored in job.ID.local */
  std::string lrms;          /* lrms type to use - pbs */
  std::string queue;         /* queue name  - default */
  std::string localid;       /* job's id in lrms */
  std::list<std::string> arguments;    /* executable + arguments */
  std::string DN;            /* user's distinguished name aka subject name */
  mds_time starttime;        /* job submission time */
  std::string lifetime;      /* time to live for submission directory */
  std::string notify;        /* notification flags used and email address */
  mds_time processtime;      /* time to start job processing (downloading) */
  mds_time exectime;         /* time to start execution */
  std::string clientname;    /* IP+port of user interface + info given by ui */
  std::string clientsoftware; /* Client's version */
  int    reruns;             /* number of allowed reruns left */
  int    downloads;          /* number of downloadable files requested */
  int    uploads;            /* number of uploadable files requested */
  std::string jobname;       /* name of job given by user */
  std::string jobreport;     /* URL of user's/VO's logger */
  mds_time cleanuptime;      /* time to remove job completely */
  mds_time expiretime;       /* when delegation expires */
  std::string stdlog;        /* dirname to which log messages will be
                                put after job finishes */
  std::string sessiondir;    /* job's session directory */
  std::string failedstate;   /* state at which job failed, used for rerun */
  bool fullaccess;           /* full access to session directory while job is running */
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
  unsigned long long int diskspace;  /* anount of requested space on disk */
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
