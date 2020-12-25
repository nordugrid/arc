#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/FileUtils.h>
#include <arc/DateTime.h>
#include <arc/ArcLocation.h>
#include <sys/stat.h>

#include "../../SQLhelpers.h"

#include "AccountingDBSQLite.h"

#define DB_SCHEMA_FILE "arex_accounting_db_schema_v1.sql"

namespace ARex {
    Arc::Logger AccountingDBSQLite::logger(Arc::Logger::getRootLogger(), "AccountingDBSQLite");

    int AccountingDBSQLite::SQLiteDB::exec(const char *sql, int (*callback)(void*,int,char**,char**), 
        void *arg, char **errmsg) {
        int err;
        while((err = sqlite3_exec(aDB, sql, callback, arg, errmsg)) == SQLITE_BUSY) {
            // Access to database is designed in such way that it should not block for long time.
            // So it should be safe to simply wait for lock to be released without any timeout.
            struct timespec delay = { 0, 10000000 }; // 0.01s - should be enough for most cases
            (void)::nanosleep(&delay, NULL);
        };
        return err;
    }

    AccountingDBSQLite::SQLiteDB::SQLiteDB(const std::string& name, bool create): aDB(NULL) {
        if (aDB != NULL) return; // already open

        int flags = SQLITE_OPEN_READWRITE;
        if (create) flags |= SQLITE_OPEN_CREATE;
        int err;
        while((err = sqlite3_open_v2(name.c_str(), &aDB, flags, NULL)) == SQLITE_BUSY) {
            // In case something prevents database from open right now - retry
            closeDB();
            struct timespec delay = { 0, 10000000 }; // 0.01s - should be enough for most cases
            (void)::nanosleep(&delay, NULL);
        };
        if(err != SQLITE_OK) {
            logError("Unable to open accounting database connection", err, Arc::ERROR);
            closeDB();
            return;
        };
        if (create) {
            std::string db_schema_str;
            std::string sql_file = Arc::ArcLocation::Get() + G_DIR_SEPARATOR_S + PKGDATASUBDIR + 
                G_DIR_SEPARATOR_S + "sql-schema" + G_DIR_SEPARATOR_S + DB_SCHEMA_FILE;
            if(!Arc::FileRead(sql_file, db_schema_str)) {
                AccountingDBSQLite::logger.msg(Arc::ERROR, "Failed to read database schema file at %s", sql_file);
                closeDB();
                return;
            }
            err = exec(db_schema_str.c_str(), NULL, NULL, NULL);
            if(err != SQLITE_OK) {
                logError("Failed to initialize accounting database schema", err, Arc::ERROR);
                closeDB();
                return;
            }
            AccountingDBSQLite::logger.msg(Arc::INFO, "Accounting database initialized succesfully");
        }
        AccountingDBSQLite::logger.msg(Arc::DEBUG, "Accounting database connection has been established");
    }

    void AccountingDBSQLite::SQLiteDB::logError(const char* errpfx, int err, Arc::LogLevel loglevel) {
#ifdef HAVE_SQLITE3_ERRSTR
        std::string msg = sqlite3_errstr(err);
#else
        std::string msg = "error code "+Arc::tostring(err);
#endif
        if (errpfx) {
            AccountingDBSQLite::logger.msg(loglevel, "%s. SQLite database error: %s", errpfx, msg);
        } else {
            AccountingDBSQLite::logger.msg(loglevel, "SQLite database error: %s", msg);
        }
    }

    bool AccountingDBSQLite::SQLiteDB::isConnected(void) {
        if (aDB) return true;
        return false;
    }

    void AccountingDBSQLite::SQLiteDB::closeDB(void) {
        if (aDB) {
            (void)sqlite3_close(aDB); // TODO: handle errors?
            aDB = NULL;
        };
    }

    AccountingDBSQLite::SQLiteDB::~SQLiteDB() {
        closeDB();
    }

