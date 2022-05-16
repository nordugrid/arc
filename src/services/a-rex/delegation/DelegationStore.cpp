#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utime.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include <arc/FileUtils.h>

#include "FileRecordSQLite.h"
#ifdef HAVE_DBCXX
#include "FileRecordBDB.h"
#endif

#include "DelegationStore.h"

namespace ARex {

  DelegationStore::DelegationStore(const std::string& base, DbType db, bool allow_recover):
           logger_(Arc::Logger::rootLogger, "Delegation Storage") {
    fstore_ = NULL;
    expiration_ = 0;
    maxrecords_ = 0;
    mtimeout_ = 0;
    mrec_ = NULL;
    switch(db) {
#ifdef HAVE_DBCXX
      case DbBerkeley:
        fstore_ = new FileRecordBDB(base, allow_recover);
        break;
#endif
      case DbSQLite:
        fstore_ = new FileRecordSQLite(base, allow_recover);
        break;
      default:
        failure_ = "Unsupported database type requested for delegation storage.";
        logger_.msg(Arc::ERROR,"%s",failure_);
        return;
    };
    if(!*fstore_) {
      failure_ = "Failed to initialize storage. " + fstore_->Error();
      logger_.msg(Arc::WARNING,"%s",failure_);
      if(allow_recover) {
        // Database creation failed. Try recovery.
        if(!fstore_->Recover()) {
          failure_ = "Failed to recover storage. " + fstore_->Error();
          logger_.msg(Arc::WARNING,"%s",failure_);
          logger_.msg(Arc::WARNING,"Wiping and re-creating whole storage");
          delete fstore_; fstore_ = NULL;
          // Full recreation of database. Delete everything.
          Glib::Dir dir(base);
          std::string name;
          while ((name = dir.read_name()) != "") {
            std::string fullpath(base);
            fullpath += G_DIR_SEPARATOR_S + name;
            struct stat st;
            if (::lstat(fullpath.c_str(), &st) == 0) {
              if(S_ISDIR(st.st_mode)) {
                Arc::DirDelete(fullpath.c_str());
              } else {
                Arc::FileDelete(fullpath.c_str());
              };
            };
          };
          switch(db) {
#ifdef HAVE_DBCXX
            case DbBerkeley:
              fstore_ = new FileRecordBDB(base);
              break;
#endif
            case DbSQLite:
              fstore_ = new FileRecordSQLite(base);
              break;
            default:
              // Must not happen - already sorted out above.
              return;
          };
          if(!*fstore_) {
            // Failure
            failure_ = "Failed to re-create storage. " + fstore_->Error();
            logger_.msg(Arc::WARNING,"%s",failure_);
          } else {
            // Database recreated.
          };
        };
      } else {
        logger_.msg(Arc::ERROR,"%s",failure_);
      };
    };
    // TODO: Do some cleaning on startup
  }

  DelegationStore::~DelegationStore(void) {
    // BDB objects must be destroyed because
    // somehow BDB does not understand that process
    // already died and keeps locks forewer.
    delete mrec_;
    delete fstore_;
    /* Following code is not executed because there must be no active 
      consumers when store being destroyed. It is probably safer to
      leave hanging consumers than to destroy them.
      Anyway by design this destructor is supposed to be called only 
      when applications exits.
       
    while(acquired_.size() > 0) {
      std::map<Arc::DelegationConsumerSOAP*,Consumer>::iterator i = acquired_.begin();
      delete i->first;
      acquired_.erase(i);
    };
    */
  }

  Arc::DelegationConsumerSOAP* DelegationStore::AddConsumer(std::string& id,const std::string& client) {
    std::string path = fstore_->Add(id,client,std::list<std::string>());
    if(path.empty()) {
      failure_ = "Local error - failed to create slot for delegation. "+fstore_->Error();
      return NULL;
    }
    Arc::DelegationConsumerSOAP* cs = new Arc::DelegationConsumerSOAP();
    std::string key;
    cs->Backup(key);
    if(!key.empty()) {
      if(!Arc::FileCreate(path,key,0,0,S_IRUSR|S_IWUSR)) {
        fstore_->Remove(id,client);
        delete cs; cs = NULL;
        failure_ = "Local error - failed to store credentials";
        return NULL;
      };
    };
    Glib::Mutex::Lock lock(lock_);
    acquired_.insert(std::pair<Arc::DelegationConsumerSOAP*,Consumer>(cs,Consumer(id,client,path)));
    return cs;
  }

