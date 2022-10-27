// Summary page of data staging for doxygen
namespace DataStaging {
/**
 * \defgroup datastaging ARC data staging (libarcdatastaging)
 *
 * ARC data staging components form a complete data transfer management system.
 * Whereas \ref data is a library for data access, enabling several types of
 * operation on data files on the Grid using a variety of access protocols,
 * \ref datastaging is a framework for managed data transfer to and from the
 * Grid. The data staging system is designed to run as a persistent process, to
 * execute data transfers on demand. Data transfers are defined and fed into
 * the system, and then notification is given when they complete. No knowledge
 * is required of the internal workings of the Grid, a user only needs to
 * specify URLs representing the source and destination of the transfer.
 *
 * The system is highly configurable and features an intelligent priority,
 * fair-share and error handling mechanism, as well as the ability to spread
 * data transfer across multiple hosts using ARC's DataDelivery service. It is
 * used by ARC's Computing Element (A-REX) for pre- and post- job data transfer
 * of input and output files. Note that this system is primarily for data
 * transfer to and from local files and that third-party transfer is not
 * supported. It is designed for the case of pulling or pushing data between
 * the Grid and a local file system, rather than a service for transfer between
 * two Grid storage elements. It is possible to transfer data between two
 * remote endpoints, but all data flows through the client.
 *
 * Simple examples of how to use libarcdatastaging are shown for several
 * languages in the \ref dtrgenerator "DTR examples page". In all the examples
 * a Generator class receives as input a source and destination, and creates
 * a DTR which describes the data transfer. It is then passed to the Scheduler
 * and the Generator defines a receiveDTR() method for the Scheduler to calls
 * to notify that the transfer has finished. The examples all allow using the
 * Generator as a basic copy tool from the command line to copy a single file.
 *
 * For more information see http://wiki.nordugrid.org/index.php/Data_Staging
 */
} // namespace DataStaging

#ifndef DTR_H_
#define DTR_H_

#include <arc/data/DataHandle.h>
#include <arc/CheckSum.h>
#include <arc/data/URLMap.h>
#include <arc/DateTime.h>
#include <arc/JobPerfLog.h>
#include <arc/Logger.h>
#include <arc/User.h>
#include <arc/UserConfig.h>
#include <arc/Thread.h>
#include "DTRStatus.h"

/// DataStaging contains all components for data transfer scheduling and execution.
namespace DataStaging {

  class DTR;

  /// Provides automatic memory management of DTRs and thread-safe destruction.
  /** \ingroup datastaging */
  typedef Arc::ThreadedPointer<DTR> DTR_ptr;

  /// The DTR's Logger object can be used outside the DTR object with DTRLogger.
  /** \ingroup datastaging */
  typedef Arc::ThreadedPointer<Arc::Logger> DTRLogger;
  typedef Arc::ThreadedPointer<Arc::LogDestination> DTRLogDestination;

  /// Components of the data staging framework
  /** \ingroup datastaging */
  enum StagingProcesses {
    GENERATOR,     ///< Creator of new DTRs and receiver of completed DTRs
    SCHEDULER,     ///< Controls queues and moves DTRs bewteen other components when necessary
    PRE_PROCESSOR, ///< Performs all pre-transfer operations
    DELIVERY,      ///< Performs physical transfer
    POST_PROCESSOR ///< Performs all post-transfer operations
  };
  
  /// Internal state of StagingProcesses
  /** \ingroup datastaging */
  enum ProcessState {
    INITIATED, ///< Process is ready to start
    RUNNING,   ///< Process is running
    TO_STOP,   ///< Process has been instructed to stop
    STOPPED    ///< Proecess has stopped
  };

  /// Represents limits and properties of a DTR transfer. These generally apply to all DTRs.
  /**
   * \ingroup datastaging
   * \headerfile DTR.h arc/data-staging/DTR.h
   */
  class TransferParameters {
    public:
    /// Minimum average bandwidth in bytes/sec.
    /**
     * If the average bandwidth used over the whole transfer drops below this
     * level the transfer will be killed.
     */
    unsigned long long int min_average_bandwidth;
    /// Maximum inactivity time in sec.
    /**
     * If transfer stops for longer than this time it will be killed.
     */
    unsigned int max_inactivity_time;
    /// Minimum current bandwidth in bytes/sec.
    /**
     * If bandwidth averaged over the previous averaging_time seconds is less
     * than min_current_bandwidth the transfer will be killed (allows transfers
     * which slow down to be killed quicker).
     */
    unsigned long long int min_current_bandwidth;
    /// The time in seconds over which to average the calculation of min_current_bandwidth.
    unsigned int averaging_time;
    /// Constructor. Initialises all values to zero.
    TransferParameters() : min_average_bandwidth(0), max_inactivity_time(0),
                           min_current_bandwidth(0), averaging_time(0) {};
  };

