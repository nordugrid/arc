// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_DATAMOVER_H__
#define __ARC_DATAMOVER_H__

#include <list>
#include <string>

#include <arc/data/FileCache.h>
#include <arc/data/DataPoint.h>
#include <arc/data/DataSpeed.h>
#include <arc/data/DataStatus.h>

namespace Arc {

  class Logger;
  class URLMap;

  /// DataMover provides an interface to transfer data between two DataPoints.
  /**
   * Its main action is represented by Transfer methods.
   * \ingroup data
   * \headerfile DataMover.h arc/data/DataMover.h
   */
  class DataMover {
  private:
    bool be_verbose;
    bool force_secure;
    bool force_passive;
    bool force_registration;
    bool do_checks;
    std::string verbose_prefix;
    bool do_retries;
    bool do_retries_retryable;
    unsigned long long int default_min_speed;
    time_t default_min_speed_time;
    unsigned long long int default_min_average_speed;
    time_t default_max_inactivity_time;
    DataSpeed::show_progress_t show_progress;
    std::string preferred_pattern;
    bool cancelled;
    /// For safe destruction of object, Transfer() holds this lock and
    /// destructor waits until the lock can be obtained
    Glib::Mutex lock_;
    static Logger logger;
  public:
    /// Callback function which can be passed to Transfer().
    /**
     * \param mover this DataMover instance
     * @param status result of the transfer
     * @param arg arguments passed in 'arg' parameter of Transfer()
     */
    typedef void (*callback)(DataMover* mover, DataStatus status, void* arg);
    /// Constructor. Sets all transfer parameters to default values.
    DataMover();
    /// Destructor cancels transfer if active and waits for cancellation to finish.
    ~DataMover();
    /// Initiates transfer from 'source' to 'destination'.
    /**
     * An optional callback can be provided, in which case this method starts
     * a separate thread for the transfer and returns immediately. The callback
     * is called after the transfer finishes.
     * \param source source DataPoint to read from.
     * \param destination destination DataPoint to write to.
     * \param cache controls caching of downloaded files (if destination
     * url is "file://"). If caching is not needed default constructor
     * FileCache() can be used.
     * \param map URL mapping/conversion table (for 'source' URL). If URL
     * mapping is not needed the default constructor URLMap() can be used.
     * \param cb if not NULL, transfer is done in separate thread and 'cb'
     * is called after transfer completes/fails.
     * \param arg passed to 'cb'.
     * \param prefix if 'verbose' is activated this information will be
     * printed before each line representing current transfer status.
     * \return DataStatus object with transfer result
     */
    DataStatus Transfer(DataPoint& source, DataPoint& destination,
                        FileCache& cache, const URLMap& map,
                        callback cb = NULL, void *arg = NULL,
                        const char *prefix = NULL);
    /// Initiates transfer from 'source' to 'destination'.
    /**
     * An optional callback can be provided, in which case this method starts
     * a separate thread for the transfer and returns immediately. The callback
     * is called after the transfer finishes.
     * \param source source DataPoint to read from.
     * \param destination destination DataPoint to write to.
     * \param cache controls caching of downloaded files (if destination
     * url is "file://"). If caching is not needed default constructor
     * FileCache() can be used.
     * \param map URL mapping/conversion table (for 'source' URL). If URL
     * mapping is not needed the default constructor URLMap() can be used.
     * \param min_speed minimal allowed current speed.
     * \param min_speed_time time for which speed should be less than
     * 'min_speed' before transfer fails.
     * \param min_average_speed minimal allowed average speed.
     * \param max_inactivity_time time for which should be no activity
     * before transfer fails.
     * \param cb if not NULL, transfer is done in separate thread and 'cb'
     * is called after transfer completes/fails.
     * \param arg passed to 'cb'.
     * \param prefix if 'verbose' is activated this information will be
     * printed before each line representing current transfer status.
     * \return DataStatus object with transfer result
     */
    DataStatus Transfer(DataPoint& source, DataPoint& destination,
                        FileCache& cache, const URLMap& map,
                        unsigned long long int min_speed,
                        time_t min_speed_time,
                        unsigned long long int min_average_speed,
                        time_t max_inactivity_time,
                        callback cb = NULL, void *arg = NULL,
                        const char *prefix = NULL);
    /// Delete the file at url.
    /**
     * This method differs from DataPoint::Remove() in that for index services,
     * it deletes all replicas in addition to removing the index entry.
     * @param url file to delete
     * @param errcont if true then replica information will be deleted from an
     * index service even if deleting the physical replica fails
     * @return DataStatus object with result of deletion
     */
    DataStatus Delete(DataPoint& url, bool errcont = false);
    /// Cancel transfer, cleaning up any data written or registered.
    void Cancel();
    /// Returns whether printing information about transfer status is activated.
    bool verbose();
    /// Set output of transfer status information during transfer.
    void verbose(bool);
    /// Set output of transfer status information during transfer.
    /**
     * \param prefix use this string if 'prefix' in DataMover::Transfer is NULL.
     */
    void verbose(const std::string& prefix);
    /// Returns whether transfer will be retried in case of any failure.
    bool retry();
    /// Set if transfer will be retried in case of any failure.
    void retry(bool);
    /// Returns whether transfer will be retried in case of retryable failure.
    bool retry_retryable();
    /// Set if transfer will be retried in case of retryable failure.
    void retry_retryable(bool);
    /// Set if high level of security (encryption) will be used during transfer if available.
    void secure(bool);
    /// Set if passive transfer should be used for FTP-like transfers.
    void passive(bool);
    /// Set if file should be transferred and registered even if such LFN is already registered and source is not one of registered locations.
    void force_to_meta(bool);
    /// Returns true if extra checks are made before transfer starts.
    bool checks();
    /// Set if extra checks are made before transfer starts.
    /**
     * If turned on, extra checks are done before commencing the transfer, such
     * as checking the existence of the source file and verifying consistency
     * of metadata between index service and physical replica.
     */
    void checks(bool v);
    /// Set minimal allowed transfer speed (default is 0) to 'min_speed'.
    /**
     * If speed drops below for time longer than 'min_speed_time', error
     * is raised. For more information see description of DataSpeed class.
     * \param min_speed minimum transfer rate in bytes/second
     * \param min_speed_time time in seconds over which min_speed is measured
     */
    void set_default_min_speed(unsigned long long int min_speed,
                               time_t min_speed_time) {
      default_min_speed = min_speed;
      default_min_speed_time = min_speed_time;
    }
    /// Set minimal allowed average transfer speed.
    /**
     * Default is 0 averaged over whole time of transfer. For more information
     * see description of DataSpeed class.
     * \param min_average_speed minimum average transfer rate over the whole
     * transfer in bytes/second
     */
    void set_default_min_average_speed(unsigned long long int min_average_speed) {
      default_min_average_speed = min_average_speed;
    }
    /// Set maximal allowed time for no data transfer.
    /**
     * For more information see description of DataSpeed class.
     * \param max_inactivity_time maximum time in seconds which is allowed
     * without any data transfer
     */
    void set_default_max_inactivity_time(time_t max_inactivity_time) {
      default_max_inactivity_time = max_inactivity_time;
    }
    /// Set function which is called every second during the transfer
    void set_progress_indicator(DataSpeed::show_progress_t func = NULL) {
      show_progress = func;
    }
    /// Set a preferred pattern for ordering of replicas.
    /**
     * This pattern will be used in the case of an index service URL with
     * multiple physical replicas and allows sorting of those replicas in order
     * of preference. It consists of one or more patterns separated by a pipe
     * character (|) listed in order of preference. If the dollar character ($)
     * is used at the end of a pattern, the pattern will be matched to the end
     * of the hostname of the replica. Example: "srm://myhost.org|.uk$|.ch$"
     * \param pattern pattern on which to order replicas
     */
    void set_preferred_pattern(const std::string& pattern) {
      preferred_pattern = pattern;
    }
  };

} // namespace Arc

#endif // __ARC_DATAMOVER_H__
