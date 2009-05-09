#ifndef DESTINATIONS_H
#define DESTINATIONS_H

#include "Destination.h"
#include "JobLogFile.h"
#include <string>

namespace Arc
{
  /** Class to handle a set of reporting destinations. */
  class Destinations:public std::map<std::string,Arc::Destination*>
  {
  public:
    /** Reports the given job log file to a destination. If an adapter
     *  object for the specific destination already exists in the set,
     *  it uses that, otherwise creates a new one.
     */
    void report(Arc::JobLogFile &joblog);
    ~Destinations();
  };

}

#endif
