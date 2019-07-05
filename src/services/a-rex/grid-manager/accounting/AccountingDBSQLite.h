#ifndef ARC_ACCOUNTING_DB_SQLITE_H
#define ARC_ACCOUNTING_DB_SQLITE_H

#include <string>
#include <map>
#include <sqlite3.h>
#include <arc/Logger.h>

#include "AccountingDB.h"

namespace ARex {
    /// Store name to ID mappings in database tables
    typedef std::map <std::string, unsigned int> name_id_map_t;

    /// Class implementing A-REX accounting records (AAR) storing in SQLite
    class AccountingDBSQLite : public AccountingDB {
      public:
        AccountingDBSQLite(const std::string& name);
        ~AccountingDBSQLite();

        /// Get database ID for the specified queue
        /**
         * This method fetches the database to get stored Queue
         * primary key. If requested Queue is missing in the database
         * it will be created and inserted ID is returned.
         *
         * Updates db_queue map.
         *
         * If db_queue map is already populated with database data and Queue name
         * is already inside the map, cached data will be returned.
         *
         * In case of failure to find and insert the queue name in the database
         * returns 0.
         *
         * @return database primary key for provided queue
         **/
        unsigned int getDBQueueId(const std::string& queue);
        /// Get database ID for the specified user DN
        unsigned int getDBUserId(const std::string& userdn);
        /// Get database ID for the specified WLCG VO name
        unsigned int getDBWLCGVOId(const std::string& voname);
        /// Get database ID for the specified status string
        unsigned int getDBStatusId(const std::string& status);

        /// Get endpoint ID
        unsigned int getDBEndpointId(const aar_endpoint_t& endpoint);
      private:
        static Arc::Logger logger;
        // General Name-ID tables
        name_id_map_t db_queue;
        name_id_map_t db_users;
        name_id_map_t db_wlcgvos;
        name_id_map_t db_status;
        // AAR specific structures representation
        std::map <aar_endpoint_t, unsigned int> db_endpoints;
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

        SQLiteDB* db;
        void initSQLiteDB(void);
        void closeSQLiteDB(void);
        /**
         * General helper that retrurn accounting database ID for requested iname 
         * performing lookup in the table with the [ID, Name] columns
         * updates the name_id_map map of the object
         **/
        unsigned int QueryAndInsertNameID(const std::string& table, const std::string& iname, name_id_map_t* name_id_map);
        /**
         * General helper to execute INSERT statement and return the autoincrement ID
         **/
        unsigned int GeneralSQLInsert(const std::string& sql);
        /**
         * General helper that query [ID, Name] table and put the result into the the name_id_map map
         **/
        bool QueryNameIDmap(const std::string& table, name_id_map_t* name_id_map);
        // Query special tables
        bool QueryEnpointsmap(void);
    };
}

#endif