  /// The configured cache directories
  /**
   * \ingroup datastaging
   * \headerfile DTR.h arc/data-staging/DTR.h
   */
  class DTRCacheParameters {
    public:
    /// List of (cache dir [link dir])
    std::vector<std::string> cache_dirs;
    /// List of draining caches
    std::vector<std::string> drain_cache_dirs;
    /// List of read-only caches
    std::vector<std::string> readonly_cache_dirs;
    /// Constructor with empty lists initialised
    DTRCacheParameters(void) {};
    /// Constructor with supplied cache lists
    DTRCacheParameters(std::vector<std::string> caches,
                       std::vector<std::string> drain_caches,
                       std::vector<std::string> readonly_caches);
  };

  /// Class for storing credential information
  /**
   * To avoid handling credentials directly this class is used to hold
   * information in simple string/time attributes. It should be filled before
   * the DTR is started.
   * \ingroup datastaging
   * \headerfile DTR.h arc/data-staging/DTR.h
   */
  class DTRCredentialInfo {
   public:
    /// Default constructor
    DTRCredentialInfo() {};
    /// Constructor with supplied credential info
    DTRCredentialInfo(const std::string& DN,
                      const Arc::Time& expirytime,
                      const std::list<std::string> vomsfqans);
    /// Get the DN
    std::string getDN() const { return DN; };
    /// Get the expiry time
    Arc::Time getExpiryTime() const { return expirytime; };
    /// Get the VOMS VO
    std::string extractVOMSVO() const;
    /// Get the VOMS Group (first in the supplied list of fqans)
    std::string extractVOMSGroup() const;
    /// Get the VOMS Role (first in the supplied list of fqans)
    std::string extractVOMSRole() const;
   private:
    std::string DN;
    Arc::Time expirytime;
    std::list<std::string> vomsfqans;

  };

  /// Represents possible cache states of this DTR
  /** \ingroup datastaging */
  enum CacheState {
    CACHEABLE,             ///< Source should be cached
    NON_CACHEABLE,         ///< Source should not be cached
    CACHE_ALREADY_PRESENT, ///< Source is available in cache from before
    CACHE_DOWNLOADED,      ///< Source has just been downloaded and put in cache
    CACHE_LOCKED,          ///< Cache file is locked
    CACHE_SKIP,            ///< Source is cacheable but due to some problem should not be cached
    CACHE_NOT_USED         ///< Cache was started but was not used
  };
  	
  /// The base class from which all callback-enabled classes should be derived.
  /**
   * This class is a container for a callback method which is called when a
   * DTR is to be passed to a component. Several components in data staging
   * (eg Scheduler, Generator) are subclasses of DTRCallback, which allows
   * them to receive DTRs through the callback system.
   * \ingroup datastaging
   * \headerfile DTR.h arc/data-staging/DTR.h
   */
  class DTRCallback {
    public:
      /// Empty virtual destructor
      virtual ~DTRCallback() {};
      /// Defines the callback method called when a DTR is pushed to this object.
      /**
       * The automatic memory management of DTR_ptr ensures that the DTR object
       * is only deleted when the last copy is deleted.
       */
      virtual void receiveDTR(DTR_ptr dtr) = 0;
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
   * Scheduler for processing using DTR::push(SCHEDULER). If the Generator is a
   * subclass of DTRCallback, when the Scheduler has finished with the DTR
   * the DTRCallback::receiveDTR() callback method is called.
   *
   * DTRs should always be used through the Arc::ThreadedPointer DTR_ptr. This
   * ensures proper memory management when passing DTRs among various threads.
   * To enforce this policy the copy constructor and assignment operator are
   * private.
   *
   * A lock protects member variables that are likely to be accessed and
   * modified by multiple threads.
   * \ingroup datastaging
   * \headerfile DTR.h arc/data-staging/DTR.h
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

    /// Source file as a string
    std::string source_url_str;
    /// Destination file as a string
    std::string destination_url_str;

    /// Endpoint of cached file.
    /* Kept as string so we don't need to duplicate DataHandle properties
     * of destination. Delivery should check if this is set and if so use
     * it as destination. */
    std::string cache_file;

    /// Cache configuration
    DTRCacheParameters cache_parameters;

    /// Cache state for this DTR
    CacheState cache_state;

    /// Local user information
    Arc::User user;

    /// Credential information
    DTRCredentialInfo credentials;

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

    /// Initial number of attempts
    unsigned int initial_tries;

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
    /// Time taken in ns to complete transfer (0 if incomplete)
    unsigned long long int transfer_time;

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

    /// Bulk start flag
    bool bulk_start;
    /// Bulk end flag
    bool bulk_end;
    /// Whether bulk operations are supported for the source
    bool source_supports_bulk;

