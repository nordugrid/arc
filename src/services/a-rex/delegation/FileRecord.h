#ifndef __ARC_DELEGATION_FILERECORD_H__
#define __ARC_DELEGATION_FILERECORD_H__

#include <list>
#include <string>

#include <db_cxx.h>

#include <arc/Thread.h>

namespace ARex {

class FileRecord {
 private:
  Glib::Mutex lock_; // TODO: use DB locking
  std::string basepath_;
  DbEnv* db_env_;
  Db*    db_rec_;
  Db*    db_lock_;
  Db*    db_locked_;
  Db*    db_link_;
  int error_num_;
  std::string error_str_;
  bool valid_;
  static int locked_callback(Db *, const Dbt *, const Dbt *, Dbt * result);
  static int lock_callback(Db *, const Dbt *, const Dbt *, Dbt * result);
  std::string uid_to_path(const std::string& uid);
  bool dberr(const char* s, int err);
  bool open(bool create);
  void close(void);
  bool verify(void);
 public:
  class Iterator {
   private:
    FileRecord& frec_;
    Dbc* cur_;
    Iterator(const Iterator&);
    std::string uid_;
    std::string id_;
    std::string owner_;
    std::list<std::string> meta_;
   public:
    Iterator(FileRecord& frec);
    ~Iterator(void);
    Iterator& operator++(void);
    Iterator& operator--(void);
    operator bool(void) { return (cur_!=NULL); };
    bool operator!(void) { return (cur_==NULL); };
    const std::string& uid(void) const { return uid_; };
    const std::string& id(void) const { return id_; };
    const std::string& owner(void) const { return owner_; };
    const std::list<std::string>& meta(void) const { return meta_; };
    const std::string path(void) const { return frec_.uid_to_path(uid_); };
  };
  friend class FileRecord::Iterator;
  FileRecord(const std::string& base, bool create = true);
  ~FileRecord(void);
  operator bool(void) { return valid_; };
  bool operator!(void) { return !valid_; };
  std::string Error(void) { return error_str_; };
  bool Recover(void);
  std::string Add(std::string& id, const std::string& owner, const std::list<std::string>& meta);
  std::string Find(const std::string& id, const std::string& owner, std::list<std::string>& meta);
  bool Modify(const std::string& id, const std::string& owner, const std::list<std::string>& meta);
  bool Remove(const std::string& id, const std::string& owner);
  // Assign specified credential ids specified lock lock_id
  bool AddLock(const std::string& lock_id, const std::list<std::string>& ids, const std::string& owner);
  // Reomove lock lock_id from all associated credentials
  bool RemoveLock(const std::string& lock_id);
  // Reomove lock lock_id from all associated credentials and store 
  // identifiers of associated credentials into ids
  bool RemoveLock(const std::string& lock_id, std::list<std::pair<std::string,std::string> >& ids);
  // Fills locks with all known lock ids.
  bool ListLocks(std::list<std::string>& locks);
  // Fills locks with all lock ids associated with specified credential id
  bool ListLocks(const std::string& id, const std::string& owner, std::list<std::string>& locks);
  // Fills ids with identifiers of credentials locked by specified lock_id lock
  bool ListLocked(const std::string& lock_id, std::list<std::pair<std::string,std::string> >& ids);
};

} // namespace ARex

#endif // __ARC_DELEGATION_FiLERECORD_H__

