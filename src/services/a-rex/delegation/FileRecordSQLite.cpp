#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include <glibmm.h>

#include <arc/FileUtils.h>
#include <arc/Logger.h>

#include "uid.h"

#include "FileRecordSQLite.h"

namespace ARex {

  #define FR_DB_NAME "list"

  bool FileRecordSQLite::dberr(const char* s, int err) {
    if(err == SQLITE_OK) return true;
    error_num_ = err;
    error_str_ = std::string(s)+": "+sqlite3_errstr(err);
    return false;
  }

  FileRecordSQLite::FileRecordSQLite(const std::string& base, bool create):
      FileRecord(base, create),
      db_(NULL) {
    valid_ = open(create);
  }

  bool FileRecordSQLite::verify(void) {
    // Performing various kinds of verifications
/*
    std::string dbpath = basepath_ + G_DIR_SEPARATOR_S + FR_DB_NAME;
    {
      Db db_test(NULL,DB_CXX_NO_EXCEPTIONS);
      if(!dberr("Error verifying databases",
                db_test.verify(dbpath.c_str(),NULL,NULL,DB_NOORDERCHK))) {
        if(error_num_ != ENOENT) return false;
      };
    };
    {
      Db db_test(NULL,DB_CXX_NO_EXCEPTIONS);
      if(!dberr("Error verifying database 'meta'",
                db_test.verify(dbpath.c_str(),"meta",NULL,DB_ORDERCHKONLY))) {
        if(error_num_ != ENOENT) return false;
      };
    };
    // Skip 'link' - it is not of btree kind
    // Skip 'lock' - for unknown reason it returns DB_NOTFOUND
    // Skip 'locked' - for unknown reason it returns DB_NOTFOUND
*/
    return true;
  }

  FileRecordSQLite::~FileRecordSQLite(void) {
    close();
  }

  bool FileRecordSQLite::open(bool create) {
    std::string dbpath = FR_DB_NAME;
    if(db_ != NULL) return true;

    if(!dberr("Error opening database", sqlite3_open(dbpath.c_str(), &db_))) {
      db_ = NULL;
      return false;
    };
    if(!dberr("Error creating table", sqlite3_exec(db_, "CREATE TABLE IF NOT EXISTS rec(id, owner, uid, meta, UNIQUE(id, owner), UNIQUE(uid))", NULL, NULL, NULL))) {
      (void)sqlite3_close(db_); // todo: handle error
      db_ = NULL;
      return false;
    };
    if(!dberr("Error creating table", sqlite3_exec(db_, "CREATE TABLE IF NOT EXISTS lock(lockid, uid)", NULL, NULL, NULL))) {
      (void)sqlite3_close(db_); // todo: handle error
      db_ = NULL;
      return false;
    };
    if(!dberr("Error creating table", sqlite3_exec(db_, "CREATE INDEX IF NOT EXISTS ON lock (lockid)", NULL, NULL, NULL))) {
      (void)sqlite3_close(db_); // todo: handle error
      db_ = NULL;
      return false;
    };
    if(!dberr("Error creating table", sqlite3_exec(db_, "CREATE INDEX IF NOT EXISTS ON lock (uid)", NULL, NULL, NULL))) {
      (void)sqlite3_close(db_); // todo: handle error
      db_ = NULL;
      return false;
    };
  }

  void FileRecordSQLite::close(void) {
    valid_ = false;
    if(db_) {
      (void)sqlite3_close(db_); // todo: handle error
      db_ = NULL;
    };
  }

  void store_strings(const std::list<std::string>& strs, std::string& buf) {
    for(std::list<std::string>::const_iterator str = strs.begin(); str != strs.end(); ++str) {
      uint32_t l = str->length();
      buf.append(1, (unsigned char)l); l >>= 8;
      buf.append(1, (unsigned char)l); l >>= 8;
      buf.append(1, (unsigned char)l); l >>= 8;
      buf.append(1, (unsigned char)l); l >>= 8;
      buf.append(*str);
    };
  }

