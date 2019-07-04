#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/FileUtils.h>
#include <arc/StringConv.h>
#include <arc/DateTime.h>

#include "AccountingDBSQLite.h"

#define DB_SCHEMA_FILE "arex_accounting_db_schema_v1.sql"

namespace ARex {
    Arc::Logger AccountingDBSQLite::logger(Arc::Logger::getRootLogger(), "AccountingDBSQLite");

    static const std::string sql_special_chars("'#\r\n\b\0",6);
    static const char sql_escape_char('%');
    static const Arc::escape_type sql_escape_type(Arc::escape_hex);

    inline static std::string sql_escape(const std::string& str) {
        return Arc::escape_chars(str, sql_special_chars, sql_escape_char, false, sql_escape_type);
    }

    inline static std::string sql_escape(const Arc::Time& val) {
        if(val.GetTime() == -1) return "";
        return Arc::escape_chars((std::string)val, sql_special_chars, sql_escape_char, false, sql_escape_type);
    }

    inline static std::string sql_unescape(const std::string& str) {
        return Arc::unescape_chars(str, sql_escape_char,sql_escape_type);
    }

    inline static void sql_unescape(const std::string& str, int& val) {
        (void)Arc::stringto(Arc::unescape_chars(str, sql_escape_char,sql_escape_type), val);
    }

    inline static void sql_unescape(const std::string& str, Arc::Time& val) {
        if(str.empty()) { val = Arc::Time(); return; }
        val = Arc::Time(Arc::unescape_chars(str, sql_escape_char,sql_escape_type));
    }

    int sqlite3_exec_nobusy(sqlite3* db, const char *sql, int (*callback)(void*,int,char**,char**), void *arg, char **errmsg) {
        int err;
        while((err = sqlite3_exec(db, sql, callback, arg, errmsg)) == SQLITE_BUSY) {
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
            std::string sql_file = DB_SCHEMA_FILE;
            if(!Arc::FileRead(sql_file, db_schema_str)) {
                AccountingDBSQLite::logger.msg(Arc::ERROR, "Failed to read database schema file at %s", sql_file);
                closeDB();
                return;
            }
            err = sqlite3_exec_nobusy(aDB, db_schema_str.c_str(), NULL, NULL, NULL);
            if(err != SQLITE_OK) {
                logError("Failed to initialize accounting database schema", err, Arc::ERROR);
                closeDB();
                return;
            }
            AccountingDBSQLite::logger.msg(Arc::INFO, "Accounting database initialized succesfully");
        }
        AccountingDBSQLite::logger.msg(Arc::DEBUG, "Accounting database connection has been establised");
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

    void AccountingDBSQLite::SQLiteDB::closeDB(void) {
        if (aDB) {
            (void)sqlite3_close(aDB); // TODO: handle errors?
            aDB = NULL;
        };
    }

    AccountingDBSQLite::SQLiteDB::~SQLiteDB() {
        closeDB();
    }

    AccountingDBSQLite::AccountingDBSQLite(const std::string& name) : AccountingDB(name) {
        isValid = false;
        // chech database file exists
        if (!Glib::file_test(name, Glib::FILE_TEST_EXISTS)) {
            const std::string dbdir = Glib::path_get_dirname(name);
            // Check if the parent directory exist
            if (!Glib::file_test(dbdir, Glib::FILE_TEST_EXISTS)) {
                logger.msg(Arc::ERROR, "Accounting database cannot be created. Parent directory %s does not exist.", dbdir);
                return;
            } else if (!Glib::file_test(dbdir, Glib::FILE_TEST_IS_DIR)) {
                logger.msg(Arc::ERROR, "Accounting database cannot be created: %s is not a directory", dbdir);
                return;
            }
            // initialize new database
            SQLiteDB adb(name, true); // automatically destroyed out of scope
            if (!adb.handle()) {
                logger.msg(Arc::ERROR, "Failed to initialize accounting database");
                return;
            }
            isValid = true;
            return;
        } else if (!Glib::file_test(name, Glib::FILE_TEST_IS_REGULAR)) {
            logger.msg(Arc::ERROR, "Accounting database file (%s) is not a regular file", name);
            return;
        }
        // if we are here database location is fine, trying to open
        SQLiteDB adb(name); // automatically destroyed out of scope
        if (!adb.handle()) {
            logger.msg(Arc::ERROR, "Error opening accounting database");
            return;
        }
        // TODO: implement schema version checking and possible updates
        isValid = true;
    }

    // callback to build (name,id) map from database table
    static int ReadIdNameCallback(void* arg, int colnum, char** texts, char** names) {
        name_id_map_t* name_id_map = static_cast<name_id_map_t*>(arg);
        std::pair <std::string, unsigned int> rec;
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
        if(!rec.first.empty()) name_id_map->insert(rec);
        return 0;
    }

    // general helper to build (name,id) map from database table
    unsigned int AccountingDBSQLite::QueryAndInsertNameID(const std::string& table, const std::string& iname, name_id_map_t* name_id_map) {
        if (!isValid) return 0;

        if (name_id_map->empty()) {
            // empty map - nothing had been fetched from database yet
            SQLiteDB adb(name);
            std::string sqlcmd = "SELECT * FROM " + sql_escape(table);
            if (sqlite3_exec_nobusy(adb.handle(), sqlcmd.c_str(), &ReadIdNameCallback, name_id_map, NULL) != SQLITE_OK ) {
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
            SQLiteDB adb(name);
            std::string sqlcmd = "INSERT INTO " + sql_escape(table) + " (Name) VALUES ('" + sql_escape(iname) + "')";
            int err;
            err = sqlite3_exec_nobusy(adb.handle(), sqlcmd.c_str(), NULL, NULL, NULL);
            if (err != SQLITE_OK ) {
                adb.logError("Failed to insert data into database", err, Arc::ERROR);
                return 0;
            }
            if(sqlite3_changes(adb.handle()) != 1) {
                logger.msg(Arc::ERROR, "Failed to add '%s' into the accounting database %s table", iname, table);
                return 0;
            }
            sqlite3_int64 newid = sqlite3_last_insert_rowid(adb.handle());
            if (newid) {
                name_id_map->insert(std::pair <std::string, unsigned int>(iname, (unsigned int) newid));
                return (unsigned int) newid;
            }
        }
        return 0;
    }

    // Queue table
    unsigned int AccountingDBSQLite::getDBQueueId(const std::string& queue) {
        return QueryAndInsertNameID("Queues", queue, &db_queue);
    }
}
