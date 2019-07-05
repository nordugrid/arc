#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include "AccountingDBSQLite.h"
#include "AARContent.h"


int main(int argc, char **argv) {
      Arc::LogStream logcerr(std::cerr);
      Arc::Logger::getRootLogger().addDestination(logcerr);
      Arc::Logger::getRootLogger().setThreshold(Arc::DEBUG);

      ARex::AccountingDBSQLite adb("/tmp/adb.sqlite");
      if (!adb.IsValid()) {
         std::cerr << "Database connection was not successfull" << std::endl;
         return EXIT_FAILURE;
      }

      std::cerr << "Queue ID:" << adb.getDBQueueId("atlas_prod") << std::endl;
      std::cerr << "User ID:" << adb.getDBUserId("/DC=org/DC=ugrid/O=people/O=KNU/CN=Andrii Salnikov") << std::endl;
      std::cerr << "WLCG VO ID:" << adb.getDBWLCGVOId("atlas") << std::endl;
      std::cerr << "Status ID:" << adb.getDBStatusId("completed") << std::endl;

      ARex::aar_endpoint_t endpoint1 {"org.ogf.glue.emies.activitycreation", "https://arc6.univ.kiev.ua:443/arex"};
      ARex::aar_endpoint_t endpoint2 {"org.nordugrid.arcrest", "https://arc6.univ.kiev.ua:443/arex"};
      std::cerr << "Endpoint ID:" << adb.getDBEndpointId(endpoint1) << std::endl;
      std::cerr << "Endpoint ID:" << adb.getDBEndpointId(endpoint2) << std::endl;
}
