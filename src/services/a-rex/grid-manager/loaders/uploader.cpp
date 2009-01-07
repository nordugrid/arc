#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
/*
  Upload files specified in job.ID.output.
  result: 0 - ok, 1 - unrecoverable error, 2 - potentially recoverable.
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>

#include <glibmm.h>

#include <arc/data/DMC.h>
#include <arc/data/CheckSum.h>
#include <arc/data/FileCache.h>
#include <arc/data/DataPoint.h>
#include <arc/data/DataMover.h>
#include <arc/StringConv.h>
#include <arc/Thread.h>
#include <arc/URL.h>
#include <arc/User.h>

#include "../jobs/job.h"
#include "../jobs/users.h"
#include "../files/info_types.h"
#include "../files/info_files.h"
#include "../files/delete.h"
#include "../conf/environment.h"
#include "../misc/proxy.h"
#include "../conf/conf_map.h"
#include "../conf/conf_cache.h"

//@
#define olog std::cerr
#define odlog(LEVEL) std::cerr
//@

/* maximum number of retries (for every source/destination) */
#define MAX_RETRIES 5
/* maximum number simultaneous downloads */
#define MAX_DOWNLOADS 5
/* maximum time for user files to upload (per file) */
#define MAX_USER_TIME 600

class PointPair;

class FileDataEx : public FileData {
 public:
  typedef std::list<FileDataEx>::iterator iterator;
  Arc::DataStatus res;
  std::string failure_description;
  PointPair* pair;
  FileDataEx(const FileData& f) : FileData(f),
				  res(Arc::DataStatus::Success),
				  pair(NULL) {}
  FileDataEx(const FileData& f,
	     Arc::DataStatus r,
	     const std::string& fd) : FileData(f),
				      res(r),
				      failure_description(fd),
				      pair(NULL) {}
};

static std::list<FileData> job_files_;
static std::list<FileDataEx> job_files;
static std::list<FileDataEx> processed_files;
static std::list<FileDataEx> failed_files;
static Arc::SimpleCondition pair_condition;
static int pairs_initiated = 0;

int clean_files(std::list<FileData> &job_files,char* session_dir) {
  std::string session(session_dir);
  if(delete_all_files(session,job_files,true) != 2) return 0;
  return 1;
}

