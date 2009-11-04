#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
/*
  Download files specified in job.ID.input and check if user uploaded files.
  Additionally check if this is a migrated job and if so kill the job on old cluster.
  result: 0 - ok, 1 - unrecoverable error, 2 - potentially recoverable,
  3 - certificate error, 4 - should retry.
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <errno.h>

#include <arc/XMLNode.h>
#include <arc/client/Job.h>
#include <arc/client/JobController.h>
#include <arc/UserConfig.h>
#include <arc/data/CheckSum.h>
#include <arc/data/FileCache.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataMover.h>
#include <arc/message/MCC.h>
#include <arc/StringConv.h>
#include <arc/Thread.h>
#include <arc/URL.h>
#include <arc/Utils.h>

#include "../jobs/job.h"
#include "../jobs/users.h"
#include "../files/info_types.h"
#include "../files/info_files.h"
#include "../files/delete.h"
#include "../conf/environment.h"
#include "../misc/proxy.h"
#include "../conf/conf_map.h"
#include "../conf/conf_cache.h"
#include "janitor.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(), "Downloader");

/* check for user uploaded files every 60 seconds */
#define CHECK_PERIOD 60
/* maximum number of retries (for every source/destination) */
#define MAX_RETRIES 5
/* maximum number simultaneous downloads */
#define MAX_DOWNLOADS 5
/* maximum time for user files to upload (per file) */
#define MAX_USER_TIME 600

class PointPair;

static void CollectCredentials(std::string& proxy,std::string& cert,std::string& key,std::string& cadir) {
  proxy=Arc::GetEnv("X509_USER_PROXY");
  if(proxy.empty()) {
    cert=Arc::GetEnv("X509_USER_CERT");
    key=Arc::GetEnv("X509_USER_KEY");
  };
  if(proxy.empty() && cert.empty()) {
    proxy="/tmp/x509_up"+Arc::tostring(getuid());
  };
  cadir=Arc::GetEnv("X509_CERT_DIR");
  if(cadir.empty()) cadir="/etc/grid-security/certificates";
}

class FileDataEx : public FileData {
 public:
  typedef std::list<FileDataEx>::iterator iterator;
  Arc::DataStatus res;
  PointPair* pair;
  FileDataEx(const FileData& f)
    : FileData(f),
      res(Arc::DataStatus::Success),
      pair(NULL) {}
  FileDataEx(const FileData& f, Arc::DataStatus r)
    : FileData(f),
      res(r),
      pair(NULL) {}
};

static std::list<FileData> job_files_;
static std::list<FileDataEx> job_files;
static std::list<FileDataEx> processed_files;
static std::list<FileDataEx> failed_files;
static Arc::SimpleCondition pair_condition;
static int pairs_initiated = 0;

class SimpleConditionLock {
 private:
  Arc::SimpleCondition& cond_;
 public:
  SimpleConditionLock(Arc::SimpleCondition& cond):cond_(cond) {
    cond_.lock();
  };
  ~SimpleConditionLock(void) {
    cond_.unlock();
  };
};

bool is_checksum(std::string str) {
  /* check if have : */
  std::string::size_type n;
  const char *s = str.c_str();
  if((n=str.find('.')) == std::string::npos) return false;
  if(str.find('.',n+1) != std::string::npos) return false;
  for(unsigned int i=0;i<n;i++) { if(!isdigit(s[i])) return false; };
  for(unsigned int i=n+1;s[i];i++) { if(!isdigit(s[i])) return false; };
  return true;
}

int clean_files(std::list<FileData> &job_files,char* session_dir) {
  std::string session(session_dir);
  /* delete only downloadable files, let user manage his/hers files */
  std::list<FileData> tmp;
  if(delete_all_files(session,job_files,false,true,false) != 2) return 0;
  return 1;
}

