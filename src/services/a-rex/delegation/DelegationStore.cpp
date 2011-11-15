#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fcntl.h>

#include <arc/FileUtils.h>

#include "DelegationStore.h"

namespace ARex {
  DelegationStore::DelegationStore(const std::string& base):fstore_(base) {
    // TODO: Do some cleaning on startup
  }

  Arc::DelegationConsumerSOAP* DelegationStore::AddConsumer(std::string& id,const std::string& client) {
    std::string path = fstore_.Add(id,client,std::list<std::string>());
    if(path.empty()) return NULL;
    Arc::DelegationConsumerSOAP* cs = new Arc::DelegationConsumerSOAP();
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

  static bool file_read(const std::string& path, std::string& content) {
  }

  static bool file_write(const std::string& path, std::string& content) {
  }

  Arc::DelegationConsumerSOAP* DelegationStore::FindConsumer(const std::string& id,const std::string& client) {
    std::list<std::string> meta;
    std::string path = fstore_.Find(id,client,meta);
    if(path.empty()) return NULL;
    std::string content;
    if(!Arc::FileRead(path,content)) return NULL;
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

  void DelegationStore::TouchConsumer(Arc::DelegationConsumerSOAP* c,const std::string& credentials) {
    if(!c) return;
    Glib::Mutex::Lock lock(lock_);
    std::map<Arc::DelegationConsumerSOAP*,Consumer>::iterator i = acquired_.find(c);
    if(i == acquired_.end()) return; // ????
    // TODO: store into file
    if(!credentials.empty()) {
      Arc::FileCreate(i->second.path,credentials,0,0,S_IRUSR|S_IWUSR);
    };
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

  void DelegationStore::RemoveConsumer(Arc::DelegationConsumerSOAP* c) {
    if(!c) return;
    Glib::Mutex::Lock lock(lock_);
    std::map<Arc::DelegationConsumerSOAP*,Consumer>::iterator i = acquired_.find(c);
    if(i == acquired_.end()) return; // ????
    fstore_.Remove(i->second.id,i->second.client); // TODO: Handle failure
    delete i->first;
    acquired_.erase(i);
  }

  void DelegationStore::CheckConsumers(void) {
    // TODO: Remove credentials which were not removed because of locks
  }

  std::string DelegationStore::FindCred(const std::string& id,const std::string& client) {
    std::list<std::string> meta;
    return fstore_.Find(id,client,meta);
  }

  bool DelegationStore::LockCred(const std::string& lock_id, const std::list<std::string>& ids,const std::string& client) {
    return fstore_.AddLock(lock_id,ids,client);
  }

  bool DelegationStore::ReleaseCred(const std::string& lock_id) {
    return fstore_.RemoveLock(lock_id);
  }

} // namespace ARex

