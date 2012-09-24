#include <string>
#include <list>
#include <map>

#include <arc/delegation/DelegationInterface.h>

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
 public:
  DelegationStore(const std::string& base);
  operator bool(void) { return (bool)*fstore_; };
  bool operator!(void) { return !*fstore_; };
  std::string Error(void) { return fstore_->Error(); };

  void Expiration(unsigned int v = 0) { expiration_ = v; };
  void MaxRecords(unsigned int v = 0) { maxrecords_ = v; };
  void CheckTimeout(unsigned int v = 0) { mtimeout_ = v; };

  virtual Arc::DelegationConsumerSOAP* AddConsumer(std::string& id,const std::string& client);
  virtual Arc::DelegationConsumerSOAP* FindConsumer(const std::string& id,const std::string& client);
  virtual bool TouchConsumer(Arc::DelegationConsumerSOAP* c,const std::string& credentials);
  virtual bool QueryConsumer(Arc::DelegationConsumerSOAP* c,std::string& credentials);
  virtual void ReleaseConsumer(Arc::DelegationConsumerSOAP* c);
  virtual void RemoveConsumer(Arc::DelegationConsumerSOAP* c);
  virtual void CheckConsumers(void);
  void PeriodicCheckConsumers(void);
  std::string FindCred(const std::string& id,const std::string& client);

  bool LockCred(const std::string& lock_id, const std::list<std::string>& ids,const std::string& client);
  bool ReleaseCred(const std::string& lock_id, bool touch = false, bool remove = false);
};

} // namespace ARex

