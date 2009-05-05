
#include "Destination.h"
#include "LutsDestination.h"
//#include "arc/URL.h"

namespace Arc
{

  Destination* Destination::createDestination(Arc::JobLogFile &joblog)
  {
    //std::string url=joblog["loggerurl"];
    //TODO distinguish
    return new LutsDestination(joblog);
  }

}

