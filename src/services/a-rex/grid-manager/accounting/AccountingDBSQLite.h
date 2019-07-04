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

        // returns accounting database ID for the requested queue
        unsigned int getDBQueueId(const std::string& queue);

      private:
        static Arc::Logger logger;
        // Class to handle SQLite DB Operations
        class SQLiteDB {
        public:
            SQLiteDB(const std::string& name, bool create = false);
            ~SQLiteDB();
            sqlite3* handle() { return aDB; }
            void logError(const char* errpfx, int err, Arc::LogLevel level = Arc::DEBUG);
        private:
            sqlite3* aDB;
            void closeDB();
        };
        /**
         * General helper that retrurn accounting database ID for requested iname 
         * performing lookup in the table with the [ID, Name] columns
         * updates the name_id_map man of the object
         **/
        unsigned int QueryAndInsertNameID(const std::string& table, const std::string& iname, name_id_map_t* name_id_map);
    };
}

#endif
