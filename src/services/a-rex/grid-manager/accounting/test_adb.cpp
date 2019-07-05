#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include "AccountingDBSQLite.h"


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
}
