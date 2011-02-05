#ifndef DTR_H_
#define DTR_H_

#include <arc/data/DataHandle.h>
#include <arc/data/CheckSum.h>
#include <arc/data/URLMap.h>
#include <arc/DateTime.h>
#include <arc/Logger.h>
#include <arc/User.h>
#include <arc/UserConfig.h>
#include <arc/Thread.h>
#include "DTRStatus.h"


namespace DataStaging {

  enum StagingProcesses {GENERATOR, SCHEDULER, PRE_PROCESSOR, DELIVERY, POST_PROCESSOR};
  
  /* Internal state of staging processes */
  enum ProcessState {INITIATED, RUNNING, TO_STOP, STOPPED};

  /** Struct which represents limits and properties of a DTR transfer */
  class TransferParameters {
    public:
    /// Minimum average bandwidth in bytes/sec - if the average bandwidth used drops below this level the transfer should be killed
    unsigned long long int min_average_bandwidth;
    /// Minimum inactivity time in sec - if transfer stops for longer than this time it should be killed
    unsigned int min_activity_time;
    /// Minimum current bandwidth - if bandwidth averaged over averaging_time is less than minimum the transfer should be killed (allows transfers which slow down to be killed quicker)
    unsigned long long int min_current_bandwidth;
    /// The time over which to average the calculation of min_curr_bandwidth
    unsigned int averaging_time;
    /// Number of bytes transferred so far
    unsigned long long int bytes_transferred; // TODO and/or offset?
    /// Time at which transfer started
    Arc::Time start_time;
    /// Pointer to checksum object
    Arc::CheckSum* checksum;
    /// Flag to say whether transfer is complete (all bytes copied successfully)
    bool transfer_finished;
  };

  class CacheParameters {
    public:
    /// List of (cache dir [link dir])
    std::vector<std::string> cache_dirs;
    /// List of (cache dir [link dir]) for remote caches
    std::vector<std::string> remote_cache_dirs;
    // Is it needed?
    std::vector<std::string> drain_cache_dirs;
    CacheParameters(void) {};
    CacheParameters(std::vector<std::string> caches, std::vector<std::string> remote_caches, std::vector<std::string> drain_caches);
  };

  std::ostream& operator<<(std::ostream& stream, const CacheParameters& obj);
  std::istream& operator>>(std::istream& stream, CacheParameters& obj);

  /** Represents possible cache states of this DTR */
  enum CacheState {
    CACHEABLE,         /// Source should be cached
    NON_CACHEABLE,     /// Source should not be cached
    CACHE_RENEW,       /// Cache file should be deleted then re-downloaded
    CACHE_ALREADY_PRESENT,     /// Source is available in cache from before
    CACHE_DOWNLOADED,  /// Source has just been downloaded and put in cache
    CACHE_LOCKED,      /// Cache file is locked
    CACHE_SKIP,        /// Source is cacheable but due to some problem should not be cached
    CACHE_NOT_USED     /// Cache was started but was not used
  };
  	
  class DTR;

  /**
   * This class is container for callback method which is
   * to be called for passing control over specified DTR.
   */
  class DTRCallback {
    public:
      virtual ~DTRCallback() {};
      virtual void receive_dtr(DTR dtr) = 0;
  };

  /**
   * DTR stands for Data Transfer Request and a DTR describes a data transfer
   * between two endpoints, a source and a destination. There are several
   * parameters and options relating to the transfer contained in a DTR.
   * The normal workflow is to create a DTR and send it to the Scheduler for
   * processing using Scheduler::receive_dtr(). An optional callback can be
   * defined using DTR::registerCallback(). When the Scheduler has finished
   * with the DTR this callback is called. The following code sample
   * illustrates how to use DTRs:
   *
   * Arc::SimpleCondition cond;
   *
   * void receive_dtr(DTR dtr) {
   *   cond.signal();
   * }
   *
   * int main() {
   *   DTR::registerCallback(receive_dtr);
   *   DTR dtr(source, destination,...);
   *   Scheduler::getInstance()->receive_dtr(dtr);
   *   // wait until callback is called
   *   cond.wait();
   *   // DTR is finished, so stop Scheduler
   *   Scheduler::getInstance()->stop();
   *   return 0;
   * }
   */
  class DTR {
  	