  static void parse_strings(std::list<std::string>& strs, const char* buf) {
    /*
    uint32_t l = 0;
    const unsigned char* p = (unsigned char*)buf;
    if(size < 4) { p += size; size = 0; return (void*)p; };
    l |= ((uint32_t)(*p)) << 0; ++p; --size;
    l |= ((uint32_t)(*p)) << 8; ++p; --size;
    l |= ((uint32_t)(*p)) << 16; ++p; --size;
    l |= ((uint32_t)(*p)) << 24; ++p; --size;
    if(l > size) l = size;
    // TODO: sanity check
    str.assign((const char*)p,l);
    p += l; size -= l;
    return (void*)p;
    */
  }

  bool FileRecordSQLite::Recover(void) {
    Glib::Mutex::Lock lock(lock_);
    // Real recovery not implemented yet.
    close();
    error_num_ = -1;
    error_str_ = "Recovery not implemented yet.";
    return false;
  }

  std::string FileRecordSQLite::Add(std::string& id, const std::string& owner, const std::list<std::string>& meta) {
    if(!valid_) return "";
    Glib::Mutex::Lock lock(lock_);
    // todo: retries for unique uid?
    std::string uid = rand_uid64().substr(4);
    std::string metas;
    store_strings(meta, metas);
    if(id.empty()) id = uid;
    // todo: protection encoding
    std::string sqlcmd = "INSERT INTO rec(id, owner, uid, meta) "
                         "VALUES ("+id+","+owner+","+uid+", "+metas+")";
    if(!dberr("Failed to add record to database", sqlite3_exec(db_, sqlcmd.c_str(), NULL, NULL, NULL))) {
      return "";
    };
    return uid_to_path(uid);
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
  }

  struct FindCallbackCountArg {
    int count;
    FindCallbackCountArg():count(0) {};
  };

  static int FindCallbackCount(void* arg, int colnum, char** texts, char** names) {
    ((FindCallbackCountArg*)arg)->count += 1;
  }

  struct FindCallbackRowidLockidArg {
    struct record {
      sqlite3_int64 rowid;
      std::string uid;
      record(): rowid(-1) {};
    };
    std::list<record> records;
    FindCallbackRowidLockidArg() {};
  };

  static int FindCallbackRowidLockid(void* arg, int colnum, char** texts, char** names) {
    for(int n = 0; n < colnum; ++n) {
      if(names[n] && texts[n]) {
        if(strcmp(names[n], "rowid") == 0) {
        } else if(strcmp(names[n], "uid") == 0) {
        };
      };
    };
  }

  std::string FileRecordSQLite::Find(const std::string& id, const std::string& owner, std::list<std::string>& meta) {
    if(!valid_) return "";
    Glib::Mutex::Lock lock(lock_);
    // todo: protection encoding
    std::string sqlcmd = "SELECT uid, meta FROM rec WHERE ((id = "+id+") AND (owner = "+owner+"))";
    std::string uid;
    FindCallbackUidMetaArg arg(uid, meta);
    if(!dberr("Failed to retrieve record from database",sqlite3_exec(db_, sqlcmd.c_str(), &FindCallbackUidMeta, &arg, NULL))) {
      return "";
    };
    return uid_to_path(uid);
  }

  bool FileRecordSQLite::Modify(const std::string& id, const std::string& owner, const std::list<std::string>& meta) {
    if(!valid_) return false;
    Glib::Mutex::Lock lock(lock_);
    std::string metas;
    store_strings(meta, metas);
    FindCallbackCountArg arg;
    // todo: protection encoding
    std::string sqlcmd = "UPDATE rec SET meta = "+metas+" WHERE ((id = "+id+") AND (owner = "+owner+"))";
    if(!dberr("Failed to update record in database",sqlite3_exec(db_, sqlcmd.c_str(), &FindCallbackCount, &arg, NULL))) {
      return false;
    };
    if(arg.count == 0) {
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
      // todo: protection encoding
      std::string sqlcmd = "SELECT uid FROM rec WHERE ((id = "+id+") AND (owner = "+owner+"))";
      FindCallbackUidArg arg(uid);
      if(!dberr("Failed to retrieve record from database",sqlite3_exec(db_, sqlcmd.c_str(), &FindCallbackUid, &arg, NULL))) {
        return false; // No such record?
      };
    };
    if(uid.empty()) {
      error_str_ = "Record not found";
      return false; // No such record
    };
    {
      std::string sqlcmd = "SELECT FROM lock WHERE (uid = "+uid+")";
      FindCallbackCountArg arg;
      if(!dberr("Failed to find locks in database",sqlite3_exec(db_, sqlcmd.c_str(), &FindCallbackCount, &arg, NULL))) {
        return false;
      };
      if(arg.count > 0) {
        error_str_ = "Record has active locks";
        return false; // have locks
      };
    };
    ::unlink(uid_to_path(uid).c_str()); // TODO: handle error
    {
      std::string sqlcmd = "DELETE FROM rec WHERE (uid = "+uid+")";
      FindCallbackCountArg arg;
      if(!dberr("Failed to delete record in database",sqlite3_exec(db_, sqlcmd.c_str(), &FindCallbackCount, &arg, NULL))) {
        return false;
      };
      if(arg.count == 0) {
        error_str_ = "Failed to delete record in database";
        return false; // no such record
      };
    };
    return true;
  }