    /// Flag to say whether success of the DTR is mandatory
    bool mandatory;

    /// Endpoint of delivery service this DTR is scheduled for.
    /** By default it is LOCAL_DELIVERY so local Delivery is used. */
    Arc::URL delivery_endpoint;

    /// List of problematic endpoints - those which the DTR definitely cannot use
    std::vector<Arc::URL> problematic_delivery_endpoints;

    /// Whether to use host instead of user credentials for contacting remote delivery services.
    bool use_host_cert_for_remote_delivery;

    /// The process in charge of this DTR right now
    StagingProcesses current_owner;

    /// Logger object.
    /** Creation and deletion of this object should be managed
     * in the Generator and a pointer passed in the DTR constructor. */
    DTRLogger logger;

    /// Log Destinations.
    /** This list is kept here so that the Logger can be connected and
     * disconnected in threads which have their own root logger
     * to avoid duplicate messages */
    std::list<DTRLogDestination> log_destinations;

    /// Flag to say whether to delete LogDestinations.
    /** Set to true when a DTR thread is stuck or lost so it doesn't crash when
     * waking up after DTR has finished */
    //bool delete_log_destinations;

    /// Performance metric logger
    Arc::JobPerfLog perf_log;

    /// Performance record used for recording transfer time
    Arc::JobPerfRecord perf_record;

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

    /// Get the list of callbacks for this owner. Protected by lock.
    std::list<DTRCallback*> get_callbacks(const std::map<StagingProcesses, std::list<DTRCallback*> >& proc_callback,
                                          StagingProcesses owner);

    /// Private and not implemented because DTR_ptr should always be used.
    DTR& operator=(const DTR& dtr);
    DTR(const DTR& dtr);
    DTR();


  public:

    /// URL that is used to denote local Delivery should be used
    static const Arc::URL LOCAL_DELIVERY;

    /// Log level for all DTR activity
    static Arc::LogLevel LOG_LEVEL;

    /// Normal constructor.
    /** Construct a new DTR.
     * @param source Endpoint from which to read data
     * @param destination Endpoint to which to write data
     * @param usercfg Provides some user configuration information
     * @param jobid ID of the job associated with this data transfer
     * @param uid UID to use when accessing local file system if source
     * or destination is a local file. If this is different to the current
     * uid then the current uid must have sufficient privileges to change uid.
     * @param logs List of ThreadedPointers to Logger Destinations to be 
     * receive DTR processing messages.
     * @param logname Subdomain name to use for internal DTR logger.
     */
    DTR(const std::string& source,
        const std::string& destination,
        const Arc::UserConfig& usercfg,
        const std::string& jobid,
        const uid_t& uid,
        std::list<DTRLogDestination> const& logs,
        const std::string& logname = std::string("DTR"));

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
    /**
     * Objects deriving from DTRCallback can be registered with this method.
     * The callback method of these objects will then be called when the DTR
     * is passed to the specified owner. Protected by lock.
     */
    void registerCallback(DTRCallback* cb, StagingProcesses owner);

    /// Reset information held on this DTR, such as resolved replicas, error state etc.
    /**
     * Useful when a failed DTR is to be retried.
     */
    void reset();

    /// Set the ID of this DTR. Useful when passing DTR between processes.
    void set_id(const std::string& id);
    /// Get the ID of this DTR
    std::string get_id() const { return DTR_ID; };
    /// Get an abbreviated version of the DTR ID - useful to reduce logging verbosity
    std::string get_short_id() const;
     
    /// Get source handle. Return by reference since DataHandle cannot be copied
    Arc::DataHandle& get_source() { return source_endpoint; };
    /// Get destination handle. Return by reference since DataHandle cannot be copied
    Arc::DataHandle& get_destination() { return destination_endpoint; };

    /// Get source as a string
    std::string get_source_str() const { return source_url_str; };
    /// Get destination as a string
    std::string get_destination_str() const { return destination_url_str; };

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
     
    /// Get the modification time
    Arc::Time get_modification_time() const { return last_modified; };

    /// Get the parent job ID
    std::string get_parent_job_id() const { return parent_job_id; };
     
    /// Set the priority
    void set_priority(int pri);
    /// Get the priority
    int get_priority() const { return priority; };
     
    /// Set credential info
    void set_credential_info(const DTRCredentialInfo& cred) { credentials = cred; };
    /// Get credential info
    const DTRCredentialInfo& get_credential_info() const { return credentials; };

    /// Set the transfer share. sub_share is automatically added to transfershare.
    void set_transfer_share(const std::string& share_name);
    /// Get the transfer share. sub_share is automatically added to transfershare.
    std::string get_transfer_share() const { return transfershare; };
     
    /// Set sub-share
    void set_sub_share(const std::string& share) { sub_share = share; };
    /// Get sub-share
    std::string get_sub_share() const { return sub_share; };

