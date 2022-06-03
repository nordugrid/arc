#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <arc/JobPerfLog.h>
#include <arc/Thread.h>
#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/data/URLMap.h>

#include "DTR.h"
#include "DTRList.h"
#include "Processor.h"
#include "DataDelivery.h"
#include "TransferShares.h"

namespace DataStaging {

/// The Scheduler is the control centre of the data staging framework.
/**
 * The Scheduler manages a global list of DTRs and schedules when they should
 * go into the next state or be sent to other processes. The DTR priority is
 * used to decide each DTR's position in a queue.
 * \ingroup datastaging
 * \headerfile Scheduler.h arc/data-staging/Scheduler.h
 */
class Scheduler: public DTRCallback {	

  private:
  
    /// All the DTRs the scheduler is aware of.
    /** The DTR comes to this list once received from the generator
     * and leaves the list only when pushed back to the generator. */
    DTRList DtrList;
    
    /// A list of jobs that have been requested to be cancelled.
    /** External threads add items to this list, and the Scheduler
     * processes it during the main loop. */
    std::list<std::string> cancelled_jobs;

    /// A list of DTRs to process
    std::list<DTR_ptr> events;

    /// Map of transfer shares to staged DTRs. Filled each event processing loop
    std::map<std::string, std::list<DTR_ptr> > staged_queue;

    /// A lock for the cancelled jobs list
    Arc::SimpleCondition cancelled_jobs_lock;

    /// Configuration of transfer shares
    TransferSharesConf transferSharesConf;

    /// URLMap containing information on any local mappings defined in the configuration
    Arc::URLMap url_map;

    /// Preferred pattern to match replicas defined in configuration
    std::string preferred_pattern;
      
    /// Lock to protect multi-threaded access to start() and stop()
    Arc::SimpleCondition state_lock;

    /// Lock for events list
    Arc::SimpleCondition event_lock;

    /// Condition to signal end of running
    Arc::SimpleCondition run_signal;

    /// Condition to signal end of dump thread
    Arc::SimpleCondition dump_signal;

    /// Limit on number of DTRs in pre-processor
    unsigned int PreProcessorSlots;
    /// Limit on number of DTRs in delivery
    unsigned int DeliverySlots;
    /// Limit on number of DTRs in post-processor
    unsigned int PostProcessorSlots;
    /// Limit on number of emergency DTRs in each state
    unsigned int EmergencySlots;
    /// Limit on number of staged-prepared files, per share
    unsigned int StagedPreparedSlots;

    /// Where to dump DTR state. Currently only a path to a file is supported.
    std::string dumplocation;

    /// Performance metrics logger
    Arc::JobPerfLog job_perf_log;

    /// Endpoints of delivery services from configuration
    std::vector<Arc::URL> configured_delivery_services;

    /// Map of delivery services and directories they can access, filled after
    /// querying all services when the first DTR is processed
    std::map<Arc::URL, std::vector<std::string> > usable_delivery_services;

    /// Timestamp of last check of delivery services
    Arc::Time delivery_last_checked;

    /// File size limit (in bytes) under which local transfer is used
    unsigned long long int remote_size_limit;

    /// Counter of transfers per delivery service
    std::map<std::string, int> delivery_hosts;

    /// Logger object
    static Arc::Logger logger;

    /// Root logger destinations, to use when logging non-DTR specific messages
    std::list<Arc::LogDestination*> root_destinations;

    /// Flag describing scheduler state. Used to decide whether to keep running main loop.
    ProcessState scheduler_state;

    /// Processor object
    Processor processor;

    /// Delivery object
    DataDelivery delivery;

    /// Static instance of Scheduler
    static Scheduler* scheduler_instance;

    /// Lock for multiple threads getting static Scheduler instance
    static Glib::Mutex instance_lock;

    /// Copy constructor is private because Scheduler should not be copied
    Scheduler(const Scheduler&); // should not happen
    /// Assignment operator is private because Scheduler should not be copied
    Scheduler& operator=(const Scheduler&); // should not happen

    /* Functions to process every state of the DTR during normal workflow */

    /// Process a DTR in the NEW state
    void ProcessDTRNEW(DTR_ptr request);
    /// Process a DTR in the CACHE_WAIT state
    void ProcessDTRCACHE_WAIT(DTR_ptr request);
    /// Process a DTR in the CACHE_CHECKED state
    void ProcessDTRCACHE_CHECKED(DTR_ptr request);
    /// Process a DTR in the RESOLVED state
    void ProcessDTRRESOLVED(DTR_ptr request);
    /// Process a DTR in the REPLICA_QUERIED state
    void ProcessDTRREPLICA_QUERIED(DTR_ptr request);
    /// Process a DTR in the PRE_CLEANED state
    void ProcessDTRPRE_CLEANED(DTR_ptr request);
    /// Process a DTR in the STAGING_PREPARING_WAIT state
    void ProcessDTRSTAGING_PREPARING_WAIT(DTR_ptr request);
    /// Process a DTR in the STAGED_PREPARED state
    void ProcessDTRSTAGED_PREPARED(DTR_ptr request);
    /// Process a DTR in the TRANSFERRED state
    void ProcessDTRTRANSFERRED(DTR_ptr request);
    /// Process a DTR in the REQUEST_RELEASED state
    void ProcessDTRREQUEST_RELEASED(DTR_ptr request);
    /// Process a DTR in the REPLICA_FINALISED state
    void ProcessDTRREPLICA_FINALISED(DTR_ptr request);
    /// Process a DTR in the REPLICA_REGISTERED state
    void ProcessDTRREPLICA_REGISTERED(DTR_ptr request);
    /// Process a DTR in the CACHE_PROCESSED state
    void ProcessDTRCACHE_PROCESSED(DTR_ptr request);
    /// Process a DTR in a final state
    /* This is a special function to deal with states after which
     * the DTR is returned to the generator, i.e. DONE, ERROR, CANCELLED */
    void ProcessDTRFINAL_STATE(DTR_ptr request);
    
