#ifndef __ARC_DELEGATION_FILERECORD_H__
#define __ARC_DELEGATION_FILERECORD_H__

#include <list>
#include <string>

namespace ARex {

class FileRecord {
 protected:
  std::string basepath_;
  int error_num_;
  std::string error_str_;
  bool valid_;
  std::string uid_to_path(const std::string& uid);
  bool make_file(const std::string& uid);
  bool remove_file(const std::string& uid);
 public:
  class Iterator {
   private:
    Iterator(const Iterator&); // disabled copy constructor
   protected:
    Iterator(FileRecord& frec):frec_(frec) {};
    FileRecord& frec_;
    std::string uid_;
    std::string id_;
    std::string owner_;
    std::list<std::string> meta_;
   public:
    virtual ~Iterator(void) {};
    virtual Iterator& operator++(void) = 0;
    virtual Iterator& operator--(void) = 0;
    virtual void suspend(void) = 0;
    virtual bool resume(void) = 0;
    virtual operator bool(void) = 0;
    virtual bool operator!(void) = 0;
    const std::string& uid(void) const { return uid_; };
    const std::string& id(void) const { return id_; };
    const std::string& owner(void) const { return owner_; };
    const std::list<std::string>& meta(void) const { return meta_; };
    const std::string path(void) const { return frec_.uid_to_path(uid_); };
  };
  friend class FileRecord::Iterator;
  FileRecord(const std::string& base, bool create = true): basepath_(base), error_num_(0), valid_(false) {};
  virtual ~FileRecord(void) {};
  operator bool(void) { return valid_; };
  bool operator!(void) { return !valid_; };
  std::string Error(void) { return error_str_; };
  virtual Iterator* NewIterator(void) = 0;
  virtual bool Recover(void) = 0;
  virtual std::string Add(std::string& id, const std::string& owner, const std::list<std::string>& meta) = 0;
  virtual std::string Find(const std::string& id, const std::string& owner, std::list<std::string>& meta) = 0;
  virtual bool Modify(const std::string& id, const std::string& owner, const std::list<std::string>& meta) = 0;
  virtual bool Remove(const std::string& id, const std::string& owner) = 0;
  // Assign specified credential ids specified lock lock_id
  virtual bool AddLock(const std::string& lock_id, const std::list<std::string>& ids, const std::string& owner) = 0;
  // Reomove lock lock_id from all associated credentials
  virtual bool RemoveLock(const std::string& lock_id) = 0;
  // Reomove lock lock_id from all associated credentials and store 
  // identifiers of associated credentials into ids
  virtual bool RemoveLock(const std::string& lock_id, std::list<std::pair<std::string,std::string> >& ids) = 0;
  // Fills locks with all known lock ids.
  virtual bool ListLocks(std::list<std::string>& locks) = 0;
  // Fills locks with all lock ids associated with specified credential id
  virtual bool ListLocks(const std::string& id, const std::string& owner, std::list<std::string>& locks) = 0;
  // Fills ids with identifiers of credentials locked by specified lock_id lock
  virtual bool ListLocked(const std::string& lock_id, std::list<std::pair<std::string,std::string> >& ids) = 0;
};

} // namespace ARex

#endif // __ARC_DELEGATION_FiLERECORD_H__

