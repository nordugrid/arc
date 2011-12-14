
#include "Destination.h"
#include "LutsDestination.h"
#include "ApelDestination.h"
#include "CARDestination.h"

namespace Arc
{

  Destination* Destination::createDestination(Arc::JobLogFile &joblog)
  {
    std::string url=joblog["loggerurl"];
    if (url.substr(0,3) == "CAR") {
        return new CARDestination(joblog);
    }
    //TODO distinguish
    if ( !joblog["topic"].empty() ||
         url.substr(0,4) == "APEL"){
        return new ApelDestination(joblog);
    }else{
        return new LutsDestination(joblog);
    }
  }

}