  static const char* key_start_tag("-----BEGIN RSA PRIVATE KEY-----");
  static const char* key_end_tag("-----END RSA PRIVATE KEY-----");

  static std::string extract_key(const std::string& proxy) {
    std::string key;
    std::string::size_type start = proxy.find(key_start_tag);
    if(start != std::string::npos) {
      std::string::size_type end = proxy.find(key_end_tag,start+strlen(key_start_tag));
      if(end != std::string::npos) {
        return proxy.substr(start,end-start+strlen(key_end_tag));
      };
    };
    return "";
  }

  static void remove_key(std::string& proxy) {
    while(true) {
      std::string::size_type start = proxy.find(key_start_tag);
      if(start == std::string::npos) break;
      std::string::size_type end = proxy.find(key_end_tag,start+strlen(key_start_tag));
      if(end == std::string::npos) end = proxy.length();
      proxy.erase(start,end-start+strlen(key_end_tag));
    };
  }

  static bool compare_no_newline(const std::string& str1, const std::string& str2) {
    std::string::size_type p1 = 0;
    std::string::size_type p2 = 0;
    for(;;) {
      if((p1 < str1.length()) && ((str1[p1] == '\r') || (str1[p1] == '\n'))) {
        ++p1; continue;
      };
      if((p2 < str2.length()) && ((str2[p2] == '\r') || (str2[p2] == '\n'))) {
        ++p2; continue;
      };
      if(p1 >= str1.length()) break;
      if(p2 >= str2.length()) break;
      if(str1[p1] != str2[p2]) break;
      ++p1; ++p2;
    };
    return ((p1 >= str1.length()) && (p2 >= str2.length()));
  }

  Arc::DelegationConsumerSOAP* DelegationStore::FindConsumer(const std::string& id,const std::string& client) {
    std::list<std::string> meta;
    std::string path = fstore_->Find(id,client,meta);
    if(path.empty()) {
      failure_ = "Identifier not found for client. "+fstore_->Error();
      return NULL;
    };
    std::string content;
    if(!Arc::FileRead(path,content)) {
      failure_ = "Local error - failed to read credentials";
      return NULL;
    };
    Arc::DelegationConsumerSOAP* cs = new Arc::DelegationConsumerSOAP();
    if(!content.empty()) {
      std::string key = extract_key(content);
      if(!key.empty()) {
        cs->Restore(key);
      };
    };
    Glib::Mutex::Lock lock(lock_);
    acquired_.insert(std::pair<Arc::DelegationConsumerSOAP*,Consumer>(cs,Consumer(id,client,path)));
    return cs;
  }

  bool DelegationStore::TouchConsumer(Arc::DelegationConsumerSOAP* c,const std::string& credentials) {
    if(!c) return false;
    Glib::Mutex::Lock lock(lock_);
    std::map<Arc::DelegationConsumerSOAP*,Consumer>::iterator i = acquired_.find(c);
    if(i == acquired_.end()) {
      failure_ = "Delegation not found";
      return false;
    };
    if(!credentials.empty()) {
      if(!Arc::FileCreate(i->second.path,credentials,0,0,S_IRUSR|S_IWUSR)) {
        failure_ = "Local error - failed to create storage for delegation";
        logger_.msg(Arc::WARNING,"DelegationStore: TouchConsumer failed to create file %s",i->second.path);
        return false;
      };
    };
    return true;
  }

  bool DelegationStore::QueryConsumer(Arc::DelegationConsumerSOAP* c,std::string& credentials) {
    if(!c) return false;
    Glib::Mutex::Lock lock(lock_);
    std::map<Arc::DelegationConsumerSOAP*,Consumer>::iterator i = acquired_.find(c);
    if(i == acquired_.end()) { failure_ = "Delegation not found"; return false; };
    Arc::FileRead(i->second.path,credentials);
    return true;
  }

