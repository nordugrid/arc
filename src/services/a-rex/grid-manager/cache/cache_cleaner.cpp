#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <list>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/statvfs.h>

#include <unistd.h>
#include <dirent.h>
#include <iostream>

#include <arc/Logger.h>
#include "../jobs/users.h"
#include "../conf/conf_file.h"
#include "../conf/environment.h"
#include "cache.h"
#include "../files/info_files.h"
#include "cache_cleaner.h"

static Arc::Logger& logger = Arc::Logger::getRootLogger();

#ifdef __CREATE_EXEC__
int main(void) {
  logger.addDestination(*(new Arc::LogStream(std::cerr)));
  logger.setThreshold(Arc::WARNING);
  if(!read_env_vars()) exit(1);
  JobUsers users;
  std::string my_username("");
  uid_t my_uid=getuid();
  JobUser *my_user = NULL;
  {
    struct passwd pw_;
    struct passwd *pw;
    char buf[BUFSIZ];
    getpwuid_r(my_uid,&pw_,buf,BUFSIZ,&pw);
    if(pw != NULL) { my_username=pw->pw_name; };
  };
  if(my_username.length() == 0) {
    logger.msg(Arc::ERROR,"Cache: Can't recognize myself."); exit(1);
  };
  my_user = new JobUser(my_username);
  if(!configure_serviced_users(users,my_uid,my_username,*my_user)) {
    std::cout<<"Error processing configuration."<<std::endl; exit(1);
  };
  if(users.size() == 0) {
    std::cout<<"No suitable users found in configuration."<<std::endl; exit(1);
  };
#else
int cache_cleaner(const JobUsers &users) {
#endif
  /* clean cache */
  for(JobUsers::const_iterator user = users.begin();user != users.end();++user) {
    std::string cache_dir = user->CacheDir();
    std::string cache_data_dir = user->CacheDataDir();
    if(cache_dir.length() == 0) continue;
    bool private_cache = user->CachePrivate();
    long long int cache_max = user->CacheMaxSize();
    long long int cache_min = user->CacheMinSize();
    long long int available = 0;
    long long int used = 0;
    long long int space_hardfree  = 0;
    long long int space_softfree  = 0;
    long long int space_hardsize  = 0;
    long long int space_softsize  = 0;
    long long int space_unclaimed = 0;
    long long int space_claimed   = 0;

    // logger.msg(Arc::ERROR,"Cache directory: %s",cache_dir.c_str());
    // logger.msg(Arc::ERROR,"Cache data directory: %s",cache_data_dir.c_str());

    /* get available space */
    struct statvfs dst;
    if(statvfs((char*)(cache_data_dir.c_str()),&dst) != 0) {
      logger.msg(Arc::ERROR,"Cache: can't obtain info about file system at %s",cache_data_dir.c_str());
      if((cache_max < 0) || (cache_min < 0)) continue;
      available=0;
      space_hardsize=0;
    }
    else {
      available = (long long int)dst.f_bavail * (dst.f_frsize ? dst.f_frsize : dst.f_bsize);
      space_hardsize = (long long int)dst.f_blocks * (dst.f_frsize ? dst.f_frsize : dst.f_bsize);
    };
    // logger.msg(Arc::ERROR,"Cache: space available: %lli bytes",available);
    space_hardfree=available;
    space_softfree=space_hardfree;
    space_softsize=space_hardsize;
    if(cache_max < 0) {
      space_softfree+=cache_max;
      space_softsize+=cache_max;
    };
    if(cache_max > 0) {
      space_softsize=cache_max;
    };

    if((cache_max == 0) && (cache_min == 0)) {
      /* write statistics to file */
      std::string fname=cache_dir+"/statistics";
      std::ofstream f(fname.c_str());
      f<<"hardfree="<<space_hardfree<<std::endl;
      f<<"softfree="<<space_softfree<<std::endl;
      f<<"hardsize="<<space_hardsize<<std::endl;
      f<<"softsize="<<space_softsize<<std::endl;
//      continue;
    }
    else {
      // logger.msg(Arc::ERROR,"Cache max: %lli",cache_max);
      // logger.msg(Arc::ERROR,"Cache min: %lli",cache_min);
    };
    used=0;
//    if((cache_max > 0) || ((available < (-cache_max))  && (cache_min > 0))) { /* get used space */
/*
      logger.msg(Arc::ERROR,"Cache: obtaining used space");
      * scan directory *
      DIR* dir;
      dir=opendir(cache_data_dir.c_str());
      if(dir == NULL) {
        logger.msg(Arc::ERROR,"Cache: can't open directory %s",cache_dir.c_str());
        continue;
      };
      struct dirent file_;
      struct dirent *file;
      for(;;) {
        readdir_r(dir,&file_,&file);
        if(file == NULL) break;
        std::string fname(cache_data_dir.c_str());
        fname+="/";
        fname+=file->d_name;
        struct stat fst;
        if(stat(fname.c_str(),&fst) == 0) {
          if(S_ISREG(fst.st_mode)) {
            logger.msg(Arc::ERROR,"File %s/%s is using %i",cache_data_dir.c_str(),file->d_name,((fst.st_size+fst.st_blksize-1)/fst.st_blksize)*fst.st_blksize);
            used+=((fst.st_size+fst.st_blksize-1)/fst.st_blksize)*fst.st_blksize;
          };
        };
      };
      closedir(dir);
*/
//    };
    uid_t cache_uid = 0;
    gid_t cache_gid = 0;
    if(private_cache) {
      cache_uid=user->get_uid();
      cache_gid=user->get_gid();
    };
    std::list<std::string> cache_files;
    if(cache_files_list(cache_dir.c_str(),cache_uid,cache_gid,cache_files)==0) {
      std::list<JobId> ids;
      for(std::list<std::string>::iterator f = cache_files.begin();
                                         f != cache_files.end();++f) {
        unsigned long long int f_data_size=0;
        unsigned long long int f_info_size=0;
        unsigned long long int f_claim_size=0;
        std::string fname;
        struct stat fst;
        fname=cache_data_dir + "/" + (*f);
        if(stat(fname.c_str(),&fst) == 0) {
          if(S_ISREG(fst.st_mode)) {
            f_data_size = 
               ((fst.st_size+fst.st_blksize-1)/fst.st_blksize)*fst.st_blksize;
          };
        };
        fname=cache_dir + "/" + (*f) + ".info";
        if(stat(fname.c_str(),&fst) == 0) {
          if(S_ISREG(fst.st_mode)) {
            f_info_size =
               ((fst.st_size+fst.st_blksize-1)/fst.st_blksize)*fst.st_blksize;
          };
        };
        fname=cache_dir + "/" + (*f) + ".claim";
        if(stat(fname.c_str(),&fst) == 0) {
          if(S_ISREG(fst.st_mode)) {
            f_claim_size =
               ((fst.st_size+fst.st_blksize-1)/fst.st_blksize)*fst.st_blksize;
          };
        };
        used+=f_data_size+f_info_size+f_claim_size;
        if(f_claim_size == 0) {
          space_unclaimed+=f_data_size+f_info_size+f_claim_size;
        } else {
          space_claimed+=f_data_size+f_info_size+f_claim_size;
        };
      };
    };
    // logger.msg(Arc::ERROR,"Cache: used %lli bytes",used);
    if(cache_max > 0) space_softfree=cache_max-used;
    long long int remove_size = 0;
    bool need_remove = false;
    if(cache_max > 0) {
      if(used > cache_max) need_remove=true;
    }
    else if(cache_max < 0) {
      if(available < (-cache_max)) need_remove=true;
    };
    if(need_remove) {
      if(cache_min > 0) {
        remove_size=used-cache_min;
      }
      else if(cache_min < 0) {
        remove_size=(-cache_min)-available;
      };
      if(remove_size <= 0) remove_size=1;
      logger.msg(Arc::ERROR,"Cache: have to remove %lli bytes",remove_size);
      /* clean cache from lost jobs */
      std::list<JobId> claiming_jobs;
      if(cache_files_list(cache_dir.c_str(),cache_uid,cache_gid,cache_files) 
                                            != 0) {
        logger.msg(Arc::WARNING,"Cache: Warning: can't obtain list of cache files");
      }
      else {
        std::list<JobId> ids;
        for(std::list<std::string>::iterator f = cache_files.begin();
                                         f != cache_files.end();++f) {
          cache_claiming_list(cache_dir.c_str(),(*f).c_str(),ids);
        };
        if(ids.size() > 0) {
          /* go through all users with same cache directory and
             list their jobs */
          for(JobUsers::const_iterator user_ = users.begin();
                                       user_ != users.end();++user_) {
            if(cache_dir == user_->CacheDir()) {
              struct dirent file_;
              struct dirent *file;
              std::string cdir=user_->ControlDir();
              DIR *dir=opendir(cdir.c_str());
              if(dir == NULL) {
                logger.msg(Arc::WARNING,"Cache: Warning: Failed reading control directory: %s",cdir.c_str());
                logger.msg(Arc::WARNING,"Cache: Warning: Stale locks won't be removed");
                ids.clear();
                break;
              }
              else {
                for(;;) {
                  readdir_r(dir,&file_,&file);
                  if(file == NULL) break;
                  int l=strlen(file->d_name);
                  if(l>(4+7)) {
                    if(!strncmp(file->d_name,"job.",4)) {
                      if(!strncmp((file->d_name)+(l-7),".status",7)) {
                        std::string id((file->d_name)+4,l-7-4);
                        job_state_t state=job_state_read_file(id,*user_);
                        if(state != JOB_STATE_FINISHED) {
                          for(std::list<JobId>::iterator i=ids.begin();
                                                       i!=ids.end();++i) {
                            if((*i) == id) { ids.erase(i); break; };
                          };
                        };
                      };
                    };
                  };
                };
              };
              closedir(dir);
            };
          };
          for(std::list<JobId>::iterator i=ids.begin();i!=ids.end();++i) {
            logger.msg(Arc::ERROR,"Cache: job %s does not exist or has finished, removing stale locks",i->c_str());
            cache_release_url(cache_dir.c_str(),cache_data_dir.c_str(),cache_uid,cache_gid,*i,false);
          };
        };
      };
      unsigned long long int removed_size = 
          cache_clean(cache_dir.c_str(),cache_data_dir.c_str(),cache_uid,cache_gid,remove_size);
      logger.msg(Arc::ERROR,"Cache: in %s - %s requested to remove %lli, removed %lli",cache_dir.c_str(),cache_data_dir.c_str(),remove_size,removed_size);
      space_hardfree+=removed_size;
      space_softfree+=removed_size;
      if(space_unclaimed<removed_size) {
        space_unclaimed=0;
      } else {
        space_unclaimed-=removed_size;
      };
    };
    /* write statistics to file */
    {
      std::string fname=cache_dir+"/statistics";
      std::ofstream f(fname.c_str());
      f<<"hardfree="<<space_hardfree<<std::endl;
      f<<"softfree="<<space_softfree<<std::endl;
      f<<"hardsize="<<space_hardsize<<std::endl;
      f<<"softsize="<<space_softsize<<std::endl;
      f<<"claimed="<<space_claimed<<std::endl;
      f<<"unclaimed="<<space_unclaimed<<std::endl;
    };
  };
#ifdef __CREATE_EXEC__
  sleep(120);
#endif
  return 0;
}

