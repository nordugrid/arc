#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utime.h>
#include <time.h>
#include <unistd.h>

#include <arc/FileUtils.h>

#include "DelegationStore.h"

namespace ARex {
  static void make_dir_for_file(std::string dpath) {
    std::string::size_type p = dpath.rfind(G_DIR_SEPARATOR_S);
    if(p == std::string::npos) return;
    if(p == 0) return;
    dpath.resize(p);
    Arc::DirCreate(dpath,0,0,S_IRUSR|S_IWUSR,true);
  }

  DelegationStore::DelegationStore(const std::string& base):fstore_(base) {
    expiration_ = 0;
    maxrecords_ = 0;
    mtimeout_ = 0;
    mrec_ = NULL;
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
      make_dir_for_file(i->second.path);
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
        make_dir_for_file(i->second.path);
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
    // Go through stored credentials
    // Remove outdated records (those with locks won't be removed)
    time_t start = ::time(NULL);
    if(expiration_) {
      if(mrec_ == NULL) mrec_ = new FileRecord::Iterator(fstore_);
      for(;(bool)(*mrec_);++(*mrec_)) {
        if(mtimeout_ && (((unsigned int)(::time(NULL) - start)) > mtimeout_)) return;
        struct stat st;
        if(::stat(mrec_->path().c_str(),&st) == 0) {
          if(((unsigned int)(::time(NULL) - st.st_mtime)) > expiration_) {
            fstore_.Remove(mrec_->id(),mrec_->owner());
          };    
        };
      };
      delete mrec_; mrec_ = NULL;
    };
    // TODO: Remove records over threshold
  }

  std::string DelegationStore::FindCred(const std::string& id,const std::string& client) {
    std::list<std::string> meta;
    return fstore_.Find(id,client,meta);
  }

  bool DelegationStore::LockCred(const std::string& lock_id, const std::list<std::string>& ids,const std::string& client) {
    return fstore_.AddLock(lock_id,ids,client);
  }

  bool DelegationStore::ReleaseCred(const std::string& lock_id, bool touch, bool remove) {
    if((!touch) && (!remove)) return fstore_.RemoveLock(lock_id);
    std::list<std::pair<std::string,std::string> > ids;
    if(!fstore_.RemoveLock(lock_id,ids)) return false;
    for(std::list<std::pair<std::string,std::string> >::iterator i = ids.begin();
                        i != ids.end(); ++i) {    
      if(touch) {
        std::list<std::string> meta;
        std::string path = fstore_.Find(i->first,i->second,meta);
        // TODO: in a future use meta for storing times
        if(!path.empty()) ::utime(path.c_str(),NULL);
      };
      if(remove) fstore_.Remove(i->first,i->second);
    };
  }

} // namespace ARex

