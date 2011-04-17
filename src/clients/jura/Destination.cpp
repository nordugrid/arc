
#include "Destination.h"
#include "LutsDestination.h"
//#include "ApelDestination.h"
//#include "arc/URL.h"

namespace Arc
{

  Destination* Destination::createDestination(Arc::JobLogFile &joblog)
  {
    //std::string url=joblog["loggerurl"];
    //TODO distinguish
    if ( !joblog["topic"].empty()){
        //If the Stomp will be ready, then need to be use the ApelDestination class.
        return new LutsDestination(joblog);
        //return new ApelDestination(joblog);
    }else{
        return new LutsDestination(joblog);
    }
  }

}