  void DelegationStore::ReleaseConsumer(Arc::DelegationConsumerSOAP* c) {
    if(!c) return;
    Glib::Mutex::Lock lock(lock_);
    std::map<Arc::DelegationConsumerSOAP*,Consumer>::iterator i = acquired_.find(c);
    if(i == acquired_.end()) return; // ????
    // Check if key changed. If yes then store only key.
    // TODO: optimize
    std::string newkey;
    i->first->Backup(newkey);
    if(!newkey.empty()) {
      std::string oldkey;
      std::string content;
      Arc::FileRead(i->second.path,content);
      if(!content.empty()) oldkey = extract_key(content);
      if(!compare_no_newline(newkey,oldkey)) {
        Arc::FileCreate(i->second.path,newkey,0,0,S_IRUSR|S_IWUSR);
      };
    };
    delete i->first;
    acquired_.erase(i);
  }

  bool DelegationStore::RemoveConsumer(Arc::DelegationConsumerSOAP* c) {
    if(!c) return false;
    Glib::Mutex::Lock lock(lock_);
    std::map<Arc::DelegationConsumerSOAP*,Consumer>::iterator i = acquired_.find(c);
    if(i == acquired_.end()) return false; // ????
    bool r = fstore_->Remove(i->second.id,i->second.client); // TODO: Handle failure
    delete i->first;
    acquired_.erase(i);
    return r;
  }

  void DelegationStore::CheckConsumers(void) {
    // Not doing any cleaning ocasionally to avoid delegation response delay.
    // Instead PeriodicCheckConsumers() is called to do periodic cleaning.
  }

  void DelegationStore::PeriodicCheckConsumers(void) {
    // Go through stored credentials
    // Remove outdated records (those with locks won't be removed)
    if(expiration_) {
      time_t start = ::time(NULL);
      Glib::Mutex::Lock check_lock(lock_);
      if(mrec_ != NULL) {
        if(!mrec_->resume()) {
          logger_.msg(Arc::WARNING,"DelegationStore: PeriodicCheckConsumers failed to resume iterator");
          delete mrec_;
          mrec_ = NULL;
        };
      };
      if(mrec_ == NULL) {
        mrec_ = fstore_->NewIterator();
      };
      for(;(bool)(*mrec_);++(*mrec_)) {
        if(mtimeout_ && (((unsigned int)(::time(NULL) - start)) > mtimeout_)) {
          mrec_->suspend();
          return;
        }
        struct stat st;
        if(::stat(mrec_->path().c_str(),&st) == 0) {
          if(((unsigned int)(::time(NULL) - st.st_mtime)) > expiration_) {
            if(fstore_->Remove(mrec_->id(),mrec_->owner())) {
            } else {
              // It is ok to fail here because Remove checks for delegation locks.
              // So reporting only for debuging purposes.
              logger_.msg(Arc::DEBUG,"DelegationStore: PeriodicCheckConsumers failed to remove old delegation %s - %s", mrec_->uid(), fstore_->Error());
            };
          };    
        };
      };
      delete mrec_; mrec_ = NULL;
    };
    // TODO: Remove records over threshold
    return;
  }

  bool DelegationStore::AddCred(std::string& id, const std::string& client, const std::string& credentials) {
    std::string path = fstore_->Add(id,client,std::list<std::string>());
    if(path.empty()) {
      failure_ = "Local error - failed to create slot for delegation. "+fstore_->Error();
      return false;
    }
    if(!Arc::FileCreate(path,credentials,0,0,S_IRUSR|S_IWUSR)) {
      fstore_->Remove(id,client);
      failure_ = "Local error - failed to create storage for delegation";
      logger_.msg(Arc::WARNING,"DelegationStore: TouchConsumer failed to create file %s",path);
      return false;
    };
    return true;
  }

  bool DelegationStore::PutCred(const std::string& id, const std::string& client, const std::string& credentials) {
    std::list<std::string> meta;
    std::string path = fstore_->Find(id,client,meta);
    if(path.empty()) {
      failure_ = "Local error - failed to find specified credentials. "+fstore_->Error();
      return false;
    }
    if(!Arc::FileCreate(path,credentials,0,0,S_IRUSR|S_IWUSR)) {
      failure_ = "Local error - failed to store delegation";
      return false;
    };
    return true;
  }

  std::string DelegationStore::FindCred(const std::string& id,const std::string& client) {
    std::list<std::string> meta;
    return fstore_->Find(id,client,meta);
  }

  bool DelegationStore::GetCred(const std::string& id, const std::string& client, std::string& credentials) {
    std::list<std::string> meta;
    std::string path = fstore_->Find(id,client,meta);
    if(path.empty()) {
      failure_ = "Local error - failed to find specified credentials. "+fstore_->Error();
      return false;
    }
    std::string content;
    if(!Arc::FileRead(path,credentials)) {
      failure_ = "Local error - failed to read credentials";
      return false;
    };
    return true;
  }