    /// Log a message to the root logger. This sends the message to the log
    /// destinations attached to the root logger at the point the Scheduler
    /// was started.
    void log_to_root_logger(Arc::LogLevel level, const std::string& message);

    /// Call the appropriate Process method depending on the DTR state
    void map_state_and_process(DTR_ptr request);
    
    /// Maps the DTR to the appropriate state when it is cancelled.
    /** This is a separate function, since cancellation request
     * can arrive at any time, breaking the normal workflow. */
    void map_cancel_state(DTR_ptr request);
    
    /// Map a DTR stuck in a processing state to new state from which it can
    /// recover and retry.
    void map_stuck_state(DTR_ptr request);

    /// Choose a delivery service for the DTR, based on the file system paths
    /// each service can access. These paths are determined by calling all the
    /// configured services when the first DTR is received.
    void choose_delivery_service(DTR_ptr request);

    /// Go through all DTRs waiting to go into a processing state and decide
    /// whether to push them into that state, depending on shares and limits.
    void revise_queues();

    /// Add a new event for the Scheduler to process. Used in receiveDTR().
    void add_event(DTR_ptr event);

    /// Process the pool of DTRs which have arrived from other processes
    void process_events(void);
    
    /// Move to the next replica in the DTR.
    /** Utility function which should be called in the case of error
     * if the next replica should be tried. It takes care of sending
     * the DTR to the appropriate state, depending on whether or not
     * there are more replicas to try.
     */
    void next_replica(DTR_ptr request);

    /// Handle a DTR whose source is mapped to another URL.
    /** If a file is mapped, this method should be called to deal
     * with the mapping. It sets the mapped_file attribute of
     * request to mapped_url. Returns true if the processing was
     * successful.
     */
    bool handle_mapped_source(DTR_ptr request, Arc::URL& mapped_url);

    /// Thread method for dumping state
    static void dump_thread(void* arg);

    /// Static version of main_thread, used when thread is created
    static void main_thread(void* arg);
    /// Main thread, which runs until stopped
    void main_thread(void);

  public:
  
    /// Get static instance of Scheduler, to use one DTR instance with multiple generators.
    /**
     * Configuration of Scheduler by Set* methods can only be done before
     * start() is called, so undetermined behaviour can result from multiple
     * threads simultaneously calling Set* then start(). It is safer to make
     * sure that all threads use the same configuration (calling start() twice
     * is harmless). It is also better to make sure that threads call stop() in
     * a roughly coordinated way, i.e. all generators stop at the same time.
     */
    static Scheduler* getInstance();

    /// Constructor, to be used when only one Generator uses this Scheduler.
    Scheduler();

    /// Destructor calls stop(), which cancels all DTRs and waits for them to complete
    ~Scheduler() { stop(); };

    /* The following Set/Add methods are only effective when called before start() */

    /// Set number of slots for processor and delivery stages
    void SetSlots(int pre_processor = 0, int post_processor = 0,
                  int delivery = 0, int emergency = 0, int staged_prepared = 0);

    /// Add URL mapping entry. See Arc::URLMap.
    void AddURLMapping(const Arc::URL& template_url, const Arc::URL& replacement_url,
                       const Arc::URL& access_url = Arc::URL());

    /// Replace all URL mapping entries
    void SetURLMapping(const Arc::URLMap& mapping = Arc::URLMap());

    /// Set the preferred pattern for ordering replicas.
    /**
     * This pattern will be used in the case of an index service URL with
     * multiple physical replicas and allows sorting of those replicas in order
     * of preference. It consists of one or more patterns separated by a pipe
     * character (|) listed in order of preference. If the dollar character ($)
     * is used at the end of a pattern, the pattern will be matched to the end
     * of the hostname of the replica. Example: "srm://myhost.org|.uk$|.ch$"
     */
    void SetPreferredPattern(const std::string& pattern);

    /// Set TransferShares configuration
    void SetTransferSharesConf(const TransferSharesConf& share_conf);

    /// Set transfer limits
    void SetTransferParameters(const TransferParameters& params);

    /// Set the list of delivery services. DTR::LOCAL_DELIVERY means local Delivery.
    void SetDeliveryServices(const std::vector<Arc::URL>& endpoints);

    /// Set the remote transfer size limit
    void SetRemoteSizeLimit(unsigned long long int limit);

    /// Set location for periodic dump of DTR state (only file paths currently supported)
    void SetDumpLocation(const std::string& location);

    /// Set JobPerfLog object for performance metrics logging
    void SetJobPerfLog(const Arc::JobPerfLog& perf_log);

    /// Start scheduling activity.
    /**
     * This method must be called after all configuration parameters are set
     * properly. Scheduler can be stopped either by calling stop() method or
     * by destroying its instance.
     */
    bool start(void);
    
    /// Callback method implemented from DTRCallback.
    /**
     * This method is called by the generator when it wants to pass a DTR
     * to the scheduler and when other processes send a DTR back to the
     * scheduler after processing.
     */
    virtual void receiveDTR(DTR_ptr dtr);
    
    /// Tell the Scheduler to cancel all the DTRs in the given job description
    bool cancelDTRs(const std::string& jobid);

    /// Tell the Scheduler to shut down all threads and exit.
    /**
     * All active DTRs are cancelled and this method waits until they finish
     * (all DTRs go to CANCELLED state)
     */
    bool stop();
  };
} // namespace DataStaging

#endif /*SCHEDULER_H_*/