/*
   Check for existence of user uploadable file
   returns 0 if file exists 
           1 - it is not proper file or other error
           2 - not here yet 
*/
int user_file_exists(FileData &dt,char* session_dir,std::string* error = NULL) {
  struct stat st;
  const char *str = dt.lfn.c_str();
  if(strcmp(str,"*.*") == 0) return 0; /* do not wait for this file */
  std::string fname=std::string(session_dir) + '/' + dt.pfn;
  /* check if file does exist at all */
  if(lstat(fname.c_str(),&st) != 0) return 2;
  /* check for misconfiguration */
  /* parse files information */
  char *str_;
  unsigned long long int fsize;
  unsigned long long int fsum = (unsigned long long int)(-1);
  bool have_size = false;
  bool have_checksum = false;
  errno = 0;
  fsize = strtoull(str,&str_,10);
  if((*str_) == '.') {
    if(str_ != str) have_size=true;
    str=str_+1;
    fsum = strtoull(str,&str_,10);
    if((*str_) != 0) {
      logger.msg(Arc::ERROR, "Invalid checksum in %s for %s", dt.lfn, dt.pfn);
      if(error) (*error)="Bad information about file: checksum can't be parsed.";
      return 1;
    };
    have_checksum=true;
  }
  else {
    if(str_ != str) have_size=true;
    if((*str_) != 0) {
      logger.msg(Arc::ERROR, "Invalid file size in %s for %s ", dt.lfn, dt.pfn);
      if(error) (*error)="Bad information about file: size can't be parsed.";
      return 1;
    };
  };
  if(S_ISDIR(st.st_mode)) {
    if(have_size || have_checksum) {
      if(error) (*error)="Expected file. Directory found.";
      return 1;
    };
    return 0;
  };
  if(!S_ISREG(st.st_mode)) {
    if(error) (*error)="Expected ordinary file. Special object found.";
    return 1;
  };
  /* now check if proper size */
  if(have_size) {
    if(st.st_size < fsize) return 2;
    if(st.st_size > fsize) {
      logger.msg(Arc::ERROR, "Invalid file: %s is too big.", dt.pfn);
      if(error) (*error)="Delivered file is bigger than specified.";
      return 1; /* too big file */
    };
  };
  if(have_checksum) {
    int h=open(fname.c_str(),O_RDONLY);
    if(h==-1) { /* if we can't read that file job won't too */
      logger.msg(Arc::ERROR, "Error accessing file %s", dt.pfn);
      if(error) (*error)="Delivered file is unreadable.";
      return 1;
    };
    Arc::CRC32Sum crc;
    char buffer[1024];
    ssize_t l;
    size_t ll = 0;
    for(;;) {
      if((l=read(h,buffer,1024)) == -1) {
        logger.msg(Arc::ERROR, "Error reading file %s", dt.pfn);
        if(error) (*error)="Could not read file to compute checksum.";
        return 1;
      };
      if(l==0) break; ll+=l;
      crc.add(buffer,l);
    };
    close(h);
    crc.end();
    if(fsum != crc.crc()) {
      if(have_size) { /* size was checked - it is an error to have wrong crc */ 
        logger.msg(Arc::ERROR, "File %s has wrong CRC.", dt.pfn);
        if(error) (*error)="Delivered file has wrong checksum.";
        return 1;
      };
      return 2; /* just not uploaded yet */
    };
  };
  return 0; /* all checks passed - file is ok */
}

class PointPair {
 private:
  Arc::URL source_url;
  Arc::URL destination_url;
 public:
  Arc::DataHandle source;
  Arc::DataHandle destination;
  PointPair(const std::string& source_str, const std::string& destination_str,
	    const Arc::UserConfig& usercfg)
    : source_url(source_str),
      destination_url(destination_str),
      source(source_url, usercfg),
      destination(destination_url, usercfg) {};
  ~PointPair(void) {};
  static void callback(Arc::DataMover*,Arc::DataStatus res,void* arg) {
    FileDataEx::iterator &it = *((FileDataEx::iterator*)arg);
    pair_condition.lock();
    if(!res.Passed()) {
      it->res=res;
      logger.msg(Arc::ERROR, "Failed downloading file %s - %s", it->lfn, std::string(res));
      if((it->pair->source->GetTries() <= 0) || (it->pair->destination->GetTries() <= 0)) {
        delete it->pair; it->pair=NULL;
        failed_files.push_back(*it);
      } else {
        job_files.push_back(*it);
        logger.msg(Arc::ERROR, "Retrying");
      };
    } else {
      logger.msg(Arc::INFO, "Downloaded file %s", it->lfn);
      delete it->pair; it->pair=NULL;
      processed_files.push_back(*it);
    };
    job_files.erase(it);
    --pairs_initiated;
    pair_condition.signal_nonblock();
    pair_condition.unlock();
    delete &it;
  };
};