  bool FileRecordSQLite::AddLock(const std::string& lock_id, const std::list<std::string>& ids, const std::string& owner) {
    if(!valid_) return false;
    Glib::Mutex::Lock lock(lock_);
    for(std::list<std::string>::const_iterator id = ids.begin(); id != ids.end(); ++id) {
      std::string uid;
      {
        // todo: protection encoding
        std::string sqlcmd = "SELECT uid FROM rec WHERE ((id = "+(*id)+") AND (owner = "+owner+"))";
        FindCallbackUidArg arg(uid);
        if(!dberr("Failed to retrieve record from database",sqlite3_exec(db_, sqlcmd.c_str(), &FindCallbackUid, &arg, NULL))) {
          return false; // No such record?
        };
      };
      if(uid.empty()) {
        // No such record
        continue;
      };
      std::string sqlcmd = "INSERT INTO lock(lockid, uid) VALUES ("+lock_id+","+uid+")";
      if(!dberr("addlock:put",sqlite3_exec(db_, sqlcmd.c_str(), NULL, NULL, NULL))) {
        return false;
      };
    };
    return true;
  }

  bool FileRecordSQLite::RemoveLock(const std::string& lock_id) {
    std::list<std::pair<std::string,std::string> > ids;
    return RemoveLock(lock_id,ids);
  }

  bool FileRecordSQLite::RemoveLock(const std::string& lock_id, std::list<std::pair<std::string,std::string> >& ids) {
    if(!valid_) return false;
    Glib::Mutex::Lock lock(lock_);
    std::list<std::string> uids;
    // todo: protection encoding
    std::string sqlcmd = "SELECT rowid, uid FROM lock WHERE (lockid = "+lock_id+")";


/*
    Dbc* cur = NULL;
    if(!dberr("removelock:cursor",db_lock_->cursor(NULL,&cur,DB_WRITECURSOR))) return false;
    Dbt key;
    Dbt data;
    make_string(lock_id,key);
    void* pkey = key.get_data();
    if(!dberr("removelock:get1",cur->get(&key,&data,DB_SET))) { // TODO: handle errors
      ::free(pkey);
      cur->close(); return false;
    };
    for(;;) {
      std::string id;
      std::string owner;
      uint32_t size = data.get_size();
      void* buf = data.get_data();
      buf = parse_string(id,buf,size); //  lock_id - skip
      buf = parse_string(id,buf,size);
      buf = parse_string(owner,buf,size);
      ids.push_back(std::pair<std::string,std::string>(id,owner));
      if(!dberr("removelock:del",cur->del(0))) {
        ::free(pkey);
        cur->close(); return false;
      };
      if(!dberr("removelock:get2",cur->get(&key,&data,DB_NEXT_DUP))) break;
    };
    db_lock_->sync(0);
    ::free(pkey);
    cur->close();
*/
    return true;
  }


  bool FileRecordSQLite::ListLocked(const std::string& lock_id, std::list<std::pair<std::string,std::string> >& ids) {
    if(!valid_) return false;
    Glib::Mutex::Lock lock(lock_);
/*
    Dbc* cur = NULL;
    if(!dberr("listlocked:cursor",db_lock_->cursor(NULL,&cur,0))) return false;
    Dbt key;
    Dbt data;
    make_string(lock_id,key);
    void* pkey = key.get_data();
    if(!dberr("listlocked:get1",cur->get(&key,&data,DB_SET))) { // TODO: handle errors
      ::free(pkey);
      cur->close(); return false;
    };
    for(;;) {
      std::string id;
      std::string owner;
      uint32_t size = data.get_size();
      void* buf = data.get_data();
      buf = parse_string(id,buf,size); //  lock_id - skip
      buf = parse_string(id,buf,size);
      buf = parse_string(owner,buf,size);
      ids.push_back(std::pair<std::string,std::string>(id,owner));
      if(cur->get(&key,&data,DB_NEXT_DUP) != 0) break;
    };
    ::free(pkey);
    cur->close();
*/
    return true;
  }

