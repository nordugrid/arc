#ifndef DESTINATION_H
#define DESTINATION_H


#include "JobLogFile.h"

namespace Arc
{
  
  class Destination
  {
  public:
    virtual void report(Arc::JobLogFile &joblog)=0;
    virtual void finish() {};
    virtual ~Destination() {}

    static Destination* createDestination(Arc::JobLogFile &joblog);
  };
}

#endif
