#ifndef DTR_H_
#define DTR_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/data/DataHandle.h>
#include <arc/CheckSum.h>
#include <arc/data/URLMap.h>
#include <arc/DateTime.h>
#include <arc/Logger.h>
#include <arc/User.h>
#include <arc/UserConfig.h>
#include <arc/Thread.h>
#include "DTRStatus.h"

/// DataStaging contains all components for data transfer scheduling and execution.
namespace DataStaging {

  /// Components of the data staging framework
  enum StagingProcesses {GENERATOR, SCHEDULER, PRE_PROCESSOR, DELIVERY, POST_PROCESSOR};
  
  /// Internal state of staging processes
  enum ProcessState {INITIATED, RUNNING, TO_STOP, STOPPED};

  /// Represents limits and properties of a DTR transfer. These generally apply
  /// to all DTRs.
  class TransferParameters {
    public:
    /// Minimum average bandwidth in bytes/sec - if the average bandwidth used
    /// drops below this level the transfer should be killed
    unsigned long long int min_average_bandwidth;
    /// Maximum inactivity time in sec - if transfer stops for longer than this
    /// time it should be killed
    unsigned int max_inactivity_time;
    /// Minimum current bandwidth - if bandwidth averaged over averaging_time
    /// is less than minimum the transfer should be killed (allows transfers
    /// which slow down to be killed quicker)
    unsigned long long int min_current_bandwidth;
    /// The time over which to average the calculation of min_curr_bandwidth
    unsigned int averaging_time;
    /// Constructor. Initialises all values to zero
    TransferParameters() : min_average_bandwidth(0), max_inactivity_time(0),
                           min_current_bandwidth(0), averaging_time(0) {};
  };

  /// The configured cache directories
  class CacheParameters {
    public:
    /// List of (cache dir [link dir])
    std::vector<std::string> cache_dirs;
    /// List of (cache dir [link dir]) for remote caches
    std::vector<std::string> remote_cache_dirs;
    /// List of draining caches. Not necessary for data staging but here for completeness.
    std::vector<std::string> drain_cache_dirs;
    /// Constructor with empty lists initialised
    CacheParameters(void) {};
    /// Constructor with supplied cache lists
    CacheParameters(std::vector<std::string> caches,
                    std::vector<std::string> remote_caches,
                    std::vector<std::string> drain_caches);
  };

  /// Represents possible cache states of this DTR
  enum CacheState {
    CACHEABLE,             ///< Source should be cached
    NON_CACHEABLE,         ///< Source should not be cached
    CACHE_RENEW,           ///< Cache file should be deleted then re-downloaded
    CACHE_ALREADY_PRESENT, ///< Source is available in cache from before
    CACHE_DOWNLOADED,      ///< Source has just been downloaded and put in cache
    CACHE_LOCKED,          ///< Cache file is locked
    CACHE_SKIP,            ///< Source is cacheable but due to some problem should not be cached
    CACHE_NOT_USED         ///< Cache was started but was not used
  };
  	
  class DTR;

  /// The base class from which all callback-enabled classes should be derived.
  /**
   * This class is a container for a callback method which is called when a
   * DTR is to be passed to a component. Several components in data staging
   * (eg Scheduler, Generator) are subclasses of DTRCallback, which allows
   * them to receive DTRs through the callback system.
   */
  class DTRCallback {
    public:
      /** Empty virtual destructor */
      virtual ~DTRCallback() {};
      /**
       * Defines the callback method called when a DTR is pushed to
       * this object. Note that the DTR object is passed by reference
       * and so there is no guarantee that it will exist after this
       * callback method is called.
       */
      virtual void receiveDTR(DTR& dtr) = 0;
      // TODO
      //virtual void suspendDTR(DTR& dtr) = 0;
      //virtual void cancelDTR(DTR& dtr) = 0;
  };

