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
      if (adb.IsValid()) {
         std::cerr << "Database connection is successfull" << std::endl;
      }
}