    /// Set the number of attempts remaining
    void set_tries_left(unsigned int tries);
    /// Get the number of attempts remaining
    unsigned int get_tries_left() const { return tries_left; };
    /// Get the initial number of attempts (set by set_tries_left())
    unsigned int get_initial_tries() const { return initial_tries; }
    /// Decrease attempt number
    void decrease_tries_left();

    /// Set the status. Protected by lock.
    void set_status(DTRStatus stat);
    /// Get the status. Protected by lock.
    DTRStatus get_status();
     
    /// Set the error status.
    /**
     * The DTRErrorStatus last error state field is set to the current status
     * of the DTR. Protected by lock.
     */
    void set_error_status(DTRErrorStatus::DTRErrorStatusType error_stat,
                          DTRErrorStatus::DTRErrorLocation error_loc,
                          const std::string& desc="");
    /// Set the error status back to NONE_ERROR and clear other fields
    void reset_error_status();
    /// Get the error status.
    DTRErrorStatus get_error_status();

    /// Set bytes transferred (should be set by whatever is controlling the transfer)
    void set_bytes_transferred(unsigned long long int bytes);
    /// Get current number of bytes transferred
    unsigned long long int get_bytes_transferred() const { return bytes_transferred; };

    /// Set transfer time (should be set by whatever is controlling the transfer)
    void set_transfer_time(unsigned long long int t);
    /// Get transfer time
    unsigned long long int get_transfer_time() const { return transfer_time; };

    /// Set the DTR to be cancelled
    void set_cancel_request();
    /// Returns true if cancellation has been requested
    bool cancel_requested() const { return cancel_request; };
     
    /// Set delivery endpoint
    void set_delivery_endpoint(const Arc::URL& endpoint) { delivery_endpoint = endpoint; };
    /// Returns delivery endpoint
    const Arc::URL& get_delivery_endpoint() const { return delivery_endpoint; };

    /// Add problematic endpoint.
    /**
     * Should only be those endpoints where there is a problem with the service
     * itself and not the transfer.
     */
    void add_problematic_delivery_service(const Arc::URL& endpoint) { problematic_delivery_endpoints.push_back(endpoint); };
    /// Get all problematic endpoints
    const std::vector<Arc::URL>& get_problematic_delivery_services() const { return problematic_delivery_endpoints; };

    /// Set the flag for using host certificate for contacting remote delivery services
    void host_cert_for_remote_delivery(bool host) { use_host_cert_for_remote_delivery = host; };
    /// Get the flag for using host certificate for contacting remote delivery services
    bool host_cert_for_remote_delivery() const { return use_host_cert_for_remote_delivery; };

    /// Set cache filename
    void set_cache_file(const std::string& filename);
    /// Get cache filename
    std::string get_cache_file() const { return cache_file; };

    /// Set cache parameters
    void set_cache_parameters(const DTRCacheParameters& param) { cache_parameters = param; };
    /// Get cache parameters
    const DTRCacheParameters& get_cache_parameters() const { return cache_parameters; };

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

    /// Set bulk start flag
    void set_bulk_start(bool value) { bulk_start = value; };
    /// Get bulk start flag
    bool get_bulk_start() const { return bulk_start; };
    /// Set bulk end flag
    void set_bulk_end(bool value) { bulk_end = value; };
    /// Get bulk start flag
    bool get_bulk_end() const { return bulk_end; };
    /// Whether bulk operation is possible according to current state and src/dest
    bool bulk_possible();

    /// Whether DTR success is mandatory
    bool is_mandatory() const { return mandatory; };

    /// Get Logger object, so that processes can log to this DTR's log
    const DTRLogger& get_logger() const { return logger; };

    /// Get log destination sassigned to this instance.
    std::list<Arc::LogDestination*> get_log_destinations() const;

    /// Pass the DTR from one process to another. Protected by lock.
    static void push(DTR_ptr dtr, StagingProcesses new_owner);
     
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

    /// Get the performance log
    Arc::JobPerfLog& get_job_perf_log() { return perf_log; };
    /// Get the performance log record
    Arc::JobPerfRecord& get_job_perf_record() { return perf_record; };

  };
  
  /// Helper method to create smart pointer, only for swig bindings
  DTR_ptr createDTRPtr(const std::string& source,
                       const std::string& destination,
                       const Arc::UserConfig& usercfg,
                       const std::string& jobid,
                       const uid_t& uid,
                       std::list<DTRLogDestination> const& logs,
                       const std::string& logname = std::string("DTR"));

  /// Helper method to create smart pointer, only for swig bindings
  DTRLogger createDTRLogger(Arc::Logger& parent,
                            const std::string& subdomain);

} // namespace DataStaging
#endif /*DTR_H_*/
