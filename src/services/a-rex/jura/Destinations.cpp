#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>

#include "Destinations.h"

namespace ArcJura
{

  void Destinations::report(JobLogFile &joblog)
  {
    std::string dest_id=joblog["loggerurl"]; 
    //TODO same service URL with different reporting parameters?

    if (find(dest_id)==end()) //New destination
      {
        // Create the appropriate adapter
        Destination *dest = Destination::createDestination(joblog);
        if (dest)
          (*this)[dest_id] = dest;
        else
          {
            Arc::Logger logger(Arc::Logger::rootLogger, "JURA.Destinations");
            logger.msg(Arc::ERROR, 
                       "Unable to create adapter for the specific reporting destination type");
            return;
          }
      }
    
    (*this)[dest_id]->report(joblog);
  }

  Destinations::~Destinations()
  {
    for (Destinations::iterator it=begin();
         it!=end();
         ++it)
      {
        delete (*it).second;
      }
  }
}
