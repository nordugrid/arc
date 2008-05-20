#ifndef __ARC_DATAMOVER_H__
#define __ARC_DATAMOVER_H__

#include <string>
#include <list>

#include <arc/data/DataPoint.h>
#include <arc/data/DataCache.h>
#include <arc/data/DataSpeed.h>
#include <arc/data/DataStatus.h>

namespace Arc {

  class Logger;
  class URLMap;

  /// A purpose of this class is to provide an interface that moves data
  /// between two locations specified by URLs. It's main action is
  /// represented by methods DataMover::Transfer.
  /// Instance represents only attributes used during transfer.
  class DataMover {
  private:
    bool be_verbose;
    bool force_secure;
    bool force_passive;
    bool force_registration;
    bool do_checks;
    std::string verbose_prefix;
    bool do_retries;
    unsigned long long int default_min_speed;
    time_t default_min_speed_time;
    unsigned long long int default_min_average_speed;
    time_t default_max_inactivity_time;
    DataSpeed::show_progress_t show_progress;
    static Logger logger;
  public:
    typedef void (*callback)(DataMover *, DataStatus, const std::string&,
			     void *);
    /// Constructor
    DataMover();
    /// Destructor
    ~DataMover();
    /// Initiates transfer from 'source' to 'destination'.
    /// \param source source URL.
    /// \param destination destination URL.
    /// \param cache controls caching of downloaded files (if destination
    /// url is "file://"). If caching is not needed default constructor
    /// DataCache() can be used.
    /// \param map URL mapping/convertion table (for 'source' URL).
    /// \param cb if not NULL, transfer is done in separate thread and 'cb'
    /// is called after transfer completes/fails.
    /// \param arg passed to 'cb'.
    /// \param prefix if 'verbose' is activated this information will be
    /// printed before each line representing current transfer status.
    DataStatus Transfer(DataPoint& source, DataPoint& destination,
			DataCache& cache, const URLMap& map,
			std::string& failure_description,
			callback cb = NULL, void *arg = NULL,
			const char *prefix = NULL);
    /// Initiates transfer from 'source' to 'destination'.
    /// \param min_speed minimal allowed current speed.
    /// \param min_speed_time time for which speed should be less than
    /// 'min_speed' before transfer fails.
    /// \param min_average_speed minimal allowed average speed.
    /// \param max_inactivity_time time for which should be no activity
    /// before transfer fails.
    DataStatus Transfer(DataPoint& source, DataPoint& destination,
			DataCache& cache, const URLMap& map,
			unsigned long long int min_speed,
			time_t min_speed_time,
			unsigned long long int min_average_speed,
			time_t max_inactivity_time,
			std::string& failure_description,
			callback cb = NULL, void *arg = NULL,
			const char *prefix = NULL);
    DataStatus Delete(DataPoint& url, bool errcont = false);
    /// Check if printing information about transfer status is activated.
    bool verbose();
    /// Activate printing information about transfer status.
    void verbose(bool);
    /// Activate printing information about transfer status.
    /// \param prefix use this string if 'prefix' in DataMover::Transfer
    /// is NULL.
    void verbose(const std::string& prefix);
    /// Check if transfer will be retried in case of failure.
    bool retry();
    /// Set if transfer will be retried in case of failure.
    void retry(bool);
    /// Set if high level of security (encryption) will be used duirng
    /// transfer if available.
    void secure(bool);
    /// Set if passive transfer should be used for FTP-like transfers.
    void passive(bool);
    /// Set if file should be transfered and registered even if such LFN
    /// is already registered and source is not one of registered locations.
    void force_to_meta(bool);
    /// Check if check for existance of remote file is done before
    /// initiating 'reading' and 'writing' operations.
    bool checks();
    /// Set if to make check for existance of remote file (and probably
    /// other checks too) before initiating 'reading' and 'writing' operations.
    /// \param v true if allowed (default is true).
    void checks(bool v);
    /// Set minimal allowed transfer speed (default is 0) to 'min_speed'.
    /// If speed drops below for time longer than 'min_speed_time' error
    /// is raised. For more information see description of DataSpeed class.
    void set_default_min_speed(unsigned long long int min_speed,
			       time_t min_speed_time) {
      default_min_speed = min_speed;
      default_min_speed_time = min_speed_time;
    }
    /// Set minimal allowed average transfer speed (default is 0 averaged
    /// over whole time of transfer. For more information see description
    /// of DataSpeed class.
    void set_default_min_average_speed(unsigned long long int
				       min_average_speed) {
      default_min_average_speed = min_average_speed;
    }
    /// Set maximal allowed time for waiting for any data. For more
    /// information see description of DataSpeed class.
    void set_default_max_inactivity_time(time_t max_inactivity_time) {
      default_max_inactivity_time = max_inactivity_time;
    }
    void set_progress_indicator(DataSpeed::show_progress_t func = NULL) {
      show_progress = func;
    }
  };

} // namespace Arc

#endif // __ARC_DATAMOVER_H__
