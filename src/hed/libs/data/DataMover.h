#ifndef __ARC_DATAMOVER_H__
#define __ARC_DATAMOVER_H__

#include <string>
#include <list>

#include "DataPoint.h"
#include "DataCache.h"
#include "DataSpeed.h"

namespace Arc {

class Logger;
class URLMap;

/// A purpose of this class is to provide interface moves data 
/// between 2 locations specified by URLs. It's main action is
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
  /// Error code/failure reason
  typedef enum {
    /// Operation completed successfully
    success = 0,
    /// Source is bad URL or can't be used due to some reason
    read_acquire_error = 1,
    /// Destination is bad URL or can't be used due to some reason
    write_acquire_error = 2,
    /// Resolving of meta-URL for source failed
    read_resolve_error = 3,
    /// Resolving of meta-URL for destination failed
    write_resolve_error = 4,
    /// First stage of registration of meta-URL failed
    preregister_error = 5,
    /// Can't read from source
    read_start_error = 6,
    /// Can't write to destination
    write_start_error = 7,
    /// Failed while reading from source
    read_error = 8,
    /// Failed while writing to destination
    write_error = 9,
    /// Failed while transfering data (mostly timeout)
    transfer_error = 10,
    /// Failed while finishing reading from source
    read_stop_error = 11,
    /// Failed while finishing writing to destination
    write_stop_error = 12,
    /// Last stage of registration of meta-URL failed
    postregister_error = 13,
    /// Error in caching procedure
    cache_error = 14,
    /// Some system function returned unexpected error  
    system_error = 15,
    /// Error due to provided credentials are expired
    credentials_expired_error = 16,
    delete_error = 17,
    location_unregister_error = 18,
    unregister_error = 19,
    /// Unknown/undefined error
    undefined_error = -1
  } result;
  typedef void(*callback)(DataMover*,DataMover::result,const char*,void*);
  /// Constructor
  DataMover(void);
  /// Destructor
  ~DataMover(void);
  /// Initiates transfer from 'source' to 'destination'.
  /// \param source source URL.
  /// \param destination destination URL.
  /// \param cache controls caching of downloaded files (if destination url is "file://"). If caching is not needed default constructor DataCache() can be used.
  /// \param map URL mapping/convertion table (for 'source' URL). 
  /// \param cb if not NULL, transfer is done in separate thread and 'cb' is called after transfer completes/fails.
  /// \param arg passed to 'cb'.
  /// \param prefix if 'verbose' is activated this information will be printed before each line representing current transfer status.
  result Transfer(DataPoint &source,DataPoint &destination,
                     DataCache &cache,const URLMap &map,
                     std::string &failure_description,
                     callback cb = NULL,void* arg = NULL,
                     const char* prefix = NULL);
  /// Initiates transfer from 'source' to 'destination'.
  /// \param min_speed minimal allowed current speed.
  /// \param min_speed_time time for which speed should be less than 'min_speed' before transfer fails.
  /// \param min_average_speed minimal allowed average speed.
  /// \param max_inactivity_time time for which should be no activity before transfer fails.
  result Transfer(DataPoint &source,DataPoint &destination,
                     DataCache &cache,const URLMap &map,
                     unsigned long long int min_speed,time_t min_speed_time,
                     unsigned long long int min_average_speed,
                     time_t max_inactivity_time,
                     std::string &failure_description,
                     callback cb = NULL,void* arg = NULL,
                     const char* prefix = NULL);
  result Delete(DataPoint &url,bool errcont = false);
  /// Check if printing information about transfer status is activated.
  bool verbose(void);
  /// Activate printing information about transfer status.
  void verbose(bool);
  /// Activate printing information about transfer status.
  /// \param prefix use this string if 'prefix' in DataMover::Transfer is NULL.
  void verbose(const std::string& prefix);
  /// Check if transfer will be retried in case of failure.
  bool retry(void);
  /// Set if transfer will be retried in case of failure.
  void retry(bool);
  /// Set if high level of security (encryption) will be used duirng transfer if available.
  void secure(bool);
  /// Set if passive transfer should be used for FTP-like transfers.
  void passive(bool);
  /// Set if file should be transfered and registered even if such LFN
  /// is already registered and source is not one of registered locations.
  void force_to_meta(bool);
  /// Check if check for existance of remote file is done before initiating 'reading' and 'writing' operations. 
  bool checks(void);
  /// Set if to make check for existance of remote file (and probably other checks too) before initiating 'reading' and 'writing' operations.
  /// \param v true if allowed (default is true).
  void checks(bool v);
  /// Set minimal allowed transfer speed (default is 0) to 'min_speed'. 
  /// If speed drops below for time longer than 'min_speed_time' error 
  /// is raised. For more information see description of DataSpeed class.
  void set_default_min_speed(unsigned long long int min_speed,time_t min_speed_time) {
    default_min_speed=min_speed;
    default_min_speed_time=min_speed_time;
  };
  /// Set minimal allowed average transfer speed (default is 0 averaged 
  /// over whole time of transfer. For more information see description 
  /// of DataSpeed class.
  void set_default_min_average_speed(unsigned long long int min_average_speed) {
    default_min_average_speed=min_average_speed;
  };
  /// Set maximal allowed time for waiting for any data. For more 
  /// information see description of DataSpeed class.
  void set_default_max_inactivity_time(time_t max_inactivity_time) {
    default_max_inactivity_time=max_inactivity_time;
  };
  void set_progress_indicator(DataSpeed::show_progress_t func = NULL) {
    show_progress=func;
  };
  static const char* get_result_string(result r);
};

} // namespace Arc

#endif // __ARC_DATAMOVER_H__