    private:
      /* Identifier */
      std::string DTR_ID;
      
      /* UserConfig and URL objects. Needed as are passed by reference to DataHandle */
      Arc::URL source_url;
      Arc::URL destination_url;
      Arc::UserConfig cfg;

      /* Source and destination files */
      Arc::DataHandle source_endpoint;
      Arc::DataHandle destination_endpoint;
      
      /* Endpoint of cached file. Keep as string so we don't need to duplicate
       * DataHandle properties of destination. Delivery should check if this
       * is set and if so use it as destination. */
      std::string cache_file;

      /* Top level cache directory. This will be replaced by values taken
       * from configuration */
      //std::string cache_dir;
      CacheParameters cache_parameters;

      /* Transfer parameters */
      TransferParameters transfer_parameters;
      
      /* Cache state for this DTR */
      CacheState cache_state;

      /* Local user information */
      Arc::User user;

      /* Job that requested the transfer */
      std::string parent_job_id;

      /* A flattened number set by the scheduler */
      int priority;
      
      /* A transfer share this DTR belongs to */
      std::string transfershare;

      /* This string can be used to form sub-sets of transfer shares. It is
       * appended to transfershare. It can be used by the Generator for
       * example to split uploads and downloads into separate shares or
       * make shares for different endpoints. */
      std::string sub_share;

      /* A flag to say whether the DTR is replicating inside the same LFN
       * of an index service */
      bool replication;

      /* A flag to say whether to register the destination in an index service
       * even if the source is not the same file. It should be set to true in
       * the case where an output file is uploaded to several locations but
       * with the same index service LFN */
      bool force_registration;

      /* The file that the current source is mapped to. Delivery should check
       * if this is set and if so use this as source. */
      std::string mapped_source;

      /* Status of the DTR */
      DTRStatus status;
      
      /* Error status of the DTR */
      DTRErrorStatus error_status;

      /** Timing variables **/
      /* When should we finish the current action */
      Arc::Time timeout;
      /* Creation time */
      Arc::Time created;
      /* Modification time */
      Arc::Time last_modified;
      /* Wait until this time before doing more processing */
      Arc::Time next_process_time;
      
      /* If some process requested cancellation */
      bool cancel_request;
      
      /* Which process is in charge of this DTR right now */
      StagingProcesses current_owner;

      /* Logger object. Creation and deletion of this object should be managed
       * in the Generator and a pointer passed in the DTR constructor. */
      Arc::Logger * logger;

      /* Object with callback method called when DTR is finished */
      std::map<StagingProcesses,DTRCallback*> proc_callback;

      /* Lock to avoid collisions while changing owner/status */
      Arc::SimpleCondition lock;
      
      /** Possible fields  (types, names and so on are subject to change) **
      
      // DTRs that are grouped must have the same number here 
      int affiliation;
      
      // History of recent statuses
      DTRStatus::DTRStatusType *history_of_statuses;
      
      **/
      
      /* Methods */
      // Change modification time
      void mark_modification () { last_modified.SetTime(time(NULL)); };

    public:
      
      /// Public empty constructor
      DTR();
      
      /// Copy constructor. Must be defined because DataHandle copy constructor is private.
      DTR(const DTR& dtr);
      
