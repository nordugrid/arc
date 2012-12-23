#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>

#include <arc/ArcLocation.h>
#include <arc/FileUtils.h>
#include <arc/Logger.h>
#include <arc/Watchdog.h>
#include "jobs/JobsList.h"
#include "jobs/CommFIFO.h"
#include "log/JobLog.h"
#include "run/RunRedirected.h"
#include "run/RunParallel.h"
#include "jobs/DTRGenerator.h"
#include "../delegation/DelegationStore.h"
#include "../delegation/DelegationStores.h"

#include "GridManager.h"

namespace ARex {

/* do job cleaning every 2 hours */
#define HARD_JOB_PERIOD 7200

/* cache cleaning every 5 minutes */
#define CACHE_CLEAN_PERIOD 300

/* cache cleaning default timeout */
#define CACHE_CLEAN_TIMEOUT 3600

#define DEFAULT_LOG_FILE "/var/log/arc/grid-manager.log"
#define DEFAULT_PID_FILE "/var/run/grid-manager.pid"

static Arc::Logger logger(Arc::Logger::getRootLogger(),"A-REX");

class cache_st {
 public:
  Arc::SimpleCounter counter;
  Arc::SimpleCondition to_exit;
  const GMConfig* config;
  cache_st(GMConfig* config_):config(config_) {
  };
  ~cache_st(void) {
    to_exit.signal();
    counter.wait();
  };
};

static void cache_func(void* arg) {
  const GMConfig* config = ((cache_st*)arg)->config;
  Arc::SimpleCondition& to_exit = ((cache_st*)arg)->to_exit;
  
  CacheConfig cache_info(config->CacheParams());
  if (!cache_info.cleanCache()) return;
  // Note: per-user substitutions do not work here. If they are used
  // cache-clean must be run manually eg via cron
  cache_info.substitute(*config, Arc::User());

  // get the cache dirs
  std::vector<std::string> cache_info_dirs = cache_info.getCacheDirs();
  if (cache_info_dirs.empty()) return;

  // in arc.conf % of used space is given, but cache-clean uses % of free space
  std::string minfreespace = Arc::tostring(100-cache_info.getCacheMax());
  std::string maxfreespace = Arc::tostring(100-cache_info.getCacheMin());
  std::string cachelifetime = cache_info.getLifeTime();
  std::string logfile = cache_info.getLogFile();

  // do cache-clean -h for explanation of options
  std::string cmd = Arc::ArcLocation::GetToolsDir() + "/cache-clean";
  cmd += " -m " + minfreespace;
  cmd += " -M " + maxfreespace;
  if (!cachelifetime.empty()) cmd += " -E " + cachelifetime;
  cmd += " -D " + cache_info.getLogLevel();
  std::vector<std::string> cache_dirs;
  for (std::vector<std::string>::iterator i = cache_info_dirs.begin(); i != cache_info_dirs.end(); i++) {
    cmd += " " + (i->substr(0, i->find(" ")));
  }

  // use large timeout, as disk scan can take a long time
  // blocks until command finishes or timeout
  int clean_timeout = cache_info.getCleanTimeout();
  if (clean_timeout == 0) clean_timeout = CACHE_CLEAN_TIMEOUT;

  // run cache cleaning periodically forever
  for(;;) {

    int h = open(logfile.c_str(), O_WRONLY | O_APPEND);
    if (h < 0) {
      std::string dirname(logfile.substr(0, logfile.rfind('/')));
      if (!dirname.empty() && !Arc::DirCreate(dirname, S_IRWXU | S_IRGRP | S_IROTH | S_IXGRP | S_IXOTH, true)) {
        logger.msg(Arc::WARNING, "Cannot create directories for log file %s."
            " Messages will be logged to this log", logfile);
      }
      else {
        h = open(logfile.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (h < 0) {
          logger.msg(Arc::WARNING, "Cannot open cache log file %s: %s. Cache cleaning"
              " messages will be logged to this log", logfile, Arc::StrError(errno));
        }
      }
    }

    logger.msg(Arc::DEBUG, "Running command %s", cmd);
    int result = RunRedirected::run(Arc::User(), "cache-clean", -1, h, h, cmd.c_str(), clean_timeout);
    close(h);
    if (result != 0) {
      if (result == -1) logger.msg(Arc::ERROR, "Failed to start cache clean script");
      else logger.msg(Arc::ERROR, "Cache cleaning script failed");
    }
    if (to_exit.wait(CACHE_CLEAN_PERIOD*1000)) {
      break;
    }
  }
}

class sleep_st {
 public:
  Arc::SimpleCondition* sleep_cond;
  CommFIFO* timeout;
  std::string control_dir;
  bool to_exit; // tells thread to exit
  bool exited;  // set by thread while exiting
  sleep_st(const std::string& control):sleep_cond(NULL),timeout(NULL),control_dir(control),to_exit(false),exited(false) {
  };
  ~sleep_st(void) {
    to_exit = true;
    SignalFIFO(control_dir);
    while(!exited) sleep(1);
  };
};

static void wakeup_func(void* arg) {
  sleep_st* s = (sleep_st*)arg;
  for(;;) {
    if(s->to_exit) break;
    s->timeout->wait();
    if(s->to_exit) break;
    s->sleep_cond->signal();
    if(s->to_exit) break;
  };
  s->exited = true;
  return;
}

static void kick_func(void* arg) {
  sleep_st* s = (sleep_st*)arg;
  s->sleep_cond->signal();
}

typedef struct {
  int argc;
  char** argv;
} args_st;

void GridManager::grid_manager(void* arg) {
  GridManager* gm = (GridManager*)arg;
  if(!arg) {
    ::kill(::getpid(),SIGTERM);
    return;
  }
  if(!gm->thread()) {
    // thread exited because of internal error
    // that means whole server must be stopped 
    ::kill(::getpid(),SIGTERM);
  }
}

bool GridManager::thread() {
  logger.msg(Arc::INFO,"Starting grid-manager thread");

  logger.msg(Arc::INFO,"Used configuration file %s",config_.ConfigFile());
  config_.Print();

  // Preparing various structures, dirs, etc.
  wakeup_interface_ = new CommFIFO;
  time_t hard_job_time; 
  CommFIFO::add_result r = wakeup_interface_->add(config_.ControlDir());
  if(r != CommFIFO::add_success) {
    if(r == CommFIFO::add_busy) {
      logger.msg(Arc::FATAL,"Error adding communication interface in %s. "
          "Maybe another instance of A-REX is already running.",config_.ControlDir());
    } else {
      logger.msg(Arc::FATAL,"Error adding communication interface in %s. "
          "Maybe permissions are not suitable.",config_.ControlDir());
    };
    return false;
  };
  ARex::DelegationStores* delegs = config_.Delegations();
  if(delegs) {
    ARex::DelegationStore& deleg = (*delegs)[config_.DelegationDir()];
    if(!deleg) {
      logger.msg(Arc::FATAL,"Error initiating delegation database in %s. "
          "Maybe permissions are not suitable. Returned error is: %s.", config_.DelegationDir(),deleg.Error());
      return false;
    };
  };
  wakeup_interface_->timeout(config_.WakeupPeriod());

  /* start timer thread - wake up every 2 minutes */
  wakeup_ = new sleep_st(config_.ControlDir());
  wakeup_->sleep_cond=sleep_cond_;
  wakeup_->timeout=wakeup_interface_;
  if(!Arc::CreateThreadFunction(wakeup_func,wakeup_)) {
    logger.msg(Arc::ERROR,"Failed to start new thread");
    wakeup_->exited = true;
    return false;
  };
  RunParallel::kicker(&kick_func,wakeup_);
  /*
  if(clean_first_level) {
    bool clean_finished = false;
    bool clean_active = false;
    bool clean_junk = false;
    if(clean_first_level >= 1) {
      clean_finished=true;
      if(clean_first_level >= 2) {
        clean_active=true;
        if(clean_first_level >= 3) {
          clean_junk=true;
        };
      };
    };
    for(;;) { 
      bool cleaned_all=true;
      for(JobUsers::iterator user = users_->begin();user != users_->end();++user) {
        size_t njobs = user->get_jobs()->size();
        user->get_jobs()->ScanNewJobs();
        if(user->get_jobs()->size() == njobs) break;
        cleaned_all=false;
        if(!(user->get_jobs()->DestroyJobs(clean_finished,clean_active)))  {
          logger.msg(Arc::WARNING,"Not all jobs are cleaned yet");
          sleep(10); 
          logger.msg(Arc::WARNING,"Trying again");
        };
        kill(getpid(),SIGCHLD);  // make sure no child is missed
      };
      if(cleaned_all) {
        if(clean_junk && clean_active && clean_finished) {  
          // at the moment cleaning junk means cleaning all the files in 
          // session and control directories
          for(JobUsers::iterator user=users_->begin();user!=users_->end();++user) {
            std::list<FileData> flist;
            for(std::vector<std::string>::const_iterator i = user->SessionRoots().begin(); i != user->SessionRoots().end(); i++) {
              logger.msg(Arc::INFO,"Cleaning all files in directory %s", *i);
              delete_all_files(*i,flist,true);
            }
            logger.msg(Arc::INFO,"Cleaning all files in directory %s", user->ControlDir());
            delete_all_files(user->ControlDir(),flist,true);
          };
        };
        break;
      };
    };
    logger.msg(Arc::INFO,"Jobs cleaned");
  };
  */
  // check if cleaning is enabled, if so activate cleaning thread
  if (!config_.CacheParams().getCacheDirs().empty() && config_.CacheParams().cleanCache()) {
    cache_st cache_h(&config_);
    if(!Arc::CreateThreadFunction(cache_func,&cache_h,&cache_h.counter)) {
      logger.msg(Arc::INFO,"Failed to start new thread: cache won't be cleaned");
    }
  }
  // Start new job list
  JobsList jobs(config_);
  logger.msg(Arc::INFO,"Picking up left jobs");
  jobs.RestartJobs();

  hard_job_time = time(NULL) + HARD_JOB_PERIOD;
  if (config_.UseDTR()) {
    logger.msg(Arc::INFO, "Starting data staging threads");
    DTRGenerator* dtr_generator = new DTRGenerator(config_, &kick_func, wakeup_);
    if (!(*dtr_generator)) {
      delete dtr_generator;
      logger.msg(Arc::ERROR, "Failed to start data staging threads, exiting Grid Manager thread");
      return false;
    }
    dtr_generator_ = dtr_generator;
    jobs.SetDataGenerator(dtr_generator);
  }
  bool scan_old = false;
  std::string heartbeat_file("gm-heartbeat");
  Arc::WatchdogChannel wd(config_.WakeupPeriod()*3+300);
  /* main loop - forever */
  logger.msg(Arc::INFO,"Starting jobs' monitoring");
  for(;;) {
    if(tostop_) break;
    config_.RunHelpers();
    config_.GetJobLog()->RunReporter(config_);
    bool hard_job = ((int)(time(NULL) - hard_job_time)) > 0;
    // touch heartbeat file
    std::string gm_heartbeat(std::string(config_.ControlDir() + "/" + heartbeat_file));
    int r = ::open(gm_heartbeat.c_str(), O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
    if (r < 0) {
      logger.msg(Arc::WARNING, "Failed to open heartbeat file %s", gm_heartbeat);
    } else {
      close(r); r = -1;
    };
    // touch temporary configuration so /tmp cleaner does not erase it
    if(config_.ConfigIsTemp()) ::utimes(config_.ConfigFile().c_str(), NULL);
    wd.Kick();
    /* check for new marks and activate related jobs */
    jobs.ScanNewMarks();
    /* look for new jobs */
    jobs.ScanNewJobs();
    /* slowly scan through old jobs for deleting them in time */
    if(hard_job || scan_old) {
      scan_old = jobs.ScanOldJobs(config_.WakeupPeriod()/2,config_.MaxJobs());
    };
    /* process known jobs */
    jobs.ActJobs();
    // Clean old delegations
    ARex::DelegationStores* delegs = config_.Delegations();
    if(delegs) {
      ARex::DelegationStore& deleg = (*delegs)[config_.DelegationDir()];
      deleg.Expiration(24*60*60);
      deleg.CheckTimeout(60);
      deleg.PeriodicCheckConsumers();
    };
    if(hard_job) hard_job_time = time(NULL) + HARD_JOB_PERIOD;
    sleep_cond_->wait();
    logger.msg(Arc::DEBUG,"Waking up");
  };
  // Waiting for children to finish
  logger.msg(Arc::INFO,"Stopping jobs processing thread");
  config_.PrepareToDestroy();
  jobs.PrepareToDestroy();
  logger.msg(Arc::INFO,"Destroying jobs and waiting for underlying processes to finish");
  return true;
}

GridManager::GridManager(GMConfig& config):tostop_(false), config_(config) {
  sleep_cond_ = new Arc::SimpleCondition;
  wakeup_interface_ = NULL;
  wakeup_ = NULL;
  dtr_generator_ = NULL;
  if(!Arc::CreateThreadFunction(&grid_manager,(void*)this,&active_)) { };
}

GridManager::~GridManager(void) {
  logger.msg(Arc::INFO, "Shutting down job processing");
  // Tell main thread to stop
  tostop_ = true;
  // Stop data staging
  if (dtr_generator_) {
    logger.msg(Arc::INFO, "Shutting down data staging threads");
    delete dtr_generator_;
  }
  // Wait for main thread
  while(true) {
    sleep_cond_->signal();
    if(active_.wait(1000)) break;
  }
  // wakeup_ is used by users through RunParallel and by 
  // dtr_generator. Hence it must be deleted almost last.
  if(wakeup_) delete wakeup_;
  // wakeup_interface_ and sleep_cond_ are used by wakeup_
  if(wakeup_interface_) delete wakeup_interface_;
  delete sleep_cond_;
}

} // namespace ARex

