#ifndef __ARC_DELEGATION_STORE_H__
#define __ARC_DELEGATION_STORE_H__

#include <string>
#include <list>
#include <map>

#include <arc/delegation/DelegationInterface.h>
#include <arc/Logger.h>

#include "FileRecord.h"

namespace ARex {

class DelegationStore: public Arc::DelegationContainerSOAP {
 private:
  class Consumer {
   public:
    std::string id;
    std::string client;
    std::string path;
    Consumer(const std::string& id_, const std::string& client_, const std::string& path_):
       id(id_),client(client_),path(path_) {
    };
  };
  Glib::Mutex lock_;
  Glib::Mutex check_lock_;
  FileRecord* fstore_;
  std::map<Arc::DelegationConsumerSOAP*,Consumer> acquired_;
  unsigned int expiration_;
  unsigned int maxrecords_;
  unsigned int mtimeout_;
  FileRecord::Iterator* mrec_;
  Arc::Logger logger_;
 public:
  enum DbType {
    DbBerkeley,
    DbSQLite
  };

  DelegationStore(const std::string& base, DbType db, bool allow_recover = true);

  ~DelegationStore(void);

  operator bool(void) { return ((bool)fstore_ && (bool)*fstore_); };

  bool operator!(void) { return !((bool)fstore_ && (bool)*fstore_); };

  /** Returns description of last error */
  std::string Error(void) { return fstore_?fstore_->Error():std::string(""); };

  /** Sets expiration time for unlocked credentials */
  void Expiration(unsigned int v = 0) { expiration_ = v; };

  /** Sets max number of credentials to store */
  void MaxRecords(unsigned int v = 0) { maxrecords_ = v; };

  void CheckTimeout(unsigned int v = 0) { mtimeout_ = v; };

  /** Create a slot for credential storing and return associated delegation consumer.
     The consumer object must be release with ReleaseConsumer/RemoveConsumer */
  virtual Arc::DelegationConsumerSOAP* AddConsumer(std::string& id,const std::string& client);

  /** Find existing delegation slot and create delegation consumer for it.
     The consumer object must be release with ReleaseConsumer/RemoveConsumer */
  virtual Arc::DelegationConsumerSOAP* FindConsumer(const std::string& id,const std::string& client);

  /** Store credentials into slot associated with specified consumer object */
  virtual bool TouchConsumer(Arc::DelegationConsumerSOAP* c,const std::string& credentials);

  /** Read credentials stored in slot associated with specified consumer object */
  virtual bool QueryConsumer(Arc::DelegationConsumerSOAP* c,std::string& credentials);

  /** Release consumer object but keep credentials store slot */
  virtual void ReleaseConsumer(Arc::DelegationConsumerSOAP* c);

  /** Release consumer object and delete associated credentials store slot */
  virtual bool RemoveConsumer(Arc::DelegationConsumerSOAP* c);

  virtual void CheckConsumers(void);

  void PeriodicCheckConsumers(void);

  /** Store new credentials associated with client and assign id to it */
  bool AddCred(std::string& id, const std::string& client, const std::string& credentials);
 
  /** Store/update credentials with specified id and associated with client */
  bool PutCred(const std::string& id, const std::string& client, const std::string& credentials);

  /** Returns path to file containing credential with specied id and client */
  std::string FindCred(const std::string& id,const std::string& client);

  /** Retrieves credentials with specified id and associated with client */
  bool GetCred(const std::string& id, const std::string& client, std::string& credentials);

  /** Retrieves locks associated with specified id and client */
  bool GetLocks(const std::string& id, const std::string& client, std::list<std::string>& lock_ids);

  /** Retrieves all locks known */
  bool GetLocks(std::list<std::string>& lock_ids);

  /** Returns credentials ids associated with specific client */
  std::list<std::string> ListCredIDs(const std::string& client);

  /** Returns all credentials ids (1st) along with their client ids (2nd) */
  std::list<std::pair<std::string,std::string> > ListCredIDs(void);

  /** Returns credentials ids and their metadata associated with specific client */
  std::list<std::pair<std::string,std::list<std::string> > > ListCredInfos(const std::string& client);

  /** Locks credentials also associating it with specific lock identifier */
  bool LockCred(const std::string& lock_id, const std::list<std::string>& ids,const std::string& client);

  /** Release lock set by previous call to LockCred by associated lock id.
     Optionally it can update credentials usage timestamp and
     force removal credentials from storage if it is not locked anymore. */
  bool ReleaseCred(const std::string& lock_id, bool touch = false, bool remove = false);

  /** Returns credential ids locked by specific lock id and associated with specified client */
  std::list<std::string> ListLockedCredIDs(const std::string& lock_id, const std::string& client);

  /** Returns credential ids locked by specific lock id */
  std::list<std::pair<std::string,std::string> > ListLockedCredIDs(const std::string& lock_id);

  /** Provides delegation request specified 'id' and 'client'. If 'id' is empty
      then new storage slot is created and its identifier stored in 'id'. */
  bool GetRequest(std::string& id,const std::string& client,std::string& request);

  /** Stores delegated credentials corresponding to delegation request obtained by call to GetRequest().
      Only public part is expected in 'credentials'. */
  bool PutDeleg(const std::string& id,const std::string& client,const std::string& credentials);

  /** Retrieves public part of credentials with specified id and associated with client */
  bool GetDeleg(const std::string& id, const std::string& client, std::string& credentials);

  /** Stores full credentials into specified 'id' and 'client'. If 'id' is empty
      then new storage slot is created and its identifier stored in 'id'. */
  bool PutCred(std::string& id,const std::string& client,const std::string& credentials,const std::list<std::string>& meta = std::list<std::string>());
};

} // namespace ARex

#endif // __ARC_DELEGATION_STORE_H__