      /// Normal constructor
      /** Construct a new DTR.
       * @param source Endpoint from which to read data
       * @param destination Endpoint to which to write data
       * @param usercfg Provides some user configuration information
       * @param job_id ID of the job associated with this data transfer
       * @param uid UID of the local user the grid job is mapped to
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

      /// Assignment operator. Must be defined because DataHandle copy constructor is private.
      DTR& operator=(const DTR& dtr);
      
     // Is DTR valid?
     operator bool() const {
       return (!DTR_ID.empty());
     }
     bool operator!() const {
       return (DTR_ID.empty());
     }

     // Register callback to be used when DTR processing needs to
     // be passed to another module.
     void registerCallback(DTRCallback* cb, StagingProcesses owner);

     // Get the ID of this DTR
     std::string get_id() const { return DTR_ID; };
     // Get an abbreviated version of the DTR ID - useful to reduce logging verbosity
     std::string get_short_id() const;
     
     // Get source and destination handles. Return by reference so they can be manipulated
     Arc::DataHandle& get_source() { return source_endpoint; };
     const Arc::DataHandle& get_source() const { return source_endpoint; };
     Arc::DataHandle& get_destination() { return destination_endpoint; };
     const Arc::DataHandle& get_destination() const { return destination_endpoint; };

     // Get the UserConfig object associated with this DTR
     const Arc::UserConfig& get_usercfg() const { return cfg; };

     // Manipulate the timeout
     void set_timeout(time_t value) { timeout.SetTime(Arc::Time().GetTime() + value); };
     Arc::Time get_timeout() const { return timeout; };
     
     // Set the next processing time to current time + given time
     void set_process_time(const Arc::Period& process_time);
     Arc::Time get_process_time() const { return next_process_time; };

     // Get the creation time
     Arc::Time get_creation_time() const { return created; };
     // Manipulate the transfer parameters
     void set_parameters(const struct TransferParameters& params);
     const TransferParameters get_parameters() const { return transfer_parameters; };
     
     // Get the parent job ID
     std::string get_parent_job_id() const { return parent_job_id; };
     
     // Manipulate the priority
     void set_priority(int pri);
     int get_priority() const { return priority; };
     
     // Manipulate the transfer share. sub_share is automatically added to transfershare
     void set_transfer_share(std::string share_name);
     std::string get_transfer_share() const { return transfershare; };
     
     // Manipulate sub-share
     void set_sub_share(const std::string& share) { sub_share = share; };
     std::string get_sub_share() const { return sub_share; };

     // Manipulate the status
     void set_status(DTRStatus stat);
     DTRStatus get_status() const { return status; };
     
     // Manipulate the error status. The DTRErrorStatus last error state field
     // is set to the current status of the DTR.
     void set_error_status(DTRErrorStatus::DTRErrorStatusType error_stat,
                           DTRErrorStatus::DTRErrorLocation error_loc,
                           const std::string& desc="");
     // Set the error status back to NO_ERROR and clear other fields
     void reset_error_status();
     DTRErrorStatus get_error_status() const { return error_status; };

     // Manipulate the cancel request
     void set_cancel_request();
     bool cancel_requested() const { return cancel_request; };
     
     // Manipulate cache filename
     void set_cache_file(const std::string& filename);
     std::string get_cache_file() const { return cache_file; };

     // Manipulate cache directory. Will become obsolete when conf file is used.
     //void set_cache_dir(const std::string& dir) { cache_dir = dir; };
     //std::string get_cache_dir() const { return cache_dir; };
     void set_cache_parameters(const CacheParameters& param) { cache_parameters = param; };
     const CacheParameters& get_cache_parameters() const { return cache_parameters; };

     // Manipulate the cache state
     void set_cache_state(CacheState state);
     CacheState get_cache_state() const { return cache_state; };

     // Manipulate the mapped file
     void set_mapped_source(const std::string& file = "") { mapped_source = file; };
     std::string get_mapped_source() const { return mapped_source; };

     // Manipulate the owner
     StagingProcesses get_owner() const { return current_owner; };
     void set_owner(StagingProcesses new_owner);
     
     // Get the local user information
     Arc::User get_local_user() const { return user; };

     // Manipulate replication flags
     void set_replication(bool rep) { replication = rep; };
     bool is_replication() const { return replication; };
     void set_force_registration(bool force) { force_registration = force; };
     bool is_force_registration() const { return force_registration; };

     // Get Logger object, so that processes can log to this DTR's log
     Arc::Logger * get_logger() const { return logger; };

     // Pass the DTR from one process to another
     void push(StagingProcesses new_owner);
     
     // Suspend the DTR which is in doing transfer in the delivery process
     bool suspend();
     
     // Did an error happen?
     bool error() const { return (error_status != DTRErrorStatus::NO_ERROR); }
     
     // Functions to figure out of the status if 
     // this DTR is about to go into a corresponding process
     bool is_destined_for_pre_processor() const;
     bool is_destined_for_post_processor() const;
     bool is_destined_for_delivery() const;
     
     // Functions to figure out which process owned 
     // this DTR before it changed its owner
     bool came_from_pre_processor() const;
     bool came_from_post_processor() const;
     bool came_from_delivery() const;
     bool came_from_generator() const;
     bool is_in_final_state() const;
  };
  
} // namespace DataStaging
#endif /*DTR_H_*/