    AccountingDBSQLite::AccountingDBSQLite(const std::string& name) : AccountingDB(name), db(NULL) {
        isValid = false;
        // check database file exists
        if (!Glib::file_test(name, Glib::FILE_TEST_EXISTS)) {
            const std::string dbdir = Glib::path_get_dirname(name);
            // Check if the parent directory exist
            if (!Glib::file_test(dbdir, Glib::FILE_TEST_EXISTS)) {
                if (Arc::DirCreate(dbdir, S_IRWXU, true)) {
                    logger.msg(Arc::INFO, "Directory %s to store accounting database has been created.", dbdir);
                } else {
                    logger.msg(Arc::ERROR, "Accounting database cannot be created. Faile to create parent directory %s.", dbdir);
                    return;
                }
            } else if (!Glib::file_test(dbdir, Glib::FILE_TEST_IS_DIR)) {
                logger.msg(Arc::ERROR, "Accounting database cannot be created: %s is not a directory", dbdir);
                return;
            }
            // initialize new database
            Glib::Mutex::Lock lock(lock_);
            db = new SQLiteDB(name, true);
            if (!db->isConnected()){
                logger.msg(Arc::ERROR, "Failed to initialize accounting database");
                closeSQLiteDB();
                return;
            }
            isValid = true;
            return;
        } else if (!Glib::file_test(name, Glib::FILE_TEST_IS_REGULAR)) {
            logger.msg(Arc::ERROR, "Accounting database file (%s) is not a regular file", name);
            return;
        }
        // if we are here database location is fine, trying to open
        initSQLiteDB();
        if (!db->isConnected()) {
            logger.msg(Arc::ERROR, "Error opening accounting database");
            closeSQLiteDB();
            return;
        }
        // TODO: implement schema version checking and possible updates
        isValid = true;
    }

    // init DB connection for multiple usages
    void AccountingDBSQLite::initSQLiteDB(void) {
        // already initialized
        if (db) return;
        db = new SQLiteDB(name);
    }

    // close DB connection
    void AccountingDBSQLite::closeSQLiteDB(void) {
        if (db) {
            logger.msg(Arc::DEBUG, "Closing connection to SQLite accounting database");
            delete db;
            db = NULL;
        }
    }

    AccountingDBSQLite::~AccountingDBSQLite() {
        closeSQLiteDB();
    }

    // perform insert query and return
    //  0 - failure
    //  id - autoincrement id of the inserted raw
    unsigned int AccountingDBSQLite::GeneralSQLInsert(const std::string& sql) {
        if (!isValid) return 0;
        initSQLiteDB();
        Glib::Mutex::Lock lock(lock_);
        int err;
        err = db->exec(sql.c_str(), NULL, NULL, NULL);
        if (err != SQLITE_OK) {
            if (err == SQLITE_CONSTRAINT) {
                db->logError("It seams record exists already", err, Arc::ERROR);
            } else {
                db->logError("Failed to insert data into database", err, Arc::ERROR);
            }
            return 0;
        }
        if(db->changes() < 1) {
            return 0;
        }
        sqlite3_int64 newid = db->insertID();
        return (unsigned int) newid;
    }

    // perform update query
    bool AccountingDBSQLite::GeneralSQLUpdate(const std::string& sql) {
        if (!isValid) return false;
        initSQLiteDB();
        Glib::Mutex::Lock lock(lock_);
        int err;
        err = db->exec(sql.c_str(), NULL, NULL, NULL);
        if (err != SQLITE_OK ) {
            db->logError("Failed to update data in the database", err, Arc::ERROR);
            return false;
        }
        if(db->changes() < 1) {
            return false;
        }
        return true;
    }

    // callback to build (name,id) map from database table
    static int ReadIdNameCallback(void* arg, int colnum, char** texts, char** names) {
        name_id_map_t* name_id_map = static_cast<name_id_map_t*>(arg);
        std::pair <std::string, unsigned int> rec;
        rec.second = 0;
        for (int n = 0; n < colnum; ++n) {
            if (names[n] && texts[n]) {
                if (strcmp(names[n], "ID") == 0) {
                    int id;
                    sql_unescape(texts[n], id);
                    rec.second = id;
                } else if (strcmp(names[n], "Name") == 0) {
                    rec.first = sql_unescape(texts[n]);
                }
            }
        }
        if(rec.second) name_id_map->insert(rec);
        return 0;
    }

