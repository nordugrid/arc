#ifndef ARC_ACCOUNTING_DB_SQLITE_H
#define ARC_ACCOUNTING_DB_SQLITE_H

#include <string>
#include <sqlite3.h>
#include <arc/Logger.h>

#include "AccountingDB.h"

namespace ARex {

    /// Class implementing A-REX accounting records (AAR) storing in SQLite
    class AccountingDBSQLite : public AccountingDB {
      public:
        AccountingDBSQLite(const std::string& name);
        virtual ~AccountingDBSQLite() {}

//        unsigned int getDBQueue(const std::string& queue);

      private:
        static Arc::Logger logger;
        // Class to handle SQLite DB Operations
        class SQLiteDB {
        public:
            SQLiteDB(const std::string& name, bool create = false);
            ~SQLiteDB();
            sqlite3* handle() { return aDB; }
        private:
            sqlite3* aDB;
            void logError(const char* errpfx, int err, Arc::LogLevel level = Arc::DEBUG);
            void closeDB();
        };  
    };
}

#endif
