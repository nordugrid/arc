#ifndef DATA_DELIVERY_H_
#define DATA_DELIVERY_H_

#include <list>

#include <arc/Logger.h>
#include <arc/Thread.h>
#include "DTR.h"
#include "DTRList.h"
#include "DTRStatus.h"

namespace DataStaging {

  /**
   * The DataDelivery takes care of transfering data to/from 
   * specified location. It is a singleton class.
   * Calling receiveDTR() starts new proces which performs
   * data transfer as specified in DTR.
   */
  class DataDelivery: public DTRCallback {	

    private:

    /* lock for dtrs' list */
    Arc::SimpleCondition dtr_list_lock;

    /* DTRs  which delivery process has in its queue.*/
    class delivery_pair_t;
    std::list<delivery_pair_t*> dtr_list;
	
    /* Logger object */
    static Arc::Logger logger;

    /* Singleton instance */
    //static DataDelivery* instance;

    /* Flag describing delivery state. Used to decide whether to keep running
     * main loop */
    ProcessState delivery_state;

    /* lock for events list */
//??  Arc::SimpleCondition event_lock;

    /* Condition to signal end of running */
    Arc::SimpleCondition run_signal;

    static void main_thread(void* arg);
    void main_thread(void);

    /* private constructors and assignment operator */
    DataDelivery(const DataDelivery&);
    DataDelivery& operator=(const DataDelivery&);

  public:
      
    //static DataDelivery* getInstance();
    DataDelivery();
    ~DataDelivery() { stop(); };

    /**
     * This method is called by the scheduler to pass a DTR to the delivery.
     * The DataDelivery starts a process to do the processing, and then returns.
     * The DataDelivery own thread then monitors started process.
     */
    virtual void receiveDTR(DTR&);

    /**
     * Kills the process corresponding to the given DTR
     */
    bool cancelDTR(DTR*);

    bool start();

    /* Tell the delivery to shut down all processes and threads and exit */
    bool stop();

  };   
  
} // namespace DataStaging
#endif /*DATA_DELIVERY_H_*/