int main(int argc,char** argv) {
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::DEBUG);
  int res=0;
  bool not_uploaded;
  time_t start_time=time(NULL);
  time_t upload_timeout = 0;
  int n_threads = 1;
  int n_files = MAX_DOWNLOADS;
  /* if != 0 - change owner of downloaded files to this user */
  std::string file_owner_username = "";
  uid_t file_owner = 0;
  gid_t file_group = 0;
  std::vector<std::string> caches;
  bool use_conf_cache=false;
  unsigned long long int min_speed = 0;
  time_t min_speed_time = 300;
  unsigned long long int min_average_speed = 0;
  time_t max_inactivity_time = 300;
  bool secure = true;
  bool userfiles_only = false;
  bool passive = false;
  std::string failure_reason("");
  std::string x509_proxy, x509_cert, x509_key, x509_cadir;

  // process optional arguments
  for(;;) {
    opterr=0;
    int optc=getopt(argc,argv,"+hclpfC:n:t:n:u:U:s:S:a:i:d:");
    if(optc == -1) break;
    switch(optc) {
      case 'h': {
        std::cerr<<"Usage: downloader [-hclpf] [-C conf_file] [-n files] [-t threads] [-U uid]"<<std::endl;
        std::cerr<<"                  [-u username] [-s min_speed] [-S min_speed_time]"<<std::endl;
        std::cerr<<"                  [-a min_average_speed] [-i min_activity_time]"<<std::endl;
        std::cerr<<"                  [-d debug_level] job_id control_directory"<<std::endl;
        std::cerr<<"                  session_directory [cache options]"<<std::endl;
        exit(1);
      }; break;
      case 'c': {
        secure=false;
      }; break;
      case 'C': {
        nordugrid_config_loc(optarg);
      }; break;
      case 'l': {
        userfiles_only=true;
      }; break;
      case 'p': {
        passive=true;
      }; break;
      case 'f': {
        use_conf_cache=true;
      }; break;
      case 'd': {
        Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(optarg));
      }; break;
      case 't': {
        n_threads=atoi(optarg);
        if(n_threads < 1) {
          logger.msg(Arc::ERROR, "Wrong number of threads: %s", optarg); exit(1);
        };
      }; break;
      case 'n': {
        n_files=atoi(optarg);
        if(n_files < 1) {
          logger.msg(Arc::ERROR, "Wrong number of files: %s", optarg); exit(1);
        };
      }; break;
      case 'U': {
        unsigned int tuid;
        if(!Arc::stringto(std::string(optarg),tuid)) {
          logger.msg(Arc::ERROR, "Bad number: %s", optarg); exit(1);
        };
        struct passwd pw_;
        struct passwd *pw;
        char buf[BUFSIZ];
        getpwuid_r(tuid,&pw_,buf,BUFSIZ,&pw);
        if(pw == NULL) {
          logger.msg(Arc::ERROR, "Wrong user name"); exit(1);
        };
        file_owner=pw->pw_uid;
        file_group=pw->pw_gid;
        if(pw->pw_name) file_owner_username=pw->pw_name;
        if((getuid() != 0) && (getuid() != file_owner)) {
          logger.msg(Arc::ERROR, "Specified user can't be handled"); exit(1);
        };
      }; break;
      case 'u': {
        struct passwd pw_;
        struct passwd *pw;
        char buf[BUFSIZ];
        getpwnam_r(optarg,&pw_,buf,BUFSIZ,&pw);
        if(pw == NULL) {
          logger.msg(Arc::ERROR, "Wrong user name"); exit(1);
        };
        file_owner=pw->pw_uid;
        file_group=pw->pw_gid;
        if(pw->pw_name) file_owner_username=pw->pw_name;
        if((getuid() != 0) && (getuid() != file_owner)) {
          logger.msg(Arc::ERROR, "Specified user can't be handled"); exit(1);
        };
      }; break;
      case 's': {
        unsigned int tmpi;
        if(!Arc::stringto(std::string(optarg),tmpi)) {
          logger.msg(Arc::ERROR, "Bad number: %s", optarg); exit(1);
        };
        min_speed=tmpi;
      }; break;
      case 'S': {
        unsigned int tmpi;
        if(!Arc::stringto(std::string(optarg),tmpi)) {
          logger.msg(Arc::ERROR, "Bad number: %s", optarg); exit(1);
        };
        min_speed_time=tmpi;
      }; break;
      case 'a': {
        unsigned int tmpi;
        if(!Arc::stringto(std::string(optarg),tmpi)) {
          logger.msg(Arc::ERROR, "Bad number: %s", optarg); exit(1);
        };
        min_average_speed=tmpi;
      }; break;
      case 'i': {
        unsigned int tmpi;
        if(!Arc::stringto(std::string(optarg),tmpi)) {
          logger.msg(Arc::ERROR, "Bad number: %s", optarg); exit(1);
        };
        max_inactivity_time=tmpi;
      }; break;
      case '?': {
        logger.msg(Arc::ERROR, "Unsupported option: %c", (char)optopt);
        exit(1);
      }; break;
      case ':': {
        logger.msg(Arc::ERROR, "Missing parameter for option %c", (char)optopt);
        exit(1);
      }; break;
      default: {
        logger.msg(Arc::ERROR, "Undefined processing error");
        exit(1);
      };
    };
  };
  // process required arguments
  char * id = argv[optind+0];
  if(!id) { logger.msg(Arc::ERROR, "Missing job id"); return 1; };
  char* control_dir = argv[optind+1];
  if(!control_dir) { logger.msg(Arc::ERROR, "Missing control directory"); return 1; };
  char* session_dir = argv[optind+2];
  if(!session_dir) { logger.msg(Arc::ERROR, "Missing session directory"); return 1; };

  read_env_vars();
  // prepare Job and User descriptions (needed for substitutions in cache dirs)
  JobDescription desc(id,session_dir);
  uid_t uid;
  gid_t gid;
  if(file_owner != 0) { uid=file_owner; }
  else { uid= getuid(); };
  if(file_group != 0) { gid=file_group; }
  else { gid= getgid(); };
  desc.set_uid(uid,gid);
  JobUser user(uid);
  user.SetControlDir(control_dir);
  user.SetSessionRoot(session_dir);
  
  // if u or U option not set, use our username
  if (file_owner_username == "") {
    struct passwd pw_;
    struct passwd *pw;
    char buf[BUFSIZ];
    getpwuid_r(getuid(),&pw_,buf,BUFSIZ,&pw);
    if(pw == NULL) {
      logger.msg(Arc::ERROR, "Wrong user name"); exit(1);
    }
    if(pw->pw_name) file_owner_username=pw->pw_name;
  }

  if(use_conf_cache) {
    // use cache dir(s) from conf file
    CacheConfig * cache_config;
    try {
      cache_config = new CacheConfig(std::string(file_owner_username));
      std::list<std::string> conf_caches = cache_config->getCacheDirs();
      // add each cache to our list
      for (std::list<std::string>::iterator i = conf_caches.begin(); i != conf_caches.end(); i++) {
        user.substitute(*i);
        caches.push_back(*i);
      }
    }
    catch (CacheConfigException e) {
      logger.msg(Arc::ERROR, "Error with cache configuration: %s", e.what());
      logger.msg(Arc::ERROR, "Will not use caching");
    }
    delete cache_config;
  }
  else {
    if(argv[optind+3]) {
      std::string cache_path = argv[optind+3];
      if(argv[optind+4]) cache_path += " "+std::string(argv[optind+4]);
      caches.push_back(cache_path);
    }
  }
  if(min_speed != 0)
    logger.msg(Arc::DEBUG, "Minimal speed: %llu B/s during %i s", min_speed, min_speed_time);
  if(min_average_speed != 0)
    logger.msg(Arc::DEBUG, "Minimal average speed: %llu B/s", min_average_speed);
  if(max_inactivity_time != 0)
    logger.msg(Arc::DEBUG, "Maximal inactivity time: %i s", max_inactivity_time);

  CollectCredentials(x509_proxy,x509_cert,x509_key,x509_cadir);
  prepare_proxy();

  if(n_threads > 10) {
    logger.msg(Arc::WARNING, "Won't use more than 10 threads");
    n_threads=10;
  };