class PointPair {
 public:
  Arc::DataPoint* source;
  Arc::DataPoint* destination;
  PointPair(const std::string& source_url,const std::string& destination_url):
                                                      source(Arc::DMC::GetDataPoint(source_url)),
                                                      destination(Arc::DMC::GetDataPoint(destination_url)) {};
  ~PointPair(void) { if(source) delete source; if(destination) delete destination; };
  static void callback(Arc::DataMover*,Arc::DataStatus res,const std::string&,void* arg) {
    FileDataEx::iterator &it = *((FileDataEx::iterator*)arg);
    pair_condition.lock();
    if(!res) {
      it->failure_description=(std::string)res;
      it->res=res;
      olog<<"Failed downloading file "<<it->lfn<<" - "<<it->failure_description<<std::endl;
      if((it->pair->source->GetTries() <= 0) || (it->pair->destination->GetTries() <= 0)) {
        delete it->pair; it->pair=NULL;
        failed_files.push_back(*it);
      } else {
        job_files.push_back(*it);
        olog<<"Retrying"<<std::endl;
      };
    } else {
      olog<<"Uploaded file "<<it->lfn<<std::endl;
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

void expand_files(std::list<FileData> &job_files,char* session_dir) {
  for(FileData::iterator i = job_files.begin();i!=job_files.end();) {
    std::string url = i->lfn;
    // Only ftp and gsiftp can be expanded to directories so far
    if(strncasecmp(url.c_str(),"ftp://",6) && 
       strncasecmp(url.c_str(),"gsiftp://",9)) { ++i; continue; };
    // user must ask explicitly
    if(url[url.length()-1] != '/') { ++i; continue; };
    std::string path(session_dir); path+="/"; path+=i->pfn;
    int l = strlen(session_dir) + 1;
    try {
      Glib::Dir dir(path);
      std::string file;
      for(;;) {
        file=dir.read_name();
        if(file.empty()) break;
        if(file == ".") continue;
        if(file == "..") continue;
        std::string path_ = path; path_+="/"; path+=file;
        struct stat st;
        if(lstat(path_.c_str(),&st) != 0) continue; // do not follow symlinks
        if(S_ISREG(st.st_mode)) {
          std::string lfn = url+file;
          job_files.push_back(FileData(path_.c_str()+l,lfn.c_str()));
        } else if(S_ISDIR(st.st_mode)) {
          std::string lfn = url+file+"/"; // cause recursive search
          job_files.push_back(FileData(path_.c_str()+l,lfn.c_str()));
        };
      };
      i=job_files.erase(i);
    } catch(Glib::FileError& e) {
      ++i;
    }; 
  };
}

int main(int argc,char** argv) {
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  int res=0;
  int n_threads = 1;
  int n_files = MAX_DOWNLOADS;
  /* used to find caches used by this user */
  std::string file_owner_username = "";
  uid_t file_owner = 0;
  gid_t file_group = 0;
  std::vector<struct Arc::CacheParameters> caches;
  bool use_conf_cache = false;
  unsigned long long int min_speed = 0;
  time_t min_speed_time = 300;
  unsigned long long int min_average_speed = 0;
  time_t max_inactivity_time = 300;
  bool secure = true;
  bool userfiles_only = false;
  bool passive = false;
  std::string failure_reason("");

  // process optional arguments
  for(;;) {
    opterr=0;
    int optc=getopt(argc,argv,"+hclpZfn:t:u:U:s:S:a:i:d:");
    if(optc == -1) break;
    switch(optc) {
      case 'h': {
        olog<<"Usage: uploader [-hclpZf] [-n files] [-t threads] [-U uid]"<<std::endl;
        olog<<"          [-u username] [-s min_speed] [-S min_speed_time]"<<std::endl;
        olog<<"          [-a min_average_speed] [-i min_activity_time]"<<std::endl;
        olog<<"          [-d debug_level] job_id control_directory"<<std::endl;
        olog<<"          session_directory [cache options]"<<std::endl; 
        exit(1);
      }; break;
      case 'c': {
        secure=false;
      }; break;
      case 'f': {
        use_conf_cache=true;
      }; break;
      case 'l': {
        userfiles_only=true;
      }; break;
      case 'p': {
        passive=true;
      }; break;
      case 'Z': {
        central_configuration=true;
      }; break;
      case 'd': {
//@        LogTime::Level(NotifyLevel(atoi(optarg)));
      }; break;
      case 't': {
        n_threads=atoi(optarg);
        if(n_threads < 1) {
          olog<<"Wrong number of threads"<<std::endl; exit(1);
        };
      }; break;
      case 'n': {
        n_files=atoi(optarg);
        if(n_files < 1) {
          olog<<"Wrong number of files"<<std::endl; exit(1);
        };
      }; break;
      case 'U': {
        unsigned int tuid;
        if(!Arc::stringto(std::string(optarg),tuid)) {
          olog<<"Bad number "<<optarg<<std::endl; exit(1);
        };
        struct passwd pw_;
        struct passwd *pw;
        char buf[BUFSIZ];
        getpwuid_r(tuid,&pw_,buf,BUFSIZ,&pw);
        if(pw == NULL) {
          olog<<"Wrong user name"<<std::endl; exit(1);
        };
        file_owner=pw->pw_uid;
        file_group=pw->pw_gid;
        if(pw->pw_name) file_owner_username=pw->pw_name;
        if((getuid() != 0) && (getuid() != file_owner)) {
          olog<<"Specified user can't be handled"<<std::endl; exit(1);
        };
      }; break;
      case 'u': {
        struct passwd pw_;
        struct passwd *pw;
        char buf[BUFSIZ];
        getpwnam_r(optarg,&pw_,buf,BUFSIZ,&pw);
        if(pw == NULL) {
          olog<<"Wrong user name"<<std::endl; exit(1);
        };
        file_owner=pw->pw_uid;
        file_group=pw->pw_gid;
        if(pw->pw_name) file_owner_username=pw->pw_name;
        if((getuid() != 0) && (getuid() != file_owner)) {
          olog<<"Specified user can't be handled"<<std::endl; exit(1);
        };
      }; break;
      case 's': {
        unsigned int tmpi;
        if(!Arc::stringto(std::string(optarg),tmpi)) {
          olog<<"Bad number "<<optarg<<std::endl; exit(1);
        };
        min_speed=tmpi;
      }; break;
      case 'S': {
        unsigned int tmpi;
        if(!Arc::stringto(std::string(optarg),tmpi)) {
          olog<<"Bad number "<<optarg<<std::endl; exit(1);
        };
        min_speed_time=tmpi;
      }; break;
      case 'a': {
        unsigned int tmpi;
        if(!Arc::stringto(std::string(optarg),tmpi)) {
          olog<<"Bad number "<<optarg<<std::endl; exit(1);
        };
        min_average_speed=tmpi;
      }; break;
      case 'i': {
        unsigned int tmpi;
        if(!Arc::stringto(std::string(optarg),tmpi)) {
          olog<<"Bad number "<<optarg<<std::endl; exit(1);
        };
        max_inactivity_time=tmpi;
      }; break;
      case '?': {
        olog<<"Unsupported option '"<<(char)optopt<<"'"<<std::endl;
        exit(1);
      }; break;
      case ':': {
        olog<<"Missing parameter for option '"<<(char)optopt<<"'"<<std::endl;
        exit(1);
      }; break;
      default: {
        olog<<"Undefined processing error"<<std::endl;
        exit(1);
      };
    };
  };
  // process required arguments
  char * id = argv[optind+0];
  if(!id) { olog << "Missing job id" << std::endl; return 1; };
  char* control_dir = argv[optind+1];
  if(!control_dir) { olog << "Missing control directory" << std::endl; return 1; };
  char* session_dir = argv[optind+2];
  if(!session_dir) { olog << "Missing session directory" << std::endl; return 1; };

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
      olog<<"Wrong user name"<<std::endl; exit(1);
    }
    if(pw->pw_name) file_owner_username=pw->pw_name;
  }
  
  if(use_conf_cache) {
    // use cache dir(s) from conf file
    try {
      CacheConfig * cache_config = new CacheConfig(std::string(file_owner_username));
      std::list<std::list<std::string> > conf_caches = cache_config->getCacheDirs();
      // add each cache to our list
      for (std::list<std::list<std::string> >::iterator i = conf_caches.begin(); i != conf_caches.end(); i++) {
        std::list<std::string>::iterator j = i->begin();
        struct Arc::CacheParameters cache_params;
        user.substitute(*j); cache_params.cache_path = (*j); j++;
        user.substitute(*j); cache_params.cache_job_dir_path = (*j); j++;
        if (j != i->end()) {user.substitute(*j); cache_params.cache_link_path = (*j);}
        else cache_params.cache_link_path = "";
        caches.push_back(cache_params);
      }
    }
    catch (CacheConfigException e) {
      olog<<"Error with cache configuration: "<<e.what()<<std::endl;
      olog<<"Cannot clean up any cache files"<<std::endl;
    }
  }
  else {
    if(argv[optind+3]) {
      struct Arc::CacheParameters cache_params;
      cache_params.cache_path = argv[optind+3];
      if(!argv[optind+4]) { olog << "Missing cache per-job dir" << std::endl; return 1; };
      cache_params.cache_job_dir_path = argv[optind+4];
      if (argv[optind+5]) cache_params.cache_link_path = argv[optind+5];
      else cache_params.cache_link_path = "";
      caches.push_back(cache_params);
    };
  }
  if(min_speed != 0) { olog<<"Minimal speed: "<<min_speed<<" B/s during "<<min_speed_time<<" s"<<std::endl; };
  if(min_average_speed != 0) { olog<<"Minimal average speed: "<<min_average_speed<<" B/s"<<std::endl; };
  if(max_inactivity_time != 0) { olog<<"Maximal inactivity time: "<<max_inactivity_time<<" s"<<std::endl; };

  prepare_proxy();

  if(n_threads > 10) {
    olog<<"Won't use more than 10 threads"<<std::endl;
    n_threads=10;
  };

  UrlMapConfig url_map;
  olog<<"Uploader started"<<std::endl;

  Arc::FileCache * cache;
  if(!caches.empty()) {
    cache = new Arc::FileCache(caches,std::string(id),uid,gid);
    if (!(*cache)) {
      olog << "Error creating cache: " << std::endl;
      exit(1);
    }
  }
  else {
    // if no cache defined, use null cache
    cache = new Arc::FileCache();
  }

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

  // get the list of output files
  if(!job_output_read_file(desc.get_id(),user,job_files_)) {
    failure_reason+="Internal error in uploader\n";
    olog << "Can't read list of output files" << std::endl; res=1; goto exit;
    //olog << "WARNING: Can't read list of output files - whole output will be removed" << std::endl;
  };

  // remove bad files
  if(clean_files(job_files_,session_dir) != 0) {
    failure_reason+="Internal error in uploader\n";
    olog << "Can't remove junk files" << std::endl; res=1; goto exit;
  };
  expand_files(job_files_,session_dir);
  for(std::list<FileData>::iterator i = job_files_.begin();i!=job_files_.end();++i) {
    job_files.push_back(*i);
  };
  // initialize structures to handle upload
  /* TODO: add threads=# to all urls if n_threads!=1 */
  desc.GetLocalDescription(user);
  // Main upload cycle
  if(!userfiles_only) for(;;) {
    // Initiate transfers
    int n = 0;
    pair_condition.lock();
    for(FileDataEx::iterator i=job_files.begin();i!=job_files.end();++i) {
      if(i->lfn.find(":") != std::string::npos) { /* is it lfn ? */
        ++n;
        if(n <= pairs_initiated) continue; // skip files being processed
        if(n > n_files) break; // quit if not allowed to process more
        /* have source and file to upload */
        std::string source;
        std::string destination = i->lfn;
        if(i->pair == NULL) {
          /* define place to store */
          std::string stdlog;
          JobLocalDescription* local = desc.get_local();
          if(local) stdlog=local->stdlog;
          if(stdlog.length() > 0) stdlog="/"+stdlog+"/";
          if((stdlog.length() > 0) &&
             (strncmp(stdlog.c_str(),i->pfn.c_str(),stdlog.length()) == 0)) {
            stdlog=i->pfn.c_str()+stdlog.length();
            source=std::string("file://")+control_dir+"/job."+id+"."+stdlog;
          } else {
            source=std::string("file://")+session_dir+i->pfn;
          };
          if(strncasecmp(destination.c_str(),"file://",7) == 0) {
            failure_reason+=std::string("User requested to store output locally ")+destination.c_str()+"\n";
            olog<<"FATAL ERROR: local destination for uploader"<<destination<<std::endl; res=1; goto exit;
          };
          PointPair* pair = new PointPair(source,destination);
          if(!(pair->source)) {
            failure_reason+=std::string("Can't accept URL ")+source.c_str()+"\n";
            olog<<"FATAL ERROR: can't accept URL: "<<source<<std::endl; res=1; goto exit;
          };
          if(!(pair->destination)) {
            failure_reason+=std::string("Can't accept URL ")+destination.c_str()+"\n";
            olog<<"FATAL ERROR: can't accept URL: "<<destination<<std::endl; res=1; goto exit;
          };
          i->pair=pair;
        };
        FileDataEx::iterator* it = new FileDataEx::iterator(i);
        if(!mover.Transfer(*(i->pair->source), *(i->pair->destination), *cache,
			   url_map, min_speed, min_speed_time,
			   min_average_speed, max_inactivity_time,
			   i->failure_description, &PointPair::callback, it,
			   i->pfn.c_str())) {
          failure_reason+=std::string("Failed to initiate file transfer: ")+source.c_str()+" - "+i->failure_description+"\n";
          olog<<"FATAL ERROR: Failed to initiate file transfer: "<<source<<" - "<<i->failure_description<<std::endl;
          delete it; res=1; goto exit;
        };
        ++pairs_initiated;
      };
    };
    if(pairs_initiated <= 0) { pair_condition.unlock(); break; }; // Looks like no more files to process
    // Processing initiated - now wait for event
    pair_condition.wait_nonblock();
    pair_condition.unlock();
  };
  // Print upload summary
  for(FileDataEx::iterator i=processed_files.begin();i!=processed_files.end();++i) {
    odlog(INFO)<<"Uploaded "<<i->lfn<<std::endl;
  };
  for(FileDataEx::iterator i=failed_files.begin();i!=failed_files.end();++i) {
    odlog(ERROR)<<"Failed to upload "<<i->lfn<<std::endl;
    failure_reason+="Input file: "+i->lfn+" - "+(std::string)(i->res)+"\n";
    if(i->res == Arc::DataStatus::CredentialsExpiredError)
      credentials_expired=true;
    transfered=false;
  };
  // Check if all files have been properly downloaded
  if(!transfered) {
    odlog(INFO)<<"Some uploads failed"<<std::endl; res=2;
    if(credentials_expired) res=3;
    goto exit;
  };
  /* all files left should be kept */
  for(FileDataEx::iterator i=job_files.begin();i!=job_files.end();) {
    i->lfn=""; ++i;
  };
  if(!userfiles_only) {
    job_files_.clear();
    for(FileDataEx::iterator i = job_files.begin();i!=job_files.end();++i) job_files_.push_back(*i);
    if(!job_output_write_file(desc,user,job_files_)) {
      olog << "WARNING: Failed writing changed output file." << std::endl;
    };
  };
exit:
  // release input files used for this job
  cache->Release();
  olog << "Leaving uploader ("<<res<<")"<<std::endl;
  // clean uploaded files here 
  job_files_.clear();
  for(FileDataEx::iterator i = job_files.begin();i!=job_files.end();++i) job_files_.push_back(*i);
  clean_files(job_files_,session_dir);
  remove_proxy();
  if(res != 0) {
    job_failed_mark_add(desc,user,failure_reason);
  };
  return res;
}

