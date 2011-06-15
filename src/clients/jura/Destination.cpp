
#include "Destination.h"
#include "LutsDestination.h"
#include "ApelDestination.h"
//#include "arc/URL.h"

namespace Arc
{

  Destination* Destination::createDestination(Arc::JobLogFile &joblog)
  {
    //std::string url=joblog["loggerurl"];
    //TODO distinguish
    if ( !joblog["topic"].empty()){
        return new ApelDestination(joblog);
    }else{
        return new LutsDestination(joblog);
    }
  }

}