    bool AccountingDBSQLite::QueryNameIDmap(const std::string& table, name_id_map_t* name_id_map) {
        if (!isValid) return false;
        initSQLiteDB();
        // empty map corresponding to the table if not empty
        if (!name_id_map->empty()) name_id_map->clear();
        std::string sql = "SELECT * FROM " + sql_escape(table);
        if (db->exec(sql.c_str(), &ReadIdNameCallback, name_id_map, NULL) != SQLITE_OK ) {
            return false;
        }
        return true;
    }


    // general helper to build (name,id) map from database table
    unsigned int AccountingDBSQLite::QueryAndInsertNameID(const std::string& table, const std::string& iname, name_id_map_t* name_id_map) {
        // fill map with db values
        if (name_id_map->empty()) {
            if (!QueryNameIDmap(table, name_id_map)) {
                logger.msg(Arc::ERROR, "Failed to fetch data from %s accounting database table", table);
                return 0;
            }
        }
        // find name
        name_id_map_t::iterator it;
        it = name_id_map->find(iname);
        if (it != name_id_map->end()) {
            return it->second;
        } else {
            // if not found - create the new record in the database
            std::string sql = "INSERT INTO " + sql_escape(table) + " (Name) VALUES ('" + sql_escape(iname) + "')";
            unsigned int newid = GeneralSQLInsert(sql);
            if ( newid ) {
                name_id_map->insert(std::pair <std::string, unsigned int>(iname, newid));
                return newid;
            } else {
                logger.msg(Arc::ERROR, "Failed to add '%s' into the accounting database %s table", iname, table);
            }
        }
        return 0;
    }

    unsigned int AccountingDBSQLite::getDBQueueId(const std::string& queue) {
        return QueryAndInsertNameID("Queues", queue, &db_queue);
    }

    unsigned int AccountingDBSQLite::getDBUserId(const std::string& userdn) {
        return QueryAndInsertNameID("Users", userdn, &db_users);
    }

    unsigned int AccountingDBSQLite::getDBWLCGVOId(const std::string& voname) {
        return QueryAndInsertNameID("WLCGVOs", voname, &db_wlcgvos);
    }

    unsigned int AccountingDBSQLite::getDBStatusId(const std::string& status) {
        return QueryAndInsertNameID("Status", status, &db_status);
    }

    // endpoints
    static int ReadEndpointsCallback(void* arg, int colnum, char** texts, char** names) {
        std::map <aar_endpoint_t, unsigned int>* endpoints_map = static_cast<std::map <aar_endpoint_t, unsigned int>*>(arg);
        std::pair <aar_endpoint_t, unsigned int> rec;
        for (int n = 0; n < colnum; ++n) {
            if (names[n] && texts[n]) {
                if (strcmp(names[n], "ID") == 0) {
                    int id;
                    sql_unescape(texts[n], id);
                    rec.second = id;
                } else if (strcmp(names[n], "Interface") == 0) {
                    rec.first.interface = sql_unescape(texts[n]);
                } else if (strcmp(names[n], "URL") == 0) {
                    rec.first.url = sql_unescape(texts[n]);
                } 
            }
        }
        endpoints_map->insert(rec);
        return 0;
    }

    bool AccountingDBSQLite::QueryEnpointsmap() {
        if (!isValid) return false;
        initSQLiteDB();
        // empty map corresponding to the table if not empty
        if (!db_endpoints.empty()) db_endpoints.clear();
        std::string sql = "SELECT * FROM Endpoints";
        if (db->exec(sql.c_str(), &ReadEndpointsCallback, &db_endpoints, NULL) != SQLITE_OK ) {
            return false;
        }
        return true;
    }

    unsigned int AccountingDBSQLite::getDBEndpointId(const aar_endpoint_t& endpoint) {
        // fill map with db values
        if (db_endpoints.empty()) {
            if (!QueryEnpointsmap()) {
                logger.msg(Arc::ERROR, "Failed to fetch data from accounting database Endpoints table");
                return 0;
            }
        }
        // find endpoint
        std::map <aar_endpoint_t, unsigned int>::iterator it;
        it = db_endpoints.find(endpoint);
        if (it != db_endpoints.end()) {
            return it->second;
        } else {
            // if not found - create the new record in the database
            std::string sql = "INSERT INTO Endpoints (Interface, URL) VALUES ('" + sql_escape(endpoint.interface) + "', '" + sql_escape(endpoint.url) +"')";
            unsigned int newid = GeneralSQLInsert(sql);
            if ( newid ) {
                db_endpoints.insert(std::pair <aar_endpoint_t, unsigned int>(endpoint, newid));
                return newid;
            } else {
                logger.msg(Arc::ERROR, "Failed to add '%s' URL (interface type %s) into the accounting database Endpoints table", endpoint.url, endpoint.interface);
            }
        }
        return 0;
    }
    