  /// Data Transfer Request.
  /**
   * DTR stands for Data Transfer Request and a DTR describes a data transfer
   * between two endpoints, a source and a destination. There are several
   * parameters and options relating to the transfer contained in a DTR.
   * The normal workflow is for a Generator to create a DTR and send it to the
   * Scheduler for processing using dtr.push(SCHEDULER). If the Generator is a
   * subclass of DTRCallback, when the Scheduler has finished with the DTR
   * the receiveDTR() callback method is called.
   *
   * registerCallback(this,DataStaging::GENERATOR) can be used to
   * activate the callback. The following simple Generator code sample
   * illustrates how to use DTRs:
   *
   * @code
   * class MyGenerator : public DTRCallback {
   *  public:
   *   void receiveDTR(DTR& dtr);
   *   void run();
   *  private:
   *   Arc::SimpleCondition cond;
   * };
   *
   * void MyGenerator::receiveDTR(DTR& dtr) {
   *   // DTR received back, so notify waiting condition
   *   std::cout << "Received DTR " << dtr.get_id() << std::endl;
   *   cond.signal();
   * }
   *
   * void MyGenerator::run() {
   *   // start Scheduler thread
   *   Scheduler scheduler;
   *   scheduler.start();
   *
   *   // create a DTR
   *   DTR dtr(source, destination,...);
   *
   *   // register this callback
   *   dtr.registerCallback(this,DataStaging::GENERATOR);
   *   // this line must be here in order to pass the DTR to the Scheduler
   *   dtr.registerCallback(&scheduler,DataStaging::SCHEDULER);
   *
   *   // push the DTR to the Scheduler
   *   dtr.push(DataStaging::SCHEDULER);
   *
   *   // wait until callback is called
   *   cond.wait();
   *   // DTR is finished, so stop Scheduler
   *   scheduler.stop();
   * }
   * @endcode
   *
   * A lock protects member variables that are likely to be accessed and
   * modified by multiple threads.
   */
  class DTR {
  	
    private:
      /// Identifier
      std::string DTR_ID;
      
      /// UserConfig and URL objects. Needed as DataHandle keeps a reference to them.
      Arc::URL source_url;
      Arc::URL destination_url;
      Arc::UserConfig cfg;

      /// Source file
      Arc::DataHandle source_endpoint;
      /// Destination file
      Arc::DataHandle destination_endpoint;
      
      /// Endpoint of cached file.
      /* Kept as string so we don't need to duplicate DataHandle properties
       * of destination. Delivery should check if this is set and if so use
       * it as destination. */
      std::string cache_file;

      /// Cache configuration
      CacheParameters cache_parameters;

      /// Cache state for this DTR
      CacheState cache_state;

      /// Local user information
      Arc::User user;

      /// Job that requested the transfer. Could be used as a generic way of grouping DTRs.
      std::string parent_job_id;

      /// A flattened number set by the scheduler
      int priority;
      
      /// Transfer share this DTR belongs to
      std::string transfershare;

      /// This string can be used to form sub-sets of transfer shares.
      /** It is appended to transfershare. It can be used by the Generator
       * for example to split uploads and downloads into separate shares or
       * make shares for different endpoints. */
      std::string sub_share;

      /// Number of attempts left to complete this DTR
      unsigned int tries_left;

      /// A flag to say whether the DTR is replicating inside the same LFN of an index service
      bool replication;

      /// A flag to say whether to forcibly register the destination in an index service.
      /** Even if the source is not the same file, the destination will be
       * registered to an existing LFN. It should be set to true in
       * the case where an output file is uploaded to several locations but
       * with the same index service LFN */
      bool force_registration;

      /// The file that the current source is mapped to.
      /** Delivery should check if this is set and if so use this as source. */
      std::string mapped_source;

      /// Status of the DTR
      DTRStatus status;
      
      /// Error status of the DTR
      DTRErrorStatus error_status;

      /// Number of bytes transferred so far
      unsigned long long int bytes_transferred; // TODO and/or offset?

      /** Timing variables **/
      /// When should we finish the current action
      Arc::Time timeout;
      /// Creation time
      Arc::Time created;
      /// Modification time
      Arc::Time last_modified;
      /// Wait until this time before doing more processing
      Arc::Time next_process_time;
      
      /// True if some process requested cancellation
      bool cancel_request;
      
      /// Endpoint of remote delivery service this DTR is scheduled for
      Arc::URL delivery_endpoint;

      /// The process in charge of this DTR right now
      StagingProcesses current_owner;

      /// Logger object.
      /** Creation and deletion of this object should be managed
       * in the Generator and a pointer passed in the DTR constructor. */
      Arc::Logger * logger;

      /// Log Destinations.
      /** This list is kept here so that the Logger can be connected and
       * disconnected in threads which have their own root logger
       * to avoid duplicate messages */
      std::list<Arc::LogDestination*> log_destinations;

      /// List of callback methods called when DTR moves between processes
      std::map<StagingProcesses,std::list<DTRCallback*> > proc_callback;

