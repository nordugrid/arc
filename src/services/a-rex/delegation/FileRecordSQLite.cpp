#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>
#include <cstring>

#include <glibmm.h>

#include <arc/FileUtils.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>

#include "uid.h"

#include "../SQLhelpers.h"

#include "FileRecordSQLite.h"

namespace ARex {

  #define FR_DB_NAME "list"

  bool FileRecordSQLite::dberr(const char* s, int err) {
    if(err == SQLITE_OK) return true;
    error_num_ = err;
#ifdef HAVE_SQLITE3_ERRSTR
    error_str_ = std::string(s)+": "+sqlite3_errstr(err);
#else
    error_str_ = std::string(s)+": error code "+Arc::tostring(err);
#endif
    return false;
  }

  FileRecordSQLite::FileRecordSQLite(const std::string& base, bool create):
      FileRecord(base, create),
      db_(NULL) {
    valid_ = open(create);
  }

  bool FileRecordSQLite::verify(void) {
    // Not implemented and probably not needed
    return true;
  }

  FileRecordSQLite::~FileRecordSQLite(void) {
    close();
  }

  int FileRecordSQLite::sqlite3_exec_nobusy(const char *sql, int (*callback)(void*,int,char**,char**), 
    void *arg, char **errmsg) {
      int err;
      while((err = sqlite3_exec(db_, sql, callback, arg, errmsg)) == SQLITE_BUSY) {
        // Access to database is designed in such way that it should not block for long time.
        // So it should be safe to simply wait for lock to be released without any timeout.
        struct timespec delay = { 0, 10000000 }; // 0.01s - should be enough for most cases
        (void)::nanosleep(&delay, NULL);
      };
      return err;
  }

  bool FileRecordSQLite::open(bool create) {
    std::string dbpath = basepath_ + G_DIR_SEPARATOR_S + FR_DB_NAME;
    if(db_ != NULL) return true; // already open

    int flags = SQLITE_OPEN_READWRITE; // it will open read-only if access is protected
    if(create) {
      flags |= SQLITE_OPEN_CREATE;
    };
    int err;
    while((err = sqlite3_open_v2(dbpath.c_str(), &db_, flags, NULL)) == SQLITE_BUSY) {
      // In case something prevents databasre from open right now - retry
      if(db_) (void)sqlite3_close(db_);
      db_ = NULL;
      struct timespec delay = { 0, 10000000 }; // 0.01s - should be enough for most cases
      (void)::nanosleep(&delay, NULL);
    };
    if(!dberr("Error opening database", err)) {
      if(db_) (void)sqlite3_close(db_);
      db_ = NULL;
      return false;
    };
    if(create) {
      if(!dberr("Error creating table rec", sqlite3_exec_nobusy("CREATE TABLE IF NOT EXISTS rec(id, owner, uid, meta, UNIQUE(id, owner), UNIQUE(uid))", NULL, NULL, NULL))) {
        (void)sqlite3_close(db_); // todo: handle error
        db_ = NULL;
        return false;
      };
      if(!dberr("Error creating table lock", sqlite3_exec_nobusy("CREATE TABLE IF NOT EXISTS lock(lockid, uid)", NULL, NULL, NULL))) {
        (void)sqlite3_close(db_); // todo: handle error
        db_ = NULL;
        return false;
      };
      if(!dberr("Error creating index lockid", sqlite3_exec_nobusy("CREATE INDEX IF NOT EXISTS lockid ON lock (lockid)", NULL, NULL, NULL))) {
        (void)sqlite3_close(db_); // todo: handle error
        db_ = NULL;
        return false;
      };
      if(!dberr("Error creating index uid", sqlite3_exec_nobusy("CREATE INDEX IF NOT EXISTS uid ON lock (uid)", NULL, NULL, NULL))) {
        (void)sqlite3_close(db_); // todo: handle error
        db_ = NULL;
        return false;
      };
    } else {
      // SQLite opens database in lazy way. But we still want to know if it is good database.
      if(!dberr("Error checking database", sqlite3_exec_nobusy("PRAGMA schema_version;", NULL, NULL, NULL))) {
        (void)sqlite3_close(db_); // todo: handle error
        db_ = NULL;
        return false;
      };
    };
    return true;
  }

  void FileRecordSQLite::close(void) {
    valid_ = false;
    if(db_) {
      (void)sqlite3_close(db_); // todo: handle error
      db_ = NULL;
    };
  }

  void store_strings(const std::list<std::string>& strs, std::string& buf) {
    if(!strs.empty()) {
      for(std::list<std::string>::const_iterator str = strs.begin(); ; ++str) {
        buf += sql_escape(*str);
        if (str == strs.end()) break;
        buf += '#';
      };
    };
  }