/*
 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!  Add this to DataMove !!!!!!!!!!!!
*/
  UrlMapConfig url_map;
  logger.msg(Arc::INFO, "Downloader started");

  Arc::FileCache * cache = NULL;
  if(!caches.empty()) {
    cache = new Arc::FileCache(caches,std::string(id),uid,gid);
    if (!(*cache)) {
      logger.msg(Arc::ERROR, "Error creating cache");
      delete cache;
      exit(1);
    }
  }
  else {
    // if no cache defined, use null cache
    cache = new Arc::FileCache();
  }

  Janitor janitor(desc.get_id(),user.ControlDir());
  
  Arc::UserConfig usercfg(Arc::initializeCredentialsType(Arc::initializeCredentialsType::TryCredentials));

  Arc::DataMover mover;
  mover.retry(false);
  mover.secure(secure);
  mover.passive(passive);
  if(min_speed != 0)
    mover.set_default_min_speed(min_speed,min_speed_time);
  if(min_average_speed != 0)
    mover.set_default_min_average_speed(min_average_speed);
  if(max_inactivity_time != 0)
    mover.set_default_max_inactivity_time(max_inactivity_time);
  bool transfered = true;
  bool credentials_expired = false;
  std::list<FileData> output_files;

  if(!job_input_read_file(desc.get_id(),user,job_files_)) {
    failure_reason+="Internal error in downloader\n";
    logger.msg(Arc::ERROR, "Can't read list of input files"); res=1; goto exit;
  };
  // check for duplicates (see bug 1285)
  for (std::list<FileData>::iterator i = job_files_.begin(); i != job_files_.end(); i++) {
    for (std::list<FileData>::iterator j = job_files_.begin(); j != job_files_.end(); j++) {
      if (i != j && j->pfn == i->pfn) {
        failure_reason+="Duplicate input files\n";
        logger.msg(Arc::ERROR, "Error: duplicate file in list of input files: %s", i->pfn);
        res=1;
        goto exit;
      }
    }
  }
  // check if any input files are also output files downloadable by user (bug 1387)
  if(job_output_read_file(desc.get_id(),user,output_files)) {
    for (std::list<FileData>::iterator j = output_files.begin(); j != output_files.end(); j++) {
      for (std::list<FileData>::iterator i = job_files_.begin(); i != job_files_.end(); i++) {
        if (i->pfn == j->pfn && j->lfn.empty()) {
          Arc::URL u(i->lfn);
          std::string opt = u.Option("cache");
          // don't add copy option if exists or current option is "no" or "renew"
          if (opt.empty() || !(opt == "no" || opt == "renew" || opt == "copy")) {
            u.AddOption("cache", "copy", true);
            i->lfn = u.fullstr();
          }
        }
      }
    }
  }
  else
    logger.msg(Arc::WARNING, "Can't read list of output files");
      
  // remove bad files
  if(clean_files(job_files_,session_dir) != 0) { 
    failure_reason+="Internal error in downloader\n";
    logger.msg(Arc::ERROR, "Can't remove junk files"); res=1; goto exit;
  };
  for(std::list<FileData>::iterator i = job_files_.begin();i!=job_files_.end();++i) {
    job_files.push_back(*i);
  };

  if(!desc.GetLocalDescription(user)) {
    failure_reason+="Internal error in downloader\n";
    logger.msg(Arc::ERROR, "Can't read job local description"); res=1; goto exit;
  };

  // Start janitor in parallel
  if(!janitor) {
    if(desc.get_local()->rtes > 0) {
      failure_reason+="Non-existing RTE(s) requested\n";
      logger.msg(Arc::ERROR, "Janitor is missing and job contains non-deployed RTEs");
      res=1; goto exit;
    };
  } else {
    if(!janitor.deploy()) {
      failure_reason+="The Janitor failed\n";
      logger.msg(Arc::ERROR, "Failed to deploy Janitor"); res=1; goto exit;
    };
  };

  // initialize structures to handle download
  /* TODO: add threads=# to all urls if n_threads!=1 */
  // Compute wait time for user files
  for(FileDataEx::iterator i=job_files.begin();i!=job_files.end();++i) {
    if(i->lfn.find(":") == std::string::npos) { /* is it lfn ? */
      upload_timeout+=MAX_USER_TIME;
    } else {
      userfiles_only=false;
    };
  };
  // Main download cycle
  if(!userfiles_only) for(;;) {
    // Initiate transfers
    int n = 0;
    SimpleConditionLock local_lock(pair_condition);
    for(FileDataEx::iterator i=job_files.begin();i!=job_files.end();++i) {
      if(i->lfn.find(":") != std::string::npos) { /* is it lfn ? */
        ++n;
        if(n <= pairs_initiated) continue; // skip files being processed
        if(n > n_files) break; // quit if not allowed to process more
        /* have place and file to download */
        std::string destination=std::string("file://") + session_dir + i->pfn;
        std::string source = i->lfn;
        if(i->pair == NULL) {
          /* define place to store */
          if(strncasecmp(source.c_str(),"file:/",6) == 0) {
            failure_reason+=std::string("User requested local input file ")+source.c_str()+"\n";
            logger.msg(Arc::ERROR, "Local source for download: %s", source); res=1; goto exit;
          };
          PointPair* pair = new PointPair(source,destination,usercfg);
          if(!(pair->source)) {
            failure_reason+=std::string("Can't accept URL ")+source.c_str()+"\n";
            logger.msg(Arc::ERROR, "Can't accept URL: %s", source); delete pair; res=1; goto exit;
          };
          if(!(pair->destination)) {
            failure_reason+=std::string("Can't accept URL ")+destination.c_str()+"\n";
            logger.msg(Arc::ERROR, "Can't accept URL: %s", destination); delete pair; res=1; goto exit;
          };
          i->pair=pair;
        };
        FileDataEx::iterator* it = new FileDataEx::iterator(i);
        Arc::DataStatus dres = mover.Transfer(*(i->pair->source), *(i->pair->destination), *cache,
                                              url_map, min_speed, min_speed_time,
                                              min_average_speed, max_inactivity_time,
                                              &PointPair::callback, it,
                                              i->pfn.c_str());
        if (!dres.Passed()) {
          failure_reason+=std::string("Failed to initiate file transfer: ")+source.c_str()+" - "+std::string(dres)+"\n";
          logger.msg(Arc::ERROR, "Failed to initiate file transfer: %s - %s", source, std::string(dres));
          delete it; res=1; goto exit;
        };
        ++pairs_initiated;
      };
    };
    if(pairs_initiated <= 0) break; // Looks like no more files to process
    // Processing initiated - now wait for event
    pair_condition.wait_nonblock();
  };
  // Print download summary
  for(FileDataEx::iterator i=processed_files.begin();i!=processed_files.end();++i) {
    logger.msg(Arc::INFO, "Downloaded %s", i->lfn);
    if(Arc::URL(i->lfn).Option("exec") == "yes") {
      fix_file_permissions(session_dir+i->pfn,true);
    };
  };
  for(FileDataEx::iterator i=failed_files.begin();i!=failed_files.end();++i) {
    if (i->res == Arc::DataStatus::CacheErrorRetryable) {
      logger.msg(Arc::ERROR, "Failed to download (but may be retried) %s",i->lfn);
      job_files.push_back(*i);
      res = 4;
      continue;
    }
    logger.msg(Arc::ERROR, "Failed to download %s", i->lfn);
    failure_reason+="Input file: "+i->lfn+" - "+(std::string)(i->res)+"\n";
    if(i->res == Arc::DataStatus::CredentialsExpiredError)
      credentials_expired=true;
    transfered=false;
  };
  // Check if all files have been properly downloaded
  if(!transfered) {
    logger.msg(Arc::INFO, "Some downloads failed"); res=2;
    if(credentials_expired) res=3;
    goto exit;
  };
  if(res == 4) logger.msg(Arc::INFO, "Some downloads failed, but may be retried");
  job_files_.clear();
  for(FileDataEx::iterator i = job_files.begin();i!=job_files.end();++i) job_files_.push_back(*i);
  if(!job_input_write_file(desc,user,job_files_)) {
    logger.msg(Arc::WARNING, "Failed writing changed input file");
  };
  // check for user uploadable files
  // run cycle waiting for uploaded files
  for(;;) {
    not_uploaded=false;
    for(FileDataEx::iterator i=job_files.begin();i!=job_files.end();) {
      if(i->lfn.find(":") == std::string::npos) { /* is it lfn ? */
        /* process user uploadable file */
        logger.msg(Arc::INFO, "Check user uploadable file: %s", i->pfn);
        std::string error;
        int err=user_file_exists(*i,session_dir,&error);
        if(err == 0) { /* file is uploaded */
          logger.msg(Arc::INFO, "User has uploaded file %s", i->pfn);
          i=job_files.erase(i);
          job_files_.clear();
          for(FileDataEx::iterator i = job_files.begin();i!=job_files.end();++i) job_files_.push_back(*i);
          if(!job_input_write_file(desc,user,job_files_)) {
            logger.msg(Arc::WARNING, "Failed writing changed input file.");
          };
        }
        else if(err == 1) { /* critical failure */
          logger.msg(Arc::ERROR, "Critical error for uploadable file %s", i->pfn);
          failure_reason+="User file: "+i->pfn+" - "+error+"\n";
          res=1; goto exit;
        }
        else {
          not_uploaded=true; ++i;
        };
      }
      else {
        ++i;
      };
    };
    if(!not_uploaded) break;
    // check for timeout
    if((time(NULL)-start_time) > upload_timeout) {
      for(FileDataEx::iterator i=job_files.begin();i!=job_files.end();++i) {
        if(i->lfn.find(":") == std::string::npos) { /* is it lfn ? */
          failure_reason+="User file: "+i->pfn+" - Timeout waiting\n";
        };
      };
      logger.msg(Arc::ERROR, "Uploadable files timed out"); res=2; break;
    };
    sleep(CHECK_PERIOD);
  };
  job_files_.clear();
  for(FileDataEx::iterator i = job_files.begin();i!=job_files.end();++i) job_files_.push_back(*i);
  if(!job_input_write_file(desc,user,job_files_)) {
    logger.msg(Arc::WARNING, "Failed writing changed input file.");
  };

  // Check for janitor result
  if(janitor) {
    unsigned int time_passed = time(NULL) - start_time;
    // Hardcoding max 30 minutes per RTE + 5 minutes just in case
    unsigned int time_left = 30*60*desc.get_local()->rtes + 5*60;
    time_left-=(time_left > time_passed)?time_passed:time_left;
    if(!janitor.wait(time_left)) {
      failure_reason+="The Janitor failed\n";
      logger.msg(Arc::ERROR, "Janitor timeout while deploying Dynamic RTE(s)");
      res=1; goto exit;
    };
    if(janitor.result() == Janitor::DEPLOYED) {
    } else if(janitor.result() == Janitor::NOTENABLED) {
      if(desc.get_local()->rtes > 0) {
        failure_reason+="The Janitor failed\n";
        logger.msg(Arc::ERROR, "Janitor not enabled and there are missing RTE(s)");
        res=1; goto exit;
      }
    } else {
      failure_reason+="The Janitor failed\n";
      logger.msg(Arc::ERROR, "Janitor failed to deploy Dynamic RTE(s)");
      res=1; goto exit;
    };
  };

  // Job migration functionality
  if (res == 0) {
    if(desc.get_local()->migrateactivityid != "") {
      // Complete the migration.
      const size_t found = desc.get_local()->migrateactivityid.rfind("/");

      if (found != std::string::npos) {
        Arc::Job job;
        job.Flavour = "ARC1";
        job.JobID = Arc::URL(desc.get_local()->migrateactivityid);
        job.Cluster = Arc::URL(desc.get_local()->migrateactivityid.substr(0, found));

        Arc::UserConfig usercfg(true);

        Arc::JobControllerLoader loader;
        Arc::JobController *jobctrl = loader.load("ARC1", usercfg);
        if (jobctrl) {
          jobctrl->FillJobStore(job);

          std::list<std::string> status;
          status.push_back("Queuing");
          
          if (!jobctrl->Kill(status, true) && !desc.get_local()->forcemigration) {
            res = 1;
            failure_reason = "FATAL ERROR: Migration failed attempting to kill old job \"" + desc.get_local()->migrateactivityid + "\".";
          }
        }
        else {
          res = 1;
          failure_reason = "FATAL ERROR: Could not locate ARC1 JobController plugin. Maybe it is not installed?";
        }
      }
    }
  }

exit:
  logger.msg(Arc::INFO, "Leaving downloader (%i)", res);
  // clean unfinished files here 
  job_files_.clear();
  for(FileDataEx::iterator i = job_files.begin();i!=job_files.end();++i) job_files_.push_back(*i);
  clean_files(job_files_,session_dir);
  // release cache just in case
  if(res != 0 && res != 4) {
    cache->Release();
  };
  delete cache;
  remove_proxy();
  if(res != 0 && res != 4) {
    job_failed_mark_add(desc,user,failure_reason);
  };
  return res;
}
