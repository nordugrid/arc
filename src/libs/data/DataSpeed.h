#ifndef __ARC_DATASPEED_H__
#define __ARC_DATASPEED_H__

#include <string>

#define DATASPEED_AVERAGING_PERIOD 60

namespace Arc {

/// Keeps track of average and instantaneous speed. Also detects data
/// transfer inactivity and other transfer timeouts.
class DataSpeed {
 private:
  time_t first_time;
  time_t last_time;
  time_t last_activity_time;
  unsigned long long int N;
  unsigned long long int Nall;
  unsigned long long int Nmax;
  time_t first_speed_failure;
  time_t last_printed;

  time_t T;
  time_t min_speed_time;
  time_t max_inactivity_time;
  unsigned long long int min_speed;
  unsigned long long int min_average_speed;
  bool be_verbose;
  std::string verbose_prefix;

  bool min_speed_failed;
  bool min_average_speed_failed;
  bool max_inactivity_time_failed;

  bool disabled;
  
 public:
  typedef void (*show_progress_t)(FILE* o,const char* s,unsigned int t,
       unsigned long long int all,unsigned long long int max,
       double instant,double average);
 private:
  show_progress_t show_progress;
  void print_statistics(FILE *o,time_t t);
 public:
  /// Constructor
  /// \param base time period used to average values (default 1 minute).
  DataSpeed(time_t base = DATASPEED_AVERAGING_PERIOD);
  /// Constructor
  /// \param base time period used to average values (default 1 minute).
  /// \param min_speed minimal allowed speed (Butes per second). If speed drops and holds below threshold for min_speed_time_ seconds error is triggered.
  /// \param min_speed_time
  /// \param min_average_speed_ minimal average speed (Bytes per second) to trigger error. Averaged over whole current transfer time.
  /// \param max_inactivity_time - if no data is passing for specified amount of time (seconds), error is triggered.
  DataSpeed(unsigned long long int min_speed,time_t min_speed_time,
            unsigned long long int min_average_speed,
            time_t max_inactivity_time,
            time_t base = DATASPEED_AVERAGING_PERIOD);
  /// Destructor
  ~DataSpeed(void);
  /// Activate printing information about current time speeds, amount 
  /// of transfered data.
  void verbose(bool val);
  /// Print information about current speed and amout of data.
  /// \param 'prefix'  add this string at the beginning of every string. 
  void verbose(const std::string &prefix);
  /// Check if speed information is going to be printed.
  bool verbose(void);
  /// Set minimal allowed speed.
  /// \param min_speed minimal allowed speed (Butes per second). If speed drops and holds below threshold for min_speed_time_ seconds error is triggered.
  /// \param min_speed_time
  void set_min_speed(unsigned long long int min_speed,time_t min_speed_time);
  /// Set minmal avaerage speed.
  /// \param min_average_speed_ minimal average speed (Bytes per second) to trigger error. Averaged over whole current transfer time.
  void set_min_average_speed(unsigned long long int min_average_speed);
  /// Set inactivity tiemout.
  /// \param max_inactivity_time - if no data is passing for specified amount of time (seconds), error is triggered.
  void set_max_inactivity_time(time_t max_inactivity_time);
  /// Set averaging time period.
  /// \param base time period used to average values (default 1 minute).
  void set_base(time_t base_ = DATASPEED_AVERAGING_PERIOD);
  /// Set amount of data to be transfered. Used in verbose messages.
  /// \param max amount of data in bytes.
  void set_max_data(unsigned long long int max = 0);
  /// Specify which external function will print verbose messages.
  /// If not specified internal one is used.
  /// \param pointer to function which prints information.
  void set_progress_indicator(show_progress_t func = NULL);
  /// Reset all counters and triggers.
  void reset(void);
  /// Inform object, about amount of data has been transfered.
  /// All errors are triggered by this method. To make them work
  /// application must call this method periodically even with zero value.
  /// \param n amount of data transfered (bytes).
  bool transfer(unsigned long long int n = 0);
  /// Turn off speed control. 
  /// \param disable true to turn off.
  void hold(bool disable);
  /// Check if minimal speed error was triggered.
  bool min_speed_failure() { return min_speed_failed; };
  /// Check if minimal average speed error was triggered.
  bool min_average_speed_failure() { return min_average_speed_failed; };
  /// Check if maximal inactivity time error was triggered.
  bool max_inactivity_time_failure() { return max_inactivity_time_failed; };
  /// Returns amount of data this object knows about.
  unsigned long long int transfered_size(void) { return Nall; };
};

} // namespace Arc

#endif // __ARC_DATASPEED_H__