  bool FileRecordSQLite::ListLocks(std::list<std::string>& locks) {
    if(!valid_) return false;
    Glib::Mutex::Lock lock(lock_);
/*
    Dbc* cur = NULL;
    if(db_lock_->cursor(NULL,&cur,0)) return false;
    for(;;) {
      Dbt key;
      Dbt data;
      if(cur->get(&key,&data,DB_NEXT_NODUP) != 0) break; // TODO: handle errors
      std::string str;
      uint32_t size = key.get_size();
      parse_string(str,key.get_data(),size);
      locks.push_back(str);
    };
    cur->close();
*/
    return true;
  }

  bool FileRecordSQLite::ListLocks(const std::string& id, const std::string& owner, std::list<std::string>& locks) {
    // Not implemented yet
    return false;
  }

  FileRecordSQLite::Iterator::Iterator(FileRecordSQLite& frec):FileRecord::Iterator(frec) {
/*
    Glib::Mutex::Lock lock(frec_.lock_);
    if(!frec_.dberr("Iterator:cursor",frec_.db_rec_->cursor(NULL,&cur_,0))) {
      if(cur_) {
        cur_->close(); cur_=NULL;
      };
      return;
    };
    Dbt key;
    Dbt data;
    if(!frec_.dberr("Iterator:first",cur_->get(&key,&data,DB_FIRST))) {
      cur_->close(); cur_=NULL;
      return;
    };
    parse_record(uid_,id_,owner_,meta_,key,data);
*/
  }

  FileRecordSQLite::Iterator::~Iterator(void) {
/*
    Glib::Mutex::Lock lock(frec_.lock_);
    if(cur_) {
      cur_->close(); cur_=NULL;
    };
*/
  }

  FileRecordSQLite::Iterator& FileRecordSQLite::Iterator::operator++(void) {
/*
    if(!cur_) return *this;
    Glib::Mutex::Lock lock(frec_.lock_);
    Dbt key;
    Dbt data;
    if(!frec_.dberr("Iterator:first",cur_->get(&key,&data,DB_NEXT))) {
      cur_->close(); cur_=NULL;
      return *this;
    };
    parse_record(uid_,id_,owner_,meta_,key,data);
*/
    return *this;
  }

  FileRecordSQLite::Iterator& FileRecordSQLite::Iterator::operator--(void) {
/*
    if(!cur_) return *this;
    Glib::Mutex::Lock lock(frec_.lock_);
    Dbt key;
    Dbt data;
    if(!frec_.dberr("Iterator:first",cur_->get(&key,&data,DB_PREV))) {
      cur_->close(); cur_=NULL;
      return *this;
    };
    parse_record(uid_,id_,owner_,meta_,key,data);
*/
    return *this;
  }

  void FileRecordSQLite::Iterator::suspend(void) {
/*
    Glib::Mutex::Lock lock(frec_.lock_);
    if(cur_) {
      cur_->close(); cur_=NULL;
    }
*/
  }

  bool FileRecordSQLite::Iterator::resume(void) {
/*
    Glib::Mutex::Lock lock(frec_.lock_);
    if(!cur_) {
      if(id_.empty()) return false;
      if(!frec_.dberr("Iterator:cursor",frec_.db_rec_->cursor(NULL,&cur_,0))) {
        if(cur_) {
          cur_->close(); cur_=NULL;
        };
        return false;
      };
      Dbt key;
      Dbt data;
      make_key(id_,owner_,key);
      void* pkey = key.get_data();
      if(!frec_.dberr("Iterator:first",cur_->get(&key,&data,DB_SET))) {
        ::free(pkey);
        cur_->close(); cur_=NULL;
        return false;
      };
      parse_record(uid_,id_,owner_,meta_,key,data);
      ::free(pkey);
    };
*/
    return true;
  }

} // namespace ARex

