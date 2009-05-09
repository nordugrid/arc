#ifndef DESTINATION_H
#define DESTINATION_H


#include "JobLogFile.h"

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
  };
}

#endif
