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
#include "log/JobsMetrics.h"
#include "log/HeartBeatMetrics.h"
#include "log/SpaceMetrics.h"
#include "run/RunRedirected.h"
#include "run/RunParallel.h"
#include "files/ControlFileHandling.h"
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

  std::string maxusedspace = Arc::tostring(cache_info.getCacheMax());
  std::string minusedspace = Arc::tostring(cache_info.getCacheMin());
  std::string cachelifetime = cache_info.getLifeTime();
  std::string logfile = cache_info.getLogFile();
  bool cacheshared = cache_info.getCacheShared();
  std::string cachespacetool = cache_info.getCacheSpaceTool();

  // do cache-clean -h for explanation of options
  std::string cmd = Arc::ArcLocation::GetToolsDir() + "/cache-clean";
  cmd += " -m " + minusedspace;
  cmd += " -M " + maxusedspace;
  if (!cachelifetime.empty()) cmd += " -E " + cachelifetime;
  if (cacheshared) cmd += " -S ";
  if (!cachespacetool.empty()) cmd += " -f \"" + cachespacetool + "\" ";
  cmd += " -D " + cache_info.getLogLevel();
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
    if (h == -1) {
      std::string dirname(logfile.substr(0, logfile.rfind('/')));
      if (!dirname.empty() && !Arc::DirCreate(dirname, S_IRWXU | S_IRGRP | S_IROTH | S_IXGRP | S_IXOTH, true)) {
        logger.msg(Arc::WARNING, "Cannot create directories for log file %s."
            " Messages will be logged to this log", logfile);
      }
      else {
        h = open(logfile.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (h == -1) {
          logger.msg(Arc::WARNING, "Cannot open cache log file %s: %s. Cache cleaning"
              " messages will be logged to this log", logfile, Arc::StrError(errno));
        }
      }
    }

    logger.msg(Arc::DEBUG, "Running command %s", cmd);
    int result = RunRedirected::run(Arc::User(), "cache-clean", -1, h, h, cmd.c_str(), clean_timeout);
    if(h != -1) close(h);
    if (result != 0) {
      if (result == -1) logger.msg(Arc::ERROR, "Failed to start cache clean script");
      else logger.msg(Arc::ERROR, "Cache cleaning script failed");
    }
    if (to_exit.wait(CACHE_CLEAN_PERIOD*1000)) {
      break;
    }
  }
}

/*!!
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
    CommFIFO::Signal(control_dir);
    while(!exited) sleep(1);
  };
};
*/

class WakeupInterface: protected Arc::Thread, public CommFIFO {
 public:
  WakeupInterface(JobsList& jobs);
  ~WakeupInterface();
  bool start();
 protected:
  void thread();
  JobsList& jobs_;
  bool to_exit; // tells thread to exit
  bool exited;  // set by thread while exiting
};

WakeupInterface::WakeupInterface(JobsList& jobs): jobs_(jobs), to_exit(false), exited(true) {
}

WakeupInterface::~WakeupInterface() {
  to_exit = true;
  CommFIFO::kick();
  while(!exited) {
    sleep(1);
    CommFIFO::kick();
  }
}

bool WakeupInterface::start() {
  // No need to do locking because this method is
  // always called from single thread
  if(!exited) return false;
  exited = !Arc::Thread::start();
  return !exited;
}

void WakeupInterface::thread() {
  for(;;) {
    if(to_exit) break; // request to stop
    std::string event;
    bool has_event = CommFIFO::wait(event);
    if(to_exit) break; // request to stop
    if(has_event) {
      // Event arrived
      if(!event.empty()) {
        // job id provided
        logger.msg(Arc::DEBUG, "External request for attention %s", event);
        jobs_.RequestAttention(event);
      } else {
        // generic kick
        jobs_.RequestAttention();
      };
    } else {
      // timeout - use as timer
      jobs_.RequestAttention();
    };
  };
  exited = true;
}

