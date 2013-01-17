#ifndef DATA_DELIVERY_H_
#define DATA_DELIVERY_H_

#include <list>

#include <arc/Logger.h>
#include <arc/Thread.h>
#include "DTR.h"
#include "DTRList.h"
#include "DTRStatus.h"

namespace DataStaging {

  /// DataDelivery transfers data between specified physical locations.
  /**
   * start() must be called to start the delivery thread for processing DTRs
   * and stop() should be called to stop it (this waits for all data transfers
   * to exit). stop() is also called in the destructor.
   *
   * All meta-operations for a DTR such as resolving replicas must be done
   * before sending to DataDelivery. Calling receiveDTR() starts a new process
   * which performs data transfer as specified in DTR.
   * \ingroup datastaging
   * \headerfile DataDelivery.h arc/data-staging/DataDelivery.h
   */
  class DataDelivery: public DTRCallback {	

   private:

    /// lock for DTRs list
    Arc::SimpleCondition dtr_list_lock;

    /// Wrapper class around delivery process handler
    class delivery_pair_t;
    /// DTRs which delivery process has in its queue
    std::list<delivery_pair_t*> dtr_list;

    /// Transfer limits
    TransferParameters transfer_params;

    /// Logger object
    static Arc::Logger logger;

    /// Flag describing delivery state. Used to decide whether to keep running main loop
    ProcessState delivery_state;

    /// Condition to signal end of running
    Arc::SimpleCondition run_signal;

    /// Condition on which main thread waits, so it can wake up immediately
    /// when a new transfer arrives
    Arc::SimpleCondition cond;

    /// Thread to start new Delivery process
    static void start_delivery(void* arg);

    /// Thread to stop Delivery process
    static void stop_delivery(void* arg);

    /// Delete delivery_pair_t object. Starts a new thread which calls stop_delivery()
    bool delete_delivery_pair(delivery_pair_t* dp);

    /// Static version of main_thread, used when thread is created
    static void main_thread(void* arg);
    /// Main thread, which runs until stopped
    void main_thread(void);

    /// Copy constructor is private because DataDelivery should not be copied
    DataDelivery(const DataDelivery&);
    /// Assignment constructor is private because DataDelivery should not be copied
    DataDelivery& operator=(const DataDelivery&);

   public:
      
    /// Constructor.
    DataDelivery();
    /// Destructor calls stop() and waits for cancelled processes to exit.
    ~DataDelivery() { stop(); };

    /// Pass a DTR to Delivery.
    /**
     * This method is called by the scheduler to pass a DTR to the delivery.
     * The DataDelivery starts the data transfer either using a local process
     * or by sending a request to a remote delivery service, and then returns.
     * DataDelivery's own thread then monitors the transfer.
     */
    virtual void receiveDTR(DTR_ptr request);

    /// Stop the transfer corresponding to the given DTR.
    bool cancelDTR(DTR_ptr request);

    /// Start the Delivery thread, which runs until stop() is called.
    bool start();

    /// Tell the delivery to stop all transfers and threads and exit.
    bool stop();

    /// Set transfer limits.
    void SetTransferParameters(const TransferParameters& params);

  };   
  
} // namespace DataStaging
#endif /*DATA_DELIVERY_H_*/
