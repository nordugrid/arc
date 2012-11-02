#ifndef DESTINATION_H
#define DESTINATION_H


#include "JobLogFile.h"
#include <arc/communication/ClientInterface.h>
#include <arc/message/MCC.h>

namespace Arc
{
  
  /** Abstract class to represent a reporting destination.
   *  Specific destination types are represented by inherited classes. 
   */
  class Destination
  {
  public:
    /** Reports the job log file content to the destination. */
    virtual void report(Arc::JobLogFile &joblog)=0;
    /** Finishes pending submission of records. */
    virtual void finish() {};
    virtual ~Destination() {}

    /** Creates an instance of the inherited class corresponding to
     *  the destination for the given job log file. 
     */
    static Destination* createDestination(Arc::JobLogFile &joblog);
    std::string Current_Time( time_t parameter_time = time(NULL) );
    Arc::MCC_Status OutputFileGeneration(std::string prefix, Arc::URL url, std::string output_dir, std::string message,Arc::Logger& logger);
  };
}

#endif