    // callback to get id from database table
    static int ReadIdCallback(void* arg, int colnum, char** texts, char** names) {
        unsigned int* dbid = static_cast<unsigned int*>(arg);
        for (int n = 0; n < colnum; ++n) {
            if (names[n] && texts[n]) {
                int id;
                sql_unescape(texts[n], id);
                *dbid = id;
            }
        }
        return 0;
    }

    // AAR processing
    unsigned int AccountingDBSQLite::getAARDBId(const AAR& aar) {
        if (!isValid) return 0;
        initSQLiteDB();
        unsigned int dbid = 0;
        std::string sql = "SELECT RecordID FROM AAR WHERE JobID = '" + sql_escape(aar.jobid) + "'";
        if (db->exec(sql.c_str(), &ReadIdCallback, &dbid, NULL) != SQLITE_OK ) {
            logger.msg(Arc::ERROR, "Failed to query AAR database ID for job %s", aar.jobid);
            return 0;
        }
        return dbid;
    }

    unsigned int AccountingDBSQLite::getAARDBId(const std::string& jobid) {
        AAR aar;
        aar.jobid = jobid;
        return getAARDBId(aar);
    }

    bool AccountingDBSQLite::createAAR(AAR& aar) {
        if (!isValid) return false;
        initSQLiteDB();
        // get the corresponding IDs in connected tables
        unsigned int endpointid = getDBEndpointId(aar.endpoint);
        if (!endpointid) return false;
        unsigned int queueid = getDBQueueId(aar.queue);
        if (!queueid) return false;
        unsigned int userid = getDBUserId(aar.userdn);
        if (!userid) return false;
        unsigned int wlcgvoid = getDBWLCGVOId(aar.wlcgvo);
        if (!wlcgvoid) return false;
        unsigned int statusid = getDBStatusId(aar.status);
        if (!statusid) return false;
        // construct insert statement
        std::string sql = "INSERT INTO AAR ("
            "JobID, LocalJobID, EndpointID, QueueID, UserID, VOID, StatusID, ExitCode, "
            "SubmitTime, EndTime, NodeCount, CPUCount, UsedMemory, UsedVirtMem, UsedWalltime, "
            "UsedCPUUserTime, UsedCPUKernelTime, UsedScratch, StageInVolume, StageOutVolume ) "
            "VALUES ('" +
                sql_escape(aar.jobid) + "', '" +
                sql_escape(aar.localid) + "', " +
                sql_escape(endpointid) + ", " + 
                sql_escape(queueid) + ", " + 
                sql_escape(userid) + ", " + 
                sql_escape(wlcgvoid) + ", " + 
                sql_escape(statusid) + ", " +
                sql_escape(aar.exitcode) + ", " +
                sql_escape(aar.submittime.GetTime()) + ", " + 
                sql_escape(aar.endtime.GetTime()) + ", " +
                sql_escape(aar.nodecount) + ", " + 
                sql_escape(aar.cpucount) + ", " + 
                sql_escape(aar.usedmemory) + ", " + 
                sql_escape(aar.usedvirtmemory) + ", " +
                sql_escape(aar.usedwalltime) + ", " + 
                sql_escape(aar.usedcpuusertime) + ", " + 
                sql_escape(aar.usedcpukerneltime) + ", " +
                sql_escape(aar.usedscratch) + ", " + 
                sql_escape(aar.stageinvolume) + ", " + 
                sql_escape(aar.stageoutvolume) +
            ")";
        unsigned int recordid = GeneralSQLInsert(sql);
        if (!recordid) {
            logger.msg(Arc::ERROR, "Failed to insert AAR into the database for job %s", aar.jobid);
            logger.msg(Arc::DEBUG, "SQL statement used: %s", sql);
            return false;
        }
        // insert authtoken attributes
        if (!writeAuthTokenAttrs(aar.authtokenattrs, recordid)) {
            logger.msg(Arc::ERROR, "Failed to write authtoken attributes for job %s", aar.jobid);
        }
        // record ACCEPTED state
        if (!writeEvents(aar.jobevents, recordid)) {
            logger.msg(Arc::ERROR, "Failed to write event records for job %s", aar.jobid);
        }
        return true;
    }

