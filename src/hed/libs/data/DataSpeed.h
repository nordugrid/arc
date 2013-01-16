// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATASPEED_H__
#define __ARC_DATASPEED_H__

#include <string>

#include <arc/Logger.h>

#define DATASPEED_AVERAGING_PERIOD 60

namespace Arc {

  /// Keeps track of average and instantaneous transfer speed.
  /**
   * Also detects data transfer inactivity and other transfer timeouts.
   * \ingroup data
   * \headerfile DataSpeed.h arc/data/DataSpeed.h
   */
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
    static Logger logger;

  public:
    /// Callback for output of transfer status.
    /**
     * A function with this signature can be passed to set_progress_indicator()
     * to enable user-defined output of transfer progress.
     * \param o FILE object connected to stderr
     * \param s prefix set in verbose(const std::string&)
     * \param t time in seconds since the start of the transfer
     * \param all number of bytes transferred so far
     * \param max total amount of bytes to be transferred (set in set_max_data())
     * \param instant instantaneous transfer rate in bytes per second
     * \param average average transfer rate in bytes per second
     */
    typedef void (*show_progress_t)(FILE *o, const char *s, unsigned int t,
                                    unsigned long long int all,
                                    unsigned long long int max,
                                    double instant, double average);
  private:
    show_progress_t show_progress;
    void print_statistics(FILE *o, time_t t);
  public:
    /// Constructor
    /**
     * \param base time period used to average values (default 1 minute).
     */
    DataSpeed(time_t base = DATASPEED_AVERAGING_PERIOD);
    /// Constructor
    /**
     * \param min_speed minimal allowed speed (bytes per second). If speed
     * drops and holds below threshold for min_speed_time seconds error is
     * triggered.
     * \param min_speed_time time over which to calculate min_speed.
     * \param min_average_speed minimal average speed (bytes per second)
     * to trigger error. Averaged over whole current transfer time.
     * \param max_inactivity_time if no data is passing for specified
     * amount of time, error is triggered.
     * \param base time period used to average values (default 1 minute).
     */
    DataSpeed(unsigned long long int min_speed, time_t min_speed_time,
              unsigned long long int min_average_speed,
              time_t max_inactivity_time,
              time_t base = DATASPEED_AVERAGING_PERIOD);
    /// Destructor
    ~DataSpeed();
    /// Set true to activate printing transfer information during transfer.
    void verbose(bool val);
    /// Activate printing transfer information using 'prefix' at the beginning of every string.
    void verbose(const std::string& prefix);
    /// Check if speed information is going to be printed.
    bool verbose();
    /// Set minimal allowed speed in bytes per second.
    /**
     * \param min_speed minimal allowed speed (bytes per second). If speed
     * drops and holds below threshold for min_speed_time seconds error
     * is triggered.
     * \param min_speed_time time over which to calculate min_speed.
     */
    void set_min_speed(unsigned long long int min_speed,
                       time_t min_speed_time);
    /// Set minimal average speed in bytes per second.
    /**
     * \param min_average_speed minimal average speed (bytes per second)
     * to trigger error. Averaged over whole current transfer time.
     */
    void set_min_average_speed(unsigned long long int min_average_speed);
    /// Set inactivity timeout.
    /**
     * \param max_inactivity_time - if no data is passing for specified
     * amount of time, error is triggered.
     */
    void set_max_inactivity_time(time_t max_inactivity_time);
    /// Get inactivity timeout.
    time_t get_max_inactivity_time() { return max_inactivity_time; };
    /// Set averaging time period (default 1 minute).
    void set_base(time_t base_ = DATASPEED_AVERAGING_PERIOD);
    /// Set amount of data (in bytes) to be transferred. Used in verbose messages.
    void set_max_data(unsigned long long int max = 0);
    /// Specify an external function to print verbose messages.
    /**
     * If not specified an internal function is used.
     * \param func pointer to function which prints information.
     */
    void set_progress_indicator(show_progress_t func = NULL);
    /// Reset all counters and triggers.
    void reset();
    /// Inform object that an amount of data has been transferred.
    /**
     * All errors are triggered by this method. To make them work the
     * application must call this method periodically even with zero value.
     * \param n amount of data transferred in bytes.
     * \return false if transfer rate is below limits
     */
    bool transfer(unsigned long long int n = 0);
    /// Turn and off off speed control.
    void hold(bool disable);
    /// Check if minimal speed error was triggered.
    bool min_speed_failure() {
      return min_speed_failed;
    }
    /// Check if minimal average speed error was triggered.
    bool min_average_speed_failure() {
      return min_average_speed_failed;
    }
    /// Check if maximal inactivity time error was triggered.
    bool max_inactivity_time_failure() {
      return max_inactivity_time_failed;
    }
    /// Returns number of bytes transferred so far (this object knows about).
    unsigned long long int transferred_size() {
      return Nall;
    }
  };

} // namespace Arc

#endif // __ARC_DATASPEED_H__