  static void parse_strings(std::list<std::string>& strs, const char* buf) {
    if(!buf || (*buf == '\0')) return;
    const char* sep = std::strchr(buf, '#');
    while(sep) {
      strs.push_back(sql_unescape(std::string(buf,sep-buf)));
      buf = sep+1;
      sep = std::strchr(buf, '#');
    };
  }

  bool FileRecordSQLite::Recover(void) {
    Glib::Mutex::Lock lock(lock_);
    // Real recovery not implemented yet.
    close();
    error_num_ = -1;
    error_str_ = "Recovery not implemented yet.";
    return false;
  }

  struct FindCallbackRecArg {
    sqlite3_int64 rowid;
    std::string id;
    std::string owner;
    std::string uid;
    std::list<std::string> meta;
    FindCallbackRecArg(): rowid(-1) {};
  };

  static int FindCallbackRec(void* arg, int colnum, char** texts, char** names) {
    for(int n = 0; n < colnum; ++n) {
      if(names[n] && texts[n]) {
        if((strcmp(names[n], "rowid") == 0) || (strcmp(names[n], "_rowid_") == 0)) {
          (void)Arc::stringto(texts[n], ((FindCallbackRecArg*)arg)->rowid);
        } else if(strcmp(names[n], "uid") == 0) {
          ((FindCallbackRecArg*)arg)->uid = texts[n];
        } else if(strcmp(names[n], "id") == 0) {
          ((FindCallbackRecArg*)arg)->id = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "owner") == 0) {
          ((FindCallbackRecArg*)arg)->owner = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "meta") == 0) {
          parse_strings(((FindCallbackRecArg*)arg)->meta, texts[n]);
        };
      };
    };
    return 0;
  }

  struct FindCallbackUidMetaArg {
    std::string& uid;
    std::list<std::string>& meta;
    FindCallbackUidMetaArg(std::string& uid, std::list<std::string>& meta): uid(uid), meta(meta) {};
  };

  static int FindCallbackUidMeta(void* arg, int colnum, char** texts, char** names) {
    for(int n = 0; n < colnum; ++n) {
      if(names[n] && texts[n]) {
        if(strcmp(names[n], "uid") == 0) {
          ((FindCallbackUidMetaArg*)arg)->uid = texts[n];
        } else if(strcmp(names[n], "meta") == 0) {
          parse_strings(((FindCallbackUidMetaArg*)arg)->meta, texts[n]);
        };
      };
    };
    return 0;
  }

  struct FindCallbackUidArg {
    std::string& uid;
    FindCallbackUidArg(std::string& uid): uid(uid) {};
  };

  static int FindCallbackUid(void* arg, int colnum, char** texts, char** names) {
    for(int n = 0; n < colnum; ++n) {
      if(names[n] && texts[n]) {
        if(strcmp(names[n], "uid") == 0) {
          ((FindCallbackUidMetaArg*)arg)->uid = texts[n];
        };
      };
    };
    return 0;
  }

  struct FindCallbackCountArg {
    int count;
    FindCallbackCountArg():count(0) {};
  };

  static int FindCallbackCount(void* arg, int colnum, char** texts, char** names) {
    ((FindCallbackCountArg*)arg)->count += 1;
    return 0;
  }

  struct FindCallbackIdOwnerArg {
    std::list< std::pair<std::string,std::string> >& records;
    FindCallbackIdOwnerArg(std::list< std::pair<std::string,std::string> >& recs): records(recs) {};
  };

  static int FindCallbackIdOwner(void* arg, int colnum, char** texts, char** names) {
    std::pair<std::string,std::string> rec;
    for(int n = 0; n < colnum; ++n) {
      if(names[n] && texts[n]) {
        if(strcmp(names[n], "id") == 0) {
          rec.first = sql_unescape(texts[n]);
        } else if(strcmp(names[n], "owner") == 0) {
          rec.second = sql_unescape(texts[n]);
        };
      };
    };
    if(!rec.first.empty()) ((FindCallbackIdOwnerArg*)arg)->records.push_back(rec);
    return 0;
  }

  struct FindCallbackLockArg {
    std::list< std::string >& records;
    FindCallbackLockArg(std::list< std::string >& recs): records(recs) {};
  };

  static int FindCallbackLock(void* arg, int colnum, char** texts, char** names) {
    for(int n = 0; n < colnum; ++n) {
      if(names[n] && texts[n]) {
        if(strcmp(names[n], "lockid") == 0) {
          std::string rec = sql_unescape(texts[n]);
          if(!rec.empty()) ((FindCallbackLockArg*)arg)->records.push_back(rec);
        };
      };
    };
    return 0;
  }


  std::string FileRecordSQLite::Add(std::string& id, const std::string& owner, const std::list<std::string>& meta) {
    if(!valid_) return "";
    int uidtries = 10; // some sane number
    std::string uid;
    while(true) {
      if(!(uidtries--)) {
        error_str_ = "Out of tries adding record to database";
        return "";
      };
      Glib::Mutex::Lock lock(lock_);
      uid = rand_uid64().substr(4);
      std::string metas;
      store_strings(meta, metas);
      std::string sqlcmd = "INSERT INTO rec(id, owner, uid, meta) VALUES ('"+
               sql_escape(id.empty()?uid:id)+"', '"+
               sql_escape(owner)+"', '"+uid+"', '"+metas+"')";
      int dbres = sqlite3_exec_nobusy(sqlcmd.c_str(), NULL, NULL, NULL);
      if(dbres == SQLITE_CONSTRAINT) {
        // retry due to non-unique id
        uid.resize(0);
        continue;
      };
      if(!dberr("Failed to add record to database", dbres)) {
        return "";
      };
      if(sqlite3_changes(db_) != 1) {
        error_str_ = "Failed to add record to database";
        return "";
      };
      break;
    };
    if(id.empty()) id = uid;
    make_file(uid);
    return uid_to_path(uid);
  }

  bool FileRecordSQLite::Add(const std::string& uid, const std::string& id, const std::string& owner, const std::list<std::string>& meta) {
    if(!valid_) return false;
    Glib::Mutex::Lock lock(lock_);
    std::string metas;
    store_strings(meta, metas);
    std::string sqlcmd = "INSERT INTO rec(id, owner, uid, meta) VALUES ('"+
             sql_escape(id.empty()?uid:id)+"', '"+
             sql_escape(owner)+"', '"+uid+"', '"+metas+"')";
    int dbres = sqlite3_exec_nobusy(sqlcmd.c_str(), NULL, NULL, NULL);
    if(!dberr("Failed to add record to database", dbres)) {
      return false;
    };
    if(sqlite3_changes(db_) != 1) {
      error_str_ = "Failed to add record to database";
      return false;
    };
    return true;
  }

  std::string FileRecordSQLite::Find(const std::string& id, const std::string& owner, std::list<std::string>& meta) {
    if(!valid_) return "";
    Glib::Mutex::Lock lock(lock_);
    std::string sqlcmd = "SELECT uid, meta FROM rec WHERE ((id = '"+sql_escape(id)+"') AND (owner = '"+sql_escape(owner)+"'))";
    std::string uid;
    FindCallbackUidMetaArg arg(uid, meta);
    if(!dberr("Failed to retrieve record from database",sqlite3_exec_nobusy(sqlcmd.c_str(), &FindCallbackUidMeta, &arg, NULL))) {
      return "";
    };
    if(uid.empty()) {
      error_str_ = "Failed to retrieve record from database";
      return "";
    };
    return uid_to_path(uid);
  }

  bool FileRecordSQLite::Modify(const std::string& id, const std::string& owner, const std::list<std::string>& meta) {
    if(!valid_) return false;
    Glib::Mutex::Lock lock(lock_);
    std::string metas;
    store_strings(meta, metas);
    std::string sqlcmd = "UPDATE rec SET meta = '"+metas+"' WHERE ((id = '"+sql_escape(id)+"') AND (owner = '"+sql_escape(owner)+"'))";
    if(!dberr("Failed to update record in database",sqlite3_exec_nobusy(sqlcmd.c_str(), NULL, NULL, NULL))) {
      return false;
    };
    if(sqlite3_changes(db_) < 1) {
      error_str_ = "Failed to find record in database";
      return false;
    };
    return true;
  }

  bool FileRecordSQLite::Remove(const std::string& id, const std::string& owner) {
    if(!valid_) return false;
    Glib::Mutex::Lock lock(lock_);
    std::string uid;
    {
      std::string sqlcmd = "SELECT uid FROM rec WHERE ((id = '"+sql_escape(id)+"') AND (owner = '"+sql_escape(owner)+"'))";
      FindCallbackUidArg arg(uid);
      if(!dberr("Failed to retrieve record from database",sqlite3_exec_nobusy(sqlcmd.c_str(), &FindCallbackUid, &arg, NULL))) {
        return false; // No such record?
      };
    };
    if(uid.empty()) {
      error_str_ = "Record not found";
      return false; // No such record
    };
    {
      std::string sqlcmd = "SELECT uid FROM lock WHERE (uid = '"+uid+"')";
      FindCallbackCountArg arg;
      if(!dberr("Failed to find locks in database",sqlite3_exec_nobusy(sqlcmd.c_str(), &FindCallbackCount, &arg, NULL))) {
        return false;
      };
      if(arg.count > 0) {
        error_str_ = "Record has active locks";
        return false; // have locks
      };
    };
    {
      std::string sqlcmd = "DELETE FROM rec WHERE (uid = '"+uid+"')";
      if(!dberr("Failed to delete record in database",sqlite3_exec_nobusy(sqlcmd.c_str(), NULL, NULL, NULL))) {
        return false;
      };
      if(sqlite3_changes(db_) < 1) {
        error_str_ = "Failed to delete record in database";
        return false; // no such record
      };
    };
    remove_file(uid);
    return true;
  }

  bool FileRecordSQLite::AddLock(const std::string& lock_id, const std::list<std::string>& ids, const std::string& owner) {
    if(!valid_) return false;
    Glib::Mutex::Lock lock(lock_);
    for(std::list<std::string>::const_iterator id = ids.begin(); id != ids.end(); ++id) {
      std::string uid;
      {
        std::string sqlcmd = "SELECT uid FROM rec WHERE ((id = '"+sql_escape(*id)+"') AND (owner = '"+sql_escape(owner)+"'))";
        FindCallbackUidArg arg(uid);
        if(!dberr("Failed to retrieve record from database",sqlite3_exec_nobusy(sqlcmd.c_str(), &FindCallbackUid, &arg, NULL))) {
          return false; // No such record?
        };
      };
      if(uid.empty()) {
        // No such record
        continue;
      };
      std::string sqlcmd = "INSERT INTO lock(lockid, uid) VALUES ('"+sql_escape(lock_id)+"','"+uid+"')";
      if(!dberr("addlock:put",sqlite3_exec_nobusy(sqlcmd.c_str(), NULL, NULL, NULL))) {
        return false;
      };
    };
    return true;
  }

  bool FileRecordSQLite::RemoveLock(const std::string& lock_id) {
    if(!valid_) return false;
    Glib::Mutex::Lock lock(lock_);
    // map lock to id,owner 
    {
      std::string sqlcmd = "DELETE FROM lock WHERE (lockid = '"+sql_escape(lock_id)+"')";
      if(!dberr("removelock:del",sqlite3_exec_nobusy(sqlcmd.c_str(), NULL, NULL, NULL))) {
        return false;
      };
      if(sqlite3_changes(db_) < 1) {
        error_str_ = "";
        return false;
      };
    };
    return true;
  }

  bool FileRecordSQLite::RemoveLock(const std::string& lock_id, std::list<std::pair<std::string,std::string> >& ids) {
    if(!valid_) return false;
    Glib::Mutex::Lock lock(lock_);
    // map lock to id,owner 
    {
      std::string sqlcmd = "SELECT id,owner FROM rec WHERE uid IN SELECT uid FROM lock WHERE (lockid = '"+sql_escape(lock_id)+"')";
      FindCallbackIdOwnerArg arg(ids);
      if(!dberr("removelock:get",sqlite3_exec_nobusy(sqlcmd.c_str(), &FindCallbackIdOwner, &arg, NULL))) {
        //return false;
      };
    };
    {
      std::string sqlcmd = "DELETE FROM lock WHERE (lockid = '"+sql_escape(lock_id)+"')";
      if(!dberr("removelock:del",sqlite3_exec_nobusy(sqlcmd.c_str(), NULL, NULL, NULL))) {
        return false;
      };
      if(sqlite3_changes(db_) < 1) {
        error_str_ = "";
        return false;
      };
    };
    return true;
  }


  bool FileRecordSQLite::ListLocked(const std::string& lock_id, std::list<std::pair<std::string,std::string> >& ids) {
    if(!valid_) return false;
    Glib::Mutex::Lock lock(lock_);
    // map lock to id,owner 
    {
      std::string sqlcmd = "SELECT id,owner FROM rec WHERE uid IN SELECT uid FROM lock WHERE (lockid = '"+sql_escape(lock_id)+"')";
      FindCallbackIdOwnerArg arg(ids);
      if(!dberr("listlocked:get",sqlite3_exec_nobusy(sqlcmd.c_str(), &FindCallbackIdOwner, &arg, NULL))) {
        return false;
      };
    };
    //if(ids.empty()) return false;
    return true;
  }

  bool FileRecordSQLite::ListLocks(std::list<std::string>& locks) {
    if(!valid_) return false;
    Glib::Mutex::Lock lock(lock_);
    {
      std::string sqlcmd = "SELECT lockid FROM lock";
      FindCallbackLockArg arg(locks);
      if(!dberr("listlocks:get",sqlite3_exec_nobusy(sqlcmd.c_str(), &FindCallbackLock, &arg, NULL))) {
        return false;
      };
    };
    return true;
  }

  bool FileRecordSQLite::ListLocks(const std::string& id, const std::string& owner, std::list<std::string>& locks) {
    if(!valid_) return false;
    Glib::Mutex::Lock lock(lock_);
    std::string uid;
    {
      std::string sqlcmd = "SELECT uid FROM rec WHERE ((id = '"+sql_escape(id)+"') AND (owner = '"+sql_escape(owner)+"'))";
      FindCallbackUidArg arg(uid);
      if(!dberr("Failed to retrieve record from database",sqlite3_exec_nobusy(sqlcmd.c_str(), &FindCallbackUid, &arg, NULL))) {
        return false; // No such record?
      };
    };
    if(uid.empty()) {
      error_str_ = "Record not found";
      return false; // No such record
    };
    {
      std::string sqlcmd = "SELECT lockid FROM lock WHERE (uid = '"+uid+"')";
      FindCallbackLockArg arg(locks);
      if(!dberr("listlocks:get",sqlite3_exec_nobusy(sqlcmd.c_str(), &FindCallbackLock, &arg, NULL))) {
        return false;
      };
    };
    return true;
  }

  FileRecordSQLite::Iterator::Iterator(FileRecordSQLite& frec):FileRecord::Iterator(frec) {
    rowid_ = -1;
    Glib::Mutex::Lock lock(frec.lock_);
    {
      std::string sqlcmd = "SELECT _rowid_,id,owner,uid,meta FROM rec ORDER BY _rowid_ LIMIT 1";
      FindCallbackRecArg arg;
      if(!frec.dberr("listlocks:get",frec.sqlite3_exec_nobusy(sqlcmd.c_str(), &FindCallbackRec, &arg, NULL))) {
        return;
      };
      if(arg.uid.empty()) {
        return;
      };
      uid_ = arg.uid;
      id_ = arg.id;
      owner_ = arg.owner;
      meta_ = arg.meta;
      rowid_ = arg.rowid;
    };
  }

  FileRecordSQLite::Iterator::~Iterator(void) {
  }

  FileRecordSQLite::Iterator& FileRecordSQLite::Iterator::operator++(void) {
    if(rowid_ == -1) return *this;
    FileRecordSQLite& frec((FileRecordSQLite&)frec_);
    Glib::Mutex::Lock lock(frec.lock_);
    {
      std::string sqlcmd = "SELECT _rowid_,id,owner,uid,meta FROM rec WHERE (_rowid_ > " + Arc::tostring(rowid_) + ") ORDER BY _rowid_ ASC LIMIT 1";
      FindCallbackRecArg arg;
      if(!frec.dberr("listlocks:get",frec.sqlite3_exec_nobusy(sqlcmd.c_str(), &FindCallbackRec, &arg, NULL))) {
        rowid_ = -1;
        return *this;
      };
      if(arg.uid.empty()) {
        rowid_ = -1;
        return *this;
      };
      uid_ = arg.uid;
      id_ = arg.id;
      owner_ = arg.owner;
      meta_ = arg.meta;
      rowid_ = arg.rowid;
    };
    return *this;
  }

  FileRecordSQLite::Iterator& FileRecordSQLite::Iterator::operator--(void) {
    if(rowid_ == -1) return *this;
    FileRecordSQLite& frec((FileRecordSQLite&)frec_);
    Glib::Mutex::Lock lock(frec.lock_);
    {
      std::string sqlcmd = "SELECT _rowid_,id,owner,uid,meta FROM rec WHERE (_rowid_ < " + Arc::tostring(rowid_) + ") ORDER BY _rowid_ DESC LIMIT 1";
      FindCallbackRecArg arg;
      if(!frec.dberr("listlocks:get",frec.sqlite3_exec_nobusy(sqlcmd.c_str(), &FindCallbackRec, &arg, NULL))) {
        rowid_ = -1;
        return *this;
      };
      if(arg.uid.empty()) {
        rowid_ = -1;
        return *this;
      };
      uid_ = arg.uid;
      id_ = arg.id;
      owner_ = arg.owner;
      meta_ = arg.meta;
      rowid_ = arg.rowid;
    };
    return *this;
  }

  void FileRecordSQLite::Iterator::suspend(void) {
  }

  bool FileRecordSQLite::Iterator::resume(void) {
    return true;
  }

} // namespace ARex

