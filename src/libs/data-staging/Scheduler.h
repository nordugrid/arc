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

class Scheduler: public DTRCallback {	

  private:
  
    /* All the DTRs the scheduler is aware of. The DTR 
     * comes to this list once received from the generator
     * and leaves the list only when pushed back to the generator
     */
    DTRList DtrList;
    
    /* A list of jobs that have been requested to be cancelled.
     * External threads add items to this list, and the Scheduler
     * processes it during the main loop
     */
    std::list<std::string> cancelled_jobs;

    /* A lock for the cancelled jobs list */
    Arc::SimpleCondition cancelled_jobs_lock;

    /* Transfer shares the scheduler uses
     */
    TransferShares transferShares;

    /* URLMap containing information on any local mappings
     * defined in the configuration */
    Arc::URLMap url_map;

    /* Preferred pattern to match replicas defined in configuration */
    std::string preferred_pattern;
      
    /* lock for events list */
    Arc::SimpleCondition event_lock;

    /* Condition to signal end of running */
    Arc::SimpleCondition run_signal;

    /* Limits on number of DTRs in each state. Will be obtained from configuration */
    int PreProcessorSlots;
    int DeliverySlots;
    int DeliveryEmergencySlots;
    int PostProcessorSlots;

    /* Where to dump DTR state */
    std::string dumplocation;

    /* Logger object */
    static Arc::Logger logger;

    /* Flag describing scheduler state. Used to decide whether to keep running
     * main loop */
    ProcessState scheduler_state;

    Processor processor;

    DataDelivery delivery;

    /* private constructors and assignment operator */
    Scheduler(const Scheduler&); // should not happen
    Scheduler& operator=(const Scheduler&); // should not happen

    /* Functions to process every state of 
     * the DTR during normal workflow
     */
    void ProcessDTRNEW(DTR* request);
    void ProcessDTRCACHE_WAIT(DTR* request);
    void ProcessDTRCACHE_CHECKED(DTR* request);
    void ProcessDTRRESOLVED(DTR* request);
    void ProcessDTRREPLICA_QUERIED(DTR* request);
    void ProcessDTRPRE_CLEANED(DTR* request);
    void ProcessDTRSTAGING_PREPARING_WAIT(DTR* request);
    void ProcessDTRSTAGED_PREPARED(DTR* request);
    void ProcessDTRTRANSFERRED(DTR* request);
    void ProcessDTRREQUEST_RELEASED(DTR* request);
    void ProcessDTRREPLICA_REGISTERED(DTR* request);
    void ProcessDTRCACHE_PROCESSED(DTR* request);
    /* A special function to deal with states after which
     * the DTR is returned to the generator,
     * i.e. DONE, ERROR, CANCELLED
     */
    void ProcessDTRFINAL_STATE(DTR* request);
   	
    /* Compute (flatten) the priority of the DTR
     * Sets the priority, according to which internal
     * queues are sorted
     * This function may take into account timeout of the DTR,
     * so for these about to expire the priority will be boosted
     */ 
    //void compute_priority(DTR* request);
    
    /* Do necessary things for the DTR in the specific state
     * For normal scheduler workflow
     */
    void map_state_and_process(DTR* request);
    
    /* Do necessary things for the DTR in the specific state
     * with a pending cancel request.
     * This is a separate function, since cancellation request
     * can arrive at any time, breaking the normal workflow
     */ 
    void map_cancel_state_and_process(DTR* request);
    
    /* Functions to loop through DTRs awaiting sending to 
     * proper processes.
     */
    void revise_pre_processor_queue();
    void revise_post_processor_queue();
    void revise_delivery_queue();
    
    /* Process the pool of DTRs, arrived from other processes 
     */
    void process_events(void);
    
    /* Utility function which should be called in the case of error
     * if the next replica should be tried. It takes care of sending
     * the DTR to the appropriate state, depending on whether or not
     * there are more replicas to try.
     */
    void next_replica(DTR* request);

    /* If a file is mapped, this method should be called to deal
     * with the mapping. It sets the mapped_file attribute of
     * request to mapped_url. Returns true if the processing was
     * successful.
     */
    bool handle_mapped_source(DTR* request, Arc::URL& mapped_url);

  public:
  
    /* Main constructor */
    Scheduler();

    ~Scheduler() { stop(); };

    /* The following Set/Add methods are only effective when called before start() */

    /* Set number of slots for processor and delivery stages */
    void SetSlots(int pre_processor = 0, int post_processor = 0,
                  int delivery = 0, int delivery_emergency = 0);

    /* Add URL mapping entry */
    void AddURLMapping(const Arc::URL& template_url, const Arc::URL& replacement_url,
                       const Arc::URL& access_url = Arc::URL());

    /* Replace all URL mapping entries*/
    void SetURLMapping(const Arc::URLMap& mapping = Arc::URLMap());

    /* Set the preferred pattern */
    void SetPreferredPattern(const std::string& pattern);

    /* Set TransferShares */
    void SetTransferShares(const TransferShares& shares);

    /* Add share */
    void AddSharePriority(const std::string& name, int priority);

    /* Replace all shares */
    void SetSharePriorities(const std::map<std::string, int>& shares);

    /* Set share type */
    void SetShareType(TransferShares::ShareType share_type);

    /* Set transfer limits */
    void SetTransferParameters(const TransferParameters& params);

    /* Set location for periodic dump of DTR state (only file paths
     * currently supported) */
    void SetDumpLocation(const std::string& location);

    /* Start scheduling activity.
     * This method must be called after all configuration
       parameters are set properly. Scheduler can be stopped 
       either by calling stop() method or by destroying 
       its instance. */
    bool start(void);
    
    /* This method is called by the generator if it wants to
     * pass a DTR to the scheduler.
     */
    virtual void receiveDTR(DTR& dtr);
    
    /* Tell the Scheduler to cancel all the DTRs in the given
     * job description.
     */
    bool cancelDTRs(const std::string& jobid);

    /* Tell the Scheduler to shut down all threads and exit
     */
    bool stop();

    static void main_thread(void* arg);
    void main_thread(void);
  };
} // namespace DataStaging

#endif /*SCHEDULER_H_*/