void touch_heartbeat(const std::string& dir, const std::string& file) {
  std::string gm_heartbeat(dir + "/" + file);
  int r = ::open(gm_heartbeat.c_str(), O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
  if (r == -1) {
    logger.msg(Arc::WARNING, "Failed to open heartbeat file %s", gm_heartbeat);
  } else {
    ::close(r); r = -1;
  };
}


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

  logger.msg(Arc::INFO,"Starting jobs processing thread");
  logger.msg(Arc::INFO,"Used configuration file %s",config_.ConfigFile());
  config_.Print();

  // Preparing various structures, dirs, etc.
  ARex::DelegationStores* delegs = config_.GetDelegations();
  if(delegs) {
    ARex::DelegationStore& deleg = (*delegs)[config_.DelegationDir()];
    if(!deleg) {
      logger.msg(Arc::FATAL,"Error initiating delegation database in %s. "
          "Maybe permissions are not suitable. Returned error is: %s.", config_.DelegationDir(),deleg.Error());
      return false;
    };
  };

  /* start timer thread - wake up every 2 minutes */
  // TODO: use timed wait instead of dedicated thread
  // check if cache cleaning is enabled, if so activate cleaning thread
  cache_st cache_h(&config_);
  if (!config_.CacheParams().getCacheDirs().empty() && config_.CacheParams().cleanCache()) {
    if(!Arc::CreateThreadFunction(cache_func,&cache_h,&cache_h.counter)) {
      logger.msg(Arc::INFO,"Failed to start new thread: cache won't be cleaned");
    }
  }

  // Start new job list
  JobsList jobs(config_);
  if(!jobs) {
    logger.msg(Arc::ERROR, "Failed to activate Jobs Processing object, exiting Grid Manager thread");
    return false;
  }

  // Setup listening for job attention requests
  WakeupInterface wakeup_interface_(jobs);
  CommFIFO::add_result r = wakeup_interface_.add(config_.ControlDir());
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
  wakeup_interface_.timeout(config_.WakeupPeriod());
  if(!wakeup_interface_.start()) {
    logger.msg(Arc::ERROR,"Failed to start new thread for monitoring job requests");
    return false;
  };  

  // Start jobs processing
  jobs_ = &jobs;
  logger.msg(Arc::INFO,"Picking up left jobs");
  jobs.RestartJobs();

  logger.msg(Arc::INFO, "Starting data staging threads");
  std::string heartbeat_file("gm-heartbeat");
  Arc::WatchdogChannel wd(config_.WakeupPeriod()*3+300);
  /* main loop - forever */
  logger.msg(Arc::INFO,"Starting jobs' monitoring");
  time_t poll_job_time = time(NULL); // run once immediately + config_.WakeupPeriod();
  for(;;) {
    if(tostop_) break;
    // TODO: make processing of SSH async or remove SSH from GridManager completely
    if (config_.UseSSH()) {
      // TODO: can there be more than one session root?
      while (!config_.SSHFS_OK(config_.SessionRoots().front())) {
        logger.msg(Arc::WARNING, "SSHFS mount point of session directory (%s) is broken - waiting for reconnect ...", config_.SessionRoots().front());
        active_.wait(10000);
      }
      while(!config_.SSHFS_OK(config_.RTEDir())) {
        logger.msg(Arc::WARNING, "SSHFS mount point of runtime directory (%s) is broken - waiting for reconnect ...", config_.RTEDir());
        active_.wait(10000);
      }
      // TODO: can there be more than one cache dir?
      while(!config_.SSHFS_OK(config_.CacheParams().getCacheDirs().front())) {
        logger.msg(Arc::WARNING, "SSHFS mount point of cache directory (%s) is broken - waiting for reconnect ...", config_.CacheParams().getCacheDirs().front());
        active_.wait(10000);
      }
    }
    // TODO: check conditions for following calls
    JobLog* joblog = config_.GetJobLog();
    if(joblog) {
      // run jura reporter if enabled
      if (joblog->ReporterEnabled()){
        joblog->RunReporter(config_);
      }
    }
    JobsMetrics* metrics = config_.GetJobsMetrics();
    if(metrics) metrics->Sync();
    // Process jobs which need attention ASAP
    jobs.ActJobsAttention();
    if(((int)(time(NULL) - poll_job_time)) >= 0) {
      // Polling time
      poll_job_time = time(NULL) + config_.WakeupPeriod();

      // touch heartbeat file
      touch_heartbeat(config_.ControlDir(), heartbeat_file);
      // touch temporary configuration so /tmp cleaner does not erase it
      if(config_.ConfigIsTemp()) ::utimes(config_.ConfigFile().c_str(), NULL);
      // Tell watchdog we are alive
      wd.Kick();
      /* check for new marks and activate related jobs - TODO: remove */
      jobs.ScanNewMarks();
      /* look for new jobs - TODO: remove */
      jobs.ScanNewJobs();
      /* process jobs which do not get attention calls in their current state */
      jobs.ActJobsPolling();
      //jobs.ActJobs();
      // Clean old delegations
      ARex::DelegationStores* delegs = config_.GetDelegations();
      if(delegs) {
        ARex::DelegationStore& deleg = (*delegs)[config_.DelegationDir()];
        deleg.Expiration(24*60*60);
        deleg.CheckTimeout(60); // During this time delegation database will be locked. So it must not be too long.
        deleg.PeriodicCheckConsumers();
        // once in a while check for delegations which are locked by non-exiting jobs
        std::list<std::string> lock_ids;
        if(deleg.GetLocks(lock_ids)) {
          for(std::list<std::string>::iterator lock_id = lock_ids.begin(); lock_id != lock_ids.end(); ++lock_id) {
            time_t t = job_state_time(*lock_id,config_);
            // Returns zero if file is not present
            if(t == 0) {
              logger.msg(Arc::ERROR,"Orphan delegation lock detected (%s) - cleaning", *lock_id);
              deleg.ReleaseCred(*lock_id); // not forcing credential removal - PeriodicCheckConsumers will do it with time control
            };
          };
        } else {
          logger.msg(Arc::ERROR,"Failed to obtain delegation locks for cleaning orphaned locks");
        };
      };

    };

    //Is this the right place to call ReportHeartBeatChange?
    HeartBeatMetrics* heartbeat_metrics = config_.GetHeartBeatMetrics();
    if(heartbeat_metrics) heartbeat_metrics->ReportHeartBeatChange(config_);

    SpaceMetrics* space_metrics = config_.GetSpaceMetrics();
    if(space_metrics) space_metrics->ReportSpaceChange(config_);

    jobs.WaitAttention();
    logger.msg(Arc::DEBUG,"Waking up");
  };
  // Waiting for children to finish
  logger.msg(Arc::INFO,"Stopping jobs processing thread");
  jobs.PrepareToDestroy();
  logger.msg(Arc::INFO,"Exiting jobs processing thread");
  jobs_ = NULL;
  return true;
}

void GridManager::RequestJobAttention(const std::string& job_id) {
  if(jobs_) { // TODO: a bit of race condition here against destructor
    jobs_->RequestAttention(job_id);
  };
}

GridManager::GridManager(GMConfig& config):tostop_(false), config_(config) {
  jobs_ = NULL;
  if(!Arc::CreateThreadFunction(&grid_manager,(void*)this,&active_)) { };
}

GridManager::~GridManager(void) {
  if(!jobs_) return; // Not initialized at all
  logger.msg(Arc::INFO, "Shutting down job processing");
  // Tell main thread to stop
  tostop_ = true;
  // Wait for main thread
  while(true) {
    if(jobs_) // Race condition again
      jobs_->RequestAttention(); // Kick jobs processor to release control
    if(active_.wait(1000)) break;
  }
}

} // namespace ARex