    bool AccountingDBSQLite::updateAAR(AAR& aar) {
        if (!isValid) return false;
        initSQLiteDB();
        // get AAR ID in the database
        unsigned int recordid = getAARDBId(aar);
        if (!recordid) {
            logger.msg(Arc::ERROR, "Cannot to update AAR. Cannot find registered AAR for job %s in accounting database.", aar.jobid);
            return false;
        }
        // get the corresponding IDs in connected tables
        unsigned int statusid = getDBStatusId(aar.status);
        
        // construct update statement 
        // NOTE: it only make sense update the dynamic information not available on submission time
        std::string sql = "UPDATE AAR SET "
            "LocalJobID = '" + sql_escape(aar.localid) + "', " +
            "StatusID = " + sql_escape(statusid) + ", " +
            "ExitCode = " + sql_escape(aar.exitcode) + ", " +
            "EndTime = " + sql_escape(aar.endtime.GetTime()) + ", " +
            "NodeCount = " + sql_escape(aar.nodecount) + ", " + 
            "CPUCount = " + sql_escape(aar.cpucount) + ", " + 
            "UsedMemory = " + sql_escape(aar.usedmemory) + ", " + 
            "UsedVirtMem = " + sql_escape(aar.usedvirtmemory) + ", " +
            "UsedWalltime = " + sql_escape(aar.usedwalltime) + ", " + 
            "UsedCPUUserTime = " + sql_escape(aar.usedcpuusertime) + ", " + 
            "UsedCPUKernelTime = " + sql_escape(aar.usedcpukerneltime) + ", " +
            "UsedScratch = " + sql_escape(aar.usedscratch) + ", " + 
            "StageInVolume = " + sql_escape(aar.stageinvolume) + ", " + 
            "StageOutVolume = " + sql_escape(aar.stageoutvolume) + " " +
            "WHERE RecordId = " + sql_escape(recordid);
        // run update
        if (!GeneralSQLUpdate(sql)) {
            logger.msg(Arc::ERROR, "Failed to update AAR in the database for job %s", aar.jobid);
            logger.msg(Arc::DEBUG, "SQL statement used: %s", sql);
            return false;
        }
        // write RTE info
        if (!writeRTEs(aar.rtes, recordid)) {
            logger.msg(Arc::ERROR, "Failed to write RTEs information for the job %s", aar.jobid);
        }
        // write DTR info
        if (!writeDTRs(aar.transfers, recordid)) {
            logger.msg(Arc::ERROR, "Failed to write data transfers information for the job %s", aar.jobid);
        }
        // write extra information
        if (!writeExtraInfo(aar.extrainfo, recordid)) {
            logger.msg(Arc::ERROR, "Failed to write data transfers information for the job %s", aar.jobid);
        }
        // record FINISHED state
        if (!writeEvents(aar.jobevents, recordid)) {
            logger.msg(Arc::ERROR, "Failed to write event records for job %s", aar.jobid);
        }
        return true;
    }

    bool AccountingDBSQLite::writeRTEs(std::list <std::string>& rtes, unsigned int recordid) {
        if (rtes.empty()) return true;
        std::string sql = "BEGIN TRANSACTION; ";
        std::string sql_base = "INSERT INTO RunTimeEnvironments (RecordID, RTEName) VALUES ";
        for (std::list<std::string>::iterator it=rtes.begin(); it != rtes.end(); ++it) {
            sql += sql_base + "(" + sql_escape(recordid) + ", '" + sql_escape(*it) + "'); ";
        }
        sql += "COMMIT;";
        if(!GeneralSQLInsert(sql)) {
            logger.msg(Arc::DEBUG, "SQL statement used: %s", sql);
            return false;
        }
        return true;
    }