      /// Lock to avoid collisions while changing DTR properties
      Arc::SimpleCondition lock;
      
      /** Possible fields  (types, names and so on are subject to change) **
      
      /// DTRs that are grouped must have the same number here
      int affiliation;
      
      /// History of recent statuses
      DTRStatus::DTRStatusType *history_of_statuses;
      
      **/
      
      /* Methods */
      /// Change modification time
      void mark_modification () { last_modified.SetTime(time(NULL)); };

      /// Assignment operator. Private and not implemented since DataHandle cannot be copied.
      DTR& operator=(const DTR& dtr);

    public:
      
      /// Public empty constructor
      DTR();
      
      /// Copy constructor. Must be defined because DataHandle copy constructor is private.
      DTR(const DTR& dtr);
      
      /// Normal constructor.
      /** Construct a new DTR.
       * @param source Endpoint from which to read data
       * @param destination Endpoint to which to write data
       * @param usercfg Provides some user configuration information
       * @param jobid ID of the job associated with this data transfer
       * @param uid UID to use when accessing local file system if source
       * or destination is a local file. If this is different to the current
       * uid then the current uid must have sufficient privileges to change uid.
       * @param log Pointer to log object. If NULL the root logger is used.
       */
      DTR(const std::string& source,
          const std::string& destination,
          const Arc::UserConfig& usercfg,
          const std::string& jobid,
          const uid_t& uid,
          Arc::Logger* log);
      
      /// Empty destructor
      ~DTR() {};

      
     /// Is DTR valid?
     operator bool() const {
       return (!DTR_ID.empty());
     }
     /// Is DTR not valid?
     bool operator!() const {
       return (DTR_ID.empty());
     }

     /// Register callback objects to be used during DTR processing.
     /** Objects deriving from DTRCallback can be registered with this method.
      * The callback method of these objects will then be called when the DTR
      * is passed to the specified owner. Protected by lock. */
     void registerCallback(DTRCallback* cb, StagingProcesses owner);

     /// Get the list of callbacks for this owner. Protected by lock.
     std::list<DTRCallback*> get_callbacks(const std::map<StagingProcesses, std::list<DTRCallback*> >& proc_callback,
                                           StagingProcesses owner);

     /// Reset information held on this DTR, such as resolved replicas, error state etc.
     /** Useful when a failed DTR is to be retried. */
     void reset();

     /// Set the ID of this DTR. Useful when passing DTR between processes
     void set_id(const std::string& id);
     /// Get the ID of this DTR
     std::string get_id() const { return DTR_ID; };
     /// Get an abbreviated version of the DTR ID - useful to reduce logging verbosity
     std::string get_short_id() const;
     
     /// Get source handle. Return by reference since DataHandle cannot be copied
     Arc::DataHandle& get_source() { return source_endpoint; };
     /// Get source handle. Return by reference since DataHandle cannot be copied
     const Arc::DataHandle& get_source() const { return source_endpoint; };
     /// Get destination handle. Return by reference since DataHandle cannot be copied
     Arc::DataHandle& get_destination() { return destination_endpoint; };
     /// Get destination handle. Return by reference since DataHandle cannot be copied
     const Arc::DataHandle& get_destination() const { return destination_endpoint; };

     /// Get the UserConfig object associated with this DTR
     const Arc::UserConfig& get_usercfg() const { return cfg; };

     /// Set the timeout for processing this DTR
     void set_timeout(time_t value) { timeout.SetTime(Arc::Time().GetTime() + value); };
     /// Get the timeout for processing this DTR
     Arc::Time get_timeout() const { return timeout; };
     
     /// Set the next processing time to current time + given time
     void set_process_time(const Arc::Period& process_time);
     /// Get the next processing time for the DTR
     Arc::Time get_process_time() const { return next_process_time; };

     /// Get the creation time
     Arc::Time get_creation_time() const { return created; };
     
     /// Get the parent job ID
     std::string get_parent_job_id() const { return parent_job_id; };
     
     /// Set the priority
     void set_priority(int pri);
     /// Get the priority
     int get_priority() const { return priority; };
     
     /// Set the transfer share. sub_share is automatically added to transfershare
     void set_transfer_share(std::string share_name);
     /// Get the transfer share. sub_share is automatically added to transfershare
     std::string get_transfer_share() const { return transfershare; };
     