  bool DelegationStore::GetLocks(const std::string& id, const std::string& client, std::list<std::string>& lock_ids) {
    return fstore_->ListLocks(id, client, lock_ids);
  }

  bool DelegationStore::GetLocks(std::list<std::string>& lock_ids) {
    return fstore_->ListLocks(lock_ids);
  }

  std::list<std::string> DelegationStore::ListCredIDs(const std::string& client) {
    std::list<std::string> res;
    FileRecord::Iterator& rec = *(fstore_->NewIterator());
    for(;(bool)rec;++rec) {
      if(rec.owner() == client) res.push_back(rec.id());
    };
    delete &rec;
    return res;
  }

  std::list<std::pair<std::string,std::string> > DelegationStore::ListLockedCredIDs(const std::string& lock_id) {
    std::list<std::pair<std::string,std::string> > ids;
    (void)fstore_->ListLocked(lock_id, ids);
    return ids;
  }

  std::list<std::string> DelegationStore::ListLockedCredIDs(const std::string& lock_id, const std::string& client) {
    std::list<std::string> res;
    std::list<std::pair<std::string,std::string> > ids;
    if(!fstore_->ListLocked(lock_id, ids)) return res;
    for(std::list<std::pair<std::string,std::string> >::iterator id = ids.begin();
                               id != ids.end();++id) {
      if(id->second == client) res.push_back(id->first);
    }
    return res;
  }

  std::list<std::pair<std::string,std::string> > DelegationStore::ListCredIDs(void) {
    std::list<std::pair<std::string,std::string> > res;
    FileRecord::Iterator& rec = *(fstore_->NewIterator());
    for(;(bool)rec;++rec) {
      res.push_back(std::pair<std::string,std::string>(rec.id(),rec.owner()));
    };
    delete &rec;
    return res;
  }


  bool DelegationStore::LockCred(const std::string& lock_id, const std::list<std::string>& ids,const std::string& client) {
    if(!fstore_->AddLock(lock_id,ids,client)) {
      failure_ = "Local error - failed set lock for delegation. "+fstore_->Error();
      return false;
    };
    return true;
  }

  bool DelegationStore::ReleaseCred(const std::string& lock_id, bool touch, bool remove) {
    if((!touch) && (!remove)) return fstore_->RemoveLock(lock_id);
    std::list<std::pair<std::string,std::string> > ids;
    if(!fstore_->RemoveLock(lock_id,ids)) return false;
    for(std::list<std::pair<std::string,std::string> >::iterator i = ids.begin();
                        i != ids.end(); ++i) {    
      if(touch) {
        std::list<std::string> meta;
        std::string path = fstore_->Find(i->first,i->second,meta);
        // TODO: in a future use meta for storing times
        if(!path.empty()) ::utime(path.c_str(),NULL);
      };
      if(remove) fstore_->Remove(i->first,i->second);
    };
    return true;
  }

  bool DelegationStore::GetRequest(std::string& id,const std::string& client,std::string& request) {
    Arc::DelegationConsumerSOAP* consumer = NULL;
    if(!id.empty()) {
      consumer = FindConsumer(id,client);
    };
    if(consumer == NULL) {
      consumer = AddConsumer(id,client);
    };
    if(consumer == NULL) return false;
    if(id.empty()) {
      ReleaseConsumer(consumer);
      return false;
    };
    bool result = consumer->Request(request);
    ReleaseConsumer(consumer);
    return result;
  }

  bool DelegationStore::PutDeleg(const std::string& id,const std::string& client,const std::string& credentials) {
    Arc::DelegationConsumerSOAP* consumer = FindConsumer(id,client);
    if(consumer == NULL) return false;
    std::string content(credentials);
    if(!consumer->Acquire(content)) {
      ReleaseConsumer(consumer);
      return false;
    };
    if(!TouchConsumer(consumer,content)) {
      ReleaseConsumer(consumer);
      return false;
    };
    ReleaseConsumer(consumer);
    return true;
  }

  bool DelegationStore::GetDeleg(const std::string& id, const std::string& client, std::string& credentials) {
    std::string creds;
    if(!GetCred(id, client, credentials))
      return false;
    remove_key(credentials);
    return true;
  }

} // namespace ARex

