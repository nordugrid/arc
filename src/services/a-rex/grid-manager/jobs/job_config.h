#ifndef GRID_MANAGER_JOBSLIST_H
#define GRID_MANAGER_JOBSLIST_H

#include <arc/URL.h>

#include "../jobs/job.h"

class JobsList;

/**
 * ZeroUInt is a wrapper around unsigned int. It provides a consistent
 * default value, as int type variables have no predefined value
 * assigned upon creation. It also protects from potential counter
 * underflow, to stop counter jumping to MAX_INT.
*/
class ZeroUInt {
 private:
  unsigned int value_;
 public:
  ZeroUInt(void):value_(0) { };
  ZeroUInt(unsigned int v):value_(v) { };
  ZeroUInt(const ZeroUInt& v):value_(v.value_) { };
  ZeroUInt& operator=(unsigned int v) { value_=v; return *this; };
  ZeroUInt& operator=(const ZeroUInt& v) { value_=v.value_; return *this; };
  ZeroUInt& operator++(void) { ++value_; return *this; };
  ZeroUInt operator++(int) { ZeroUInt temp(value_); ++value_; return temp; };
  ZeroUInt& operator--(void) { if(value_) --value_; return *this; };
  ZeroUInt operator--(int) { ZeroUInt temp(value_); if(value_) --value_; return temp; };
  operator unsigned int(void) const { return value_; };
};

/* default job ttl after finished - 1 week */
#define DEFAULT_KEEP_FINISHED (7*24*60*60)
/* default job ttr after deleted - 1 month */
#define DEFAULT_KEEP_DELETED (30*24*60*60)
/* default maximum number of jobs in download/upload */
#define DEFAULT_MAX_LOAD (10)
/* default maximal allowed amount of reruns */
#define DEFAULT_JOB_RERUNS (5)
/* not used */
#define DEFAULT_DISKSPACE (200*1024L*1024L)
/* default maximum down/upload retries */
#define DEFAULT_MAX_RETRIES (10)

/**
 * Class to represent information read from configuration.
 */
class JobsListConfig {
 friend class JobsList;
 private:
  /* number of jobs for every state */
  int jobs_num[JOB_STATE_NUM];
  /* map of number of active jobs for each DN */
  std::map<std::string, ZeroUInt> jobs_dn;
  int jobs_pending;
  /* maximal allowed values */
  int max_jobs_running;
  int max_jobs_total;
  int max_jobs_processing;
  int max_jobs_processing_emergency;
  int max_jobs;
  int max_jobs_per_dn;
  unsigned int max_processing_share;
  std::string share_type;
  unsigned long long int min_speed;
  time_t min_speed_time;
  unsigned long long int min_average_speed;
  time_t max_inactivity_time;
  int max_downloads;
  int max_retries;
  bool use_secure_transfer;
  bool use_passive_transfer;
  bool use_local_transfer;
  bool use_new_data_staging;
  unsigned int wakeup_period;
  std::string preferred_pattern;
  std::vector<Arc::URL> delivery_services;
  /* the list of shares with defined limits */
  std::map<std::string, int> limited_share;
 public:
  JobsListConfig(void);
  void SetMaxJobs(int max = -1,int max_running = -1, int max_per_dn = -1, int max_total = -1) {
    max_jobs=max;
    max_jobs_running=max_running;
    max_jobs_per_dn=max_per_dn;
    max_jobs_total=max_total;
  }
  void SetMaxJobsLoad(int max_processing = -1,int max_processing_emergency = 1,int max_down = -1) {
    max_jobs_processing=max_processing;
    max_jobs_processing_emergency=max_processing_emergency;
    max_downloads=max_down;
  }
  void GetMaxJobs(int &max,int &max_running,int &max_per_dn,int &max_total) const {
    max=max_jobs;
    max_running=max_jobs_running;
    max_per_dn=max_jobs_per_dn;
    max_total=max_jobs_total;
  }
  void GetMaxJobsLoad(int &max_processing,int &max_processing_emergency,int &max_down) const {
    max_processing=max_jobs_processing;
    max_processing_emergency=max_jobs_processing_emergency;
    max_down=max_downloads;
  }
  void SetSpeedControl(unsigned long long int min=0,time_t min_time=300,unsigned long long int min_average=0,time_t max_time=300) {
    min_speed = min;
    min_speed_time = min_time;
    min_average_speed = min_average;
    max_inactivity_time = max_time;
  }
  void GetSpeedControl(unsigned long long int& min, time_t& min_time, unsigned long long int& min_average, time_t& max_time) const {
    min = min_speed;
    min_time = min_speed_time;
    min_average = min_average_speed;
    max_time = max_inactivity_time;
  }
  void SetSecureTransfer(bool val) {
    use_secure_transfer=val;
  }
  bool GetSecureTransfer() const {
    return use_secure_transfer;
  }
  void SetPassiveTransfer(bool val) {
    use_passive_transfer=val;
  }
  bool GetPassiveTransfer() const {
    return use_passive_transfer;
  }
  void SetLocalTransfer(bool val) {
    use_local_transfer=val;
  }
  bool GetLocalTransfer() const {
    return use_local_transfer;
  }
  void SetNewDataStaging(bool val) {
    use_new_data_staging = val;
  }
  bool GetNewDataStaging() const {
    return use_new_data_staging;
  }
  void SetWakeupPeriod(unsigned int t) {
    wakeup_period=t;
  }
  unsigned int WakeupPeriod(void) const {
    return wakeup_period;
  }
  void SetMaxRetries(int r) {
    max_retries = r;
  }
  int MaxRetries() const {
    return max_retries;
  }
  void SetPreferredPattern(const std::string& pattern) {
    preferred_pattern = pattern;
  }
  std::string GetPreferredPattern() const {
    return preferred_pattern;
  }
  void SetTransferShare(unsigned int max_share, std::string type){
    max_processing_share = max_share;
    share_type = type;
  };
  std::string GetShareType() const {
    return share_type;
  }
  bool AddDeliveryService(const std::string& url) {
    Arc::URL u(url);
    if (!u) return false;
    delivery_services.push_back(u);
    return true;
  }
  std::vector<Arc::URL> GetDeliveryServices() const {
    return delivery_services;
  }
  bool AddLimitedShare(std::string share_name, unsigned int share_limit) {
    if(max_processing_share == 0)
      return false;
    if(share_limit == 0)
      share_limit = max_processing_share;
    limited_share[share_name] = share_limit;
    return true;
  }
  const std::map<std::string, int>& GetLimitedShares(void) const {
    return limited_share;
  };

};

#endif