     /// Set sub-share
     void set_sub_share(const std::string& share) { sub_share = share; };
     /// Get sub-share
     std::string get_sub_share() const { return sub_share; };

     /// Set the number of attempts remaining
     void set_tries_left(unsigned int tries);
     /// Get the number of attempts remaining
     unsigned int get_tries_left() const { return tries_left; };
     /// Decrease attempt number
     void decrease_tries_left();

     /// Set the status. Protected by lock.
     void set_status(DTRStatus stat);
     /// Get the status. Protected by lock.
     DTRStatus get_status();
     
     /// Set the error status.
     /** The DTRErrorStatus last error state field is set to the current status
      * of the DTR. Protected by lock. */
     void set_error_status(DTRErrorStatus::DTRErrorStatusType error_stat,
                           DTRErrorStatus::DTRErrorLocation error_loc,
                           const std::string& desc="");
     /// Set the error status back to NONE_ERROR and clear other fields
     void reset_error_status();
     /// Get the error status.
     DTRErrorStatus get_error_status();

     /// Set bytes transferred (should be set by whatever is controlling the transfer)
     void set_bytes_transferred(unsigned long long int bytes) { bytes_transferred = bytes; };
     /// Get current number of bytes transferred
     unsigned long long int get_bytes_transferred() const { return bytes_transferred; };

     /// Set the DTR to be cancelled
     void set_cancel_request();
     /// Returns true if cancellation has been requested
     bool cancel_requested() const { return cancel_request; };
     
     /// Set remote delivery endpoint. If empty then local delivery is used
     void set_remote_delivery_endpoint(const Arc::URL& endpoint) { delivery_endpoint = endpoint; };
     /// Returns remote delivery endpoint
     const Arc::URL& get_remote_delivery_endpoint() const { return delivery_endpoint; };

     /// Set cache filename
     void set_cache_file(const std::string& filename);
     /// Get cache filename
     std::string get_cache_file() const { return cache_file; };

     /// Set cache parameters
     void set_cache_parameters(const CacheParameters& param) { cache_parameters = param; };
     /// Get cache parameters
     const CacheParameters& get_cache_parameters() const { return cache_parameters; };

     /// Set the cache state
     void set_cache_state(CacheState state);
     /// Get the cache state
     CacheState get_cache_state() const { return cache_state; };

     /// Set the mapped file
     void set_mapped_source(const std::string& file = "") { mapped_source = file; };
     /// Get the mapped file
     std::string get_mapped_source() const { return mapped_source; };

     /// Find the DTR owner
     StagingProcesses get_owner() const { return current_owner; };
     
     /// Get the local user information
     Arc::User get_local_user() const { return user; };

     /// Set replication flag
     void set_replication(bool rep) { replication = rep; };
     /// Get replication flag
     bool is_replication() const { return replication; };
     /// Set force replication flag
     void set_force_registration(bool force) { force_registration = force; };
     /// Get force replication flag
     bool is_force_registration() const { return force_registration; };

     /// Get Logger object, so that processes can log to this DTR's log
     Arc::Logger * get_logger() const { return logger; };

     /// Connect log destinations to logger. Only needs to be done after disconnect()
     void connect_logger() { logger->addDestinations(log_destinations); };
     /// Disconnect log destinations from logger.
     void disconnect_logger() { logger->removeDestinations(); };

     /// Pass the DTR from one process to another. Protected by lock.
     void push(StagingProcesses new_owner);
     
     /// Suspend the DTR which is in doing transfer in the delivery process
     bool suspend();
     
     /// Did an error happen?
     bool error() const { return (error_status != DTRErrorStatus::NONE_ERROR); }
     
     /// Returns true if this DTR is about to go into the pre-processor
     bool is_destined_for_pre_processor() const;
     /// Returns true if this DTR is about to go into the post-processor
     bool is_destined_for_post_processor() const;
     /// Returns true if this DTR is about to go into delivery
     bool is_destined_for_delivery() const;
     
     /// Returns true if this DTR just came from the pre-processor
     bool came_from_pre_processor() const;
     /// Returns true if this DTR just came from the post-processor
     bool came_from_post_processor() const;
     /// Returns true if this DTR just came from delivery
     bool came_from_delivery() const;
     /// Returns true if this DTR just came from the generator
     bool came_from_generator() const;
     /// Returns true if this DTR is in a final state (finished, failed or cancelled)
     bool is_in_final_state() const;
  };
  
} // namespace DataStaging
#endif /*DTR_H_*/
