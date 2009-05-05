#ifndef DESTINATIONS_H
#define DESTINATIONS_H

#include "Destination.h"
#include "JobLogFile.h"
#include <string>

namespace Arc
{
  class Destinations:public std::map<std::string,Arc::Destination*>
  {
  public:
    void report(Arc::JobLogFile &joblog);
    ~Destinations();
  };

}

#endif
