#ifndef SCHEDULER_H_
#define SCHEDULER_H_

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

    /// A lock for the cancelled jobs list
    Arc::SimpleCondition cancelled_jobs_lock;

    /// Transfer shares the scheduler uses
    TransferShares transferShares;

    /// URLMap containing information on any local mappings defined in the configuration
    Arc::URLMap url_map;

    /// Preferred pattern to match replicas defined in configuration
    std::string preferred_pattern;
      
    /// Lock for events list
    Arc::SimpleCondition event_lock;

    /// Condition to signal end of running
    Arc::SimpleCondition run_signal;

    /// Limit on number of DTRs in pre-processor
    int PreProcessorSlots;
    /// Limit on number of DTRs in delivery
    int DeliverySlots;
    /// Limit on number of emergency DTRs in delivery
    int DeliveryEmergencySlots;
    /// Limit on number of DTRs in post-processor
    int PostProcessorSlots;

    /// Where to dump DTR state. Currently only a path to a file is supported.
    std::string dumplocation;

    /// Endpoints of remote delivery services.
    std::vector<Arc::URL> remote_delivery;

    /// Logger object
    static Arc::Logger logger;

    /// Flag describing scheduler state. Used to decide whether to keep running main loop.
    ProcessState scheduler_state;

    /// Processor object
    Processor processor;

    /// Delivery object
    DataDelivery delivery;

    /// Copy constructor is private because Scheduler should not be copied
    Scheduler(const Scheduler&); // should not happen
    /// Assignment operator is private because Scheduler should not be copied
    Scheduler& operator=(const Scheduler&); // should not happen

    /* Functions to process every state of the DTR during normal workflow */

    /// Process a DTR in the NEW state
    void ProcessDTRNEW(DTR* request);
    /// Process a DTR in the CACHE_WAIT state
    void ProcessDTRCACHE_WAIT(DTR* request);
    /// Process a DTR in the CACHE_CHECKED state
    void ProcessDTRCACHE_CHECKED(DTR* request);
    /// Process a DTR in the RESOLVED state
    void ProcessDTRRESOLVED(DTR* request);
    /// Process a DTR in the REPLICA_QUERIED state
    void ProcessDTRREPLICA_QUERIED(DTR* request);
    /// Process a DTR in the PRE_CLEANED state
    void ProcessDTRPRE_CLEANED(DTR* request);
    /// Process a DTR in the STAGING_PREPARING_WAIT state
    void ProcessDTRSTAGING_PREPARING_WAIT(DTR* request);
    /// Process a DTR in the STAGED_PREPARED state
    void ProcessDTRSTAGED_PREPARED(DTR* request);
    /// Process a DTR in the TRANSFERRED state
    void ProcessDTRTRANSFERRED(DTR* request);
    /// Process a DTR in the REQUEST_RELEASED state
    void ProcessDTRREQUEST_RELEASED(DTR* request);
    /// Process a DTR in the REPLICA_REGISTERED state
    void ProcessDTRREPLICA_REGISTERED(DTR* request);
    /// Process a DTR in the CACHE_PROCESSED state
    void ProcessDTRCACHE_PROCESSED(DTR* request);
    /// Process a DTR in a final state
    /* This is a special function to deal with states after which
     * the DTR is returned to the generator, i.e. DONE, ERROR, CANCELLED */
    void ProcessDTRFINAL_STATE(DTR* request);
   	
    /* Compute (flatten) the priority of the DTR
     * Sets the priority, according to which internal
     * queues are sorted
     * This function may take into account timeout of the DTR,
     * so for these about to expire the priority will be boosted
     */ 
    //void compute_priority(DTR* request);
    
    /// Call the appropriate Process method depending on the DTR state
    void map_state_and_process(DTR* request);
    
    /// Call the appropriate Process method when the DTR has a pending cancel request.
    /** This is a separate function, since cancellation request
     * can arrive at any time, breaking the normal workflow. */
    void map_cancel_state_and_process(DTR* request);
    
    /* Functions to loop through DTRs awaiting sending to proper processes. */

    /// Go through the list of DTRs waiting to go to the pre-processor and possibly submit them
    void revise_pre_processor_queue();
    /// Go through the list of DTRs waiting to go to the post-processor and possibly submit them
    void revise_post_processor_queue();
    /// Go through the list of DTRs waiting to go to the delivery and possibly submit them
    void revise_delivery_queue();
    
    /// Process the pool of DTRs which have arrived from other processes
    void process_events(void);
    
    /// Move to the next replica in the DTR.
    /** Utility function which should be called in the case of error
     * if the next replica should be tried. It takes care of sending
     * the DTR to the appropriate state, depending on whether or not
     * there are more replicas to try.
     */
    void next_replica(DTR* request);

    /// Handle a DTR whose source is mapped to another URL.
    /** If a file is mapped, this method should be called to deal
     * with the mapping. It sets the mapped_file attribute of
     * request to mapped_url. Returns true if the processing was
     * successful.
     */
    bool handle_mapped_source(DTR* request, Arc::URL& mapped_url);

    /// Static version of main_thread, used when thread is created
    static void main_thread(void* arg);
    /// Main thread, which runs until stopped
    void main_thread(void);

  public:
  
    /// Constructor
    Scheduler();

    /// Destructor calls stop(), which cancels all DTRs and waits for them to complete
    ~Scheduler() { stop(); };

    /* The following Set/Add methods are only effective when called before start() */

    /// Set number of slots for processor and delivery stages
    void SetSlots(int pre_processor = 0, int post_processor = 0,
                  int delivery = 0, int delivery_emergency = 0);

    /// Add URL mapping entry
    void AddURLMapping(const Arc::URL& template_url, const Arc::URL& replacement_url,
                       const Arc::URL& access_url = Arc::URL());

    /// Replace all URL mapping entries
    void SetURLMapping(const Arc::URLMap& mapping = Arc::URLMap());

    /// Set the preferred pattern
    void SetPreferredPattern(const std::string& pattern);

    /// Set TransferShares
    void SetTransferShares(const TransferShares& shares);

    /// Add share
    void AddSharePriority(const std::string& name, int priority);

    /// Replace all shares
    void SetSharePriorities(const std::map<std::string, int>& shares);

    /// Set share type
    void SetShareType(TransferShares::ShareType share_type);

    /// Set transfer limits
    void SetTransferParameters(const TransferParameters& params);

    /// Set the list of remote delivery endpoints
    void SetRemoteDeliveryServices(const std::vector<Arc::URL>& endpoints);

    /// Set location for periodic dump of DTR state (only file paths currently supported)
    void SetDumpLocation(const std::string& location);

    /// Start scheduling activity.
    /** This method must be called after all configuration parameters are set
     * properly. Scheduler can be stopped either by calling stop() method or
     * by destroying its instance. */
    bool start(void);
    
    /// Callback method implemented from DTRCallback.
    /** This method is called by the generator when it wants to pass a DTR
     * to the scheduler. */
    virtual void receiveDTR(DTR& dtr);
    
    /// Tell the Scheduler to cancel all the DTRs in the given job description
    bool cancelDTRs(const std::string& jobid);

    /// Tell the Scheduler to shut down all threads and exit.
    /** All active DTRs are cancelled and this method waits until they finish
     * (all DTRs go to CANCELLED state) */
    bool stop();
  };
} // namespace DataStaging

#endif /*SCHEDULER_H_*/
