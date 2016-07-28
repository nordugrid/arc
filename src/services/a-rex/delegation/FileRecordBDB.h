#ifndef __ARC_DELEGATION_FILERECORDBDB_H__
#define __ARC_DELEGATION_FILERECORDBDB_H__

#include <list>
#include <string>

#include <db_cxx.h>

#include <arc/Thread.h>

#include "FileRecord.h"

namespace ARex {

class FileRecordBDB: public FileRecord {
 private:
  Glib::Mutex lock_; // TODO: use DB locking
  DbEnv* db_env_;
  Db*    db_rec_;
  Db*    db_lock_;
  Db*    db_locked_;
  Db*    db_link_;
  static int locked_callback(Db *, const Dbt *, const Dbt *, Dbt * result);
  static int lock_callback(Db *, const Dbt *, const Dbt *, Dbt * result);
  bool dberr(const char* s, int err);
  bool open(bool create);
  void close(void);
  bool verify(void);
 public:
  class Iterator: public FileRecord::Iterator {
   friend class FileRecordBDB;
   private:
    Dbc* cur_;
    Iterator(const Iterator&); // disabled constructor
    Iterator(FileRecordBDB& frec);
   public:
    ~Iterator(void);
    virtual Iterator& operator++(void);
    virtual Iterator& operator--(void);
    virtual void suspend(void);
    virtual bool resume(void);
    virtual operator bool(void) { return (cur_!=NULL); };
    virtual bool operator!(void) { return (cur_==NULL); };
  };
  friend class FileRecordBDB::Iterator;
  FileRecordBDB(const std::string& base, bool create = true);
  virtual ~FileRecordBDB(void);
  virtual Iterator* NewIterator(void) { return new Iterator(*this); };
  virtual bool Recover(void);
  virtual std::string Add(std::string& id, const std::string& owner, const std::list<std::string>& meta);
  virtual bool Add(const std::string& uid, const std::string& id, const std::string& owner, const std::list<std::string>& meta);
  virtual std::string Find(const std::string& id, const std::string& owner, std::list<std::string>& meta);
  virtual bool Modify(const std::string& id, const std::string& owner, const std::list<std::string>& meta);
  virtual bool Remove(const std::string& id, const std::string& owner);
  // Assign specified credential ids specified lock lock_id
  virtual bool AddLock(const std::string& lock_id, const std::list<std::string>& ids, const std::string& owner);
  // Reomove lock lock_id from all associated credentials
  virtual bool RemoveLock(const std::string& lock_id);
  // Reomove lock lock_id from all associated credentials and store 
  // identifiers of associated credentials into ids
  virtual bool RemoveLock(const std::string& lock_id, std::list<std::pair<std::string,std::string> >& ids);
  // Fills locks with all known lock ids.
  virtual bool ListLocks(std::list<std::string>& locks);
  // Fills locks with all lock ids associated with specified credential id
  virtual bool ListLocks(const std::string& id, const std::string& owner, std::list<std::string>& locks);
  // Fills ids with identifiers of credentials locked by specified lock_id lock
  virtual bool ListLocked(const std::string& lock_id, std::list<std::pair<std::string,std::string> >& ids);
};

} // namespace ARex

#endif // __ARC_DELEGATION_FiLERECORDBDB_H__