    bool AccountingDBSQLite::writeAuthTokenAttrs(std::list <aar_authtoken_t>& attrs, unsigned int recordid) {
        if (attrs.empty()) return true;
        std::string sql = "BEGIN TRANSACTION; ";
        std::string sql_base = "INSERT INTO AuthTokenAttributes (RecordID, AttrKey, AttrValue) VALUES ";
        for (std::list <aar_authtoken_t>::iterator it=attrs.begin(); it!=attrs.end(); ++it) {
            sql += sql_base + "(" + sql_escape(recordid) + ", '" + sql_escape(it->first) + "', '" +
                   sql_escape(it->second) + "'); ";
        }
        sql += "COMMIT;";
        if(!GeneralSQLInsert(sql)) {
            logger.msg(Arc::DEBUG, "SQL statement used: %s", sql);
            return false;
        }
        return true;
    }

    bool AccountingDBSQLite::writeExtraInfo(std::map <std::string, std::string>& info, unsigned int recordid) {
        if (info.empty()) return true;
        std::string sql = "BEGIN TRANSACTION; ";
        std::string sql_base = "INSERT INTO JobExtraInfo (RecordID, InfoKey, InfoValue) VALUES ";
        for (std::map<std::string,std::string>::iterator it=info.begin(); it!=info.end(); ++it) {
            sql += sql_base + "(" + sql_escape(recordid) + ", '" + sql_escape(it->first) + "', '" +
                   sql_escape(it->second) + "'); ";
        }
        sql += "COMMIT;";
        if(!GeneralSQLInsert(sql)) {
            logger.msg(Arc::DEBUG, "SQL statement used: %s", sql);
            return false;
        }
        return true;
    }

    bool AccountingDBSQLite::writeDTRs(std::list <aar_data_transfer_t>& dtrs, unsigned int recordid) {
        if (dtrs.empty()) return true;
        std::string sql = "BEGIN TRANSACTION; ";
        std::string sql_base = "INSERT INTO DataTransfers "
            "(RecordID, URL, FileSize, TransferStart, TransferEnd, TransferType) VALUES ";
        for (std::list<aar_data_transfer_t>::iterator it=dtrs.begin(); it != dtrs.end(); ++it) {
            sql += sql_base + "( " + sql_escape(recordid) + ", '" + 
                   sql_escape(it->url) + "', " +
                   sql_escape(it->size) + ", " + 
                   sql_escape(it->transferstart.GetTime()) + ", " + 
                   sql_escape(it->transferend.GetTime()) + ", " + 
                   sql_escape(static_cast<int>(it->type)) + "); ";
        }
        sql += "COMMIT;";
        if(!GeneralSQLInsert(sql)) {
            logger.msg(Arc::DEBUG, "SQL statement used: %s", sql);
            return false;
        }
        return true;
    }

    bool AccountingDBSQLite::writeEvents(std::list <aar_jobevent_t>& events, unsigned int recordid) {
        if (events.empty()) return true;
        std::string sql = "BEGIN TRANSACTION; ";
        std::string sql_base = "INSERT INTO JobEvents (RecordID, EventKey, EventTime) VALUES ";
        for (std::list<aar_jobevent_t>::iterator it=events.begin(); it != events.end(); ++it) {
            sql += sql_base + "( " + sql_escape(recordid) + ", '" + sql_escape(it->first) + "', '" +
                   sql_escape(it->second) + "'); ";
        }
        sql += "COMMIT;";
        if(!GeneralSQLInsert(sql)) {
            logger.msg(Arc::DEBUG, "SQL statement used: %s", sql);
            return false;
        }
        return true;
    }

    bool AccountingDBSQLite::addJobEvent(aar_jobevent_t& event, const std::string& jobid) {
        unsigned int recordid = getAARDBId(jobid);
        if (!recordid) {
            logger.msg(Arc::ERROR, "Unable to add event: cannot find AAR for job %s in accounting database.", jobid);
            return false;
        }
        std::string sql = "INSERT INTO JobEvents (RecordID, EventKey, EventTime) VALUES (" +
            sql_escape(recordid) + ", '" + sql_escape(event.first) + "', '" + sql_escape(event.second) + "')";
        if(!GeneralSQLInsert(sql)) {
            logger.msg(Arc::DEBUG, "SQL statement used: %s", sql);
            return false;
        }
        return true;
    }
}
