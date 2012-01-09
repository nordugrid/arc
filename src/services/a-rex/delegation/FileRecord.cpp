#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <iostream>

#include "uid.h"

#include "FileRecord.h"

namespace ARex {

  static int dberr(const char* s, int err) {
    //if(err != 0) std::cerr<<"DB ERROR("<<s<<"): "<<err<<std::endl;
    return err;
  }

  FileRecord::FileRecord(const std::string& base):
      basepath_(base),
      db_rec_(NULL,DB_CXX_NO_EXCEPTIONS),
      db_lock_(NULL,DB_CXX_NO_EXCEPTIONS),
      db_locked_(NULL,DB_CXX_NO_EXCEPTIONS),
      db_link_(NULL,DB_CXX_NO_EXCEPTIONS),
      valid_(false) {
    if(dberr("set 1",db_lock_.set_flags(DB_DUPSORT)) != 0) return;
    if(dberr("set 2",db_locked_.set_flags(DB_DUPSORT)) != 0) return;
    if(dberr("assoc1",db_link_.associate(NULL,&db_lock_,&locked_callback,0)) != 0) return;
    if(dberr("assoc2",db_link_.associate(NULL,&db_locked_,&lock_callback,0)) != 0) return;
    if(dberr("open 1",db_rec_.open(NULL,(basepath_+"/list").c_str(), "meta", DB_BTREE, DB_CREATE, S_IRUSR | S_IWUSR)) != 0) return;
    if(dberr("open 2",db_link_.open(NULL,(basepath_+"/list").c_str(), "link", DB_RECNO, DB_CREATE, S_IRUSR | S_IWUSR)) != 0) return;
    if(dberr("open 2",db_lock_.open(NULL,(basepath_+"/list").c_str(), "lock", DB_BTREE, DB_CREATE, S_IRUSR | S_IWUSR)) != 0) return;
    if(dberr("open 3",db_locked_.open(NULL,(basepath_+"/list").c_str(), "locked", DB_BTREE, DB_CREATE, S_IRUSR | S_IWUSR)) != 0) return;
    //if(db_rec_.associate(NULL,&db_uid_,&uid_callback,0) != 0) return;
    //if(db_rec_.associate(NULL,&db_id_,&id_callback,0) != 0) return;
    valid_ = true;
  }

  static void* store_string(const std::string& str, void* buf) {
    uint32_t l = str.length();
    unsigned char* p = (unsigned char*)buf;
    *p = (unsigned char)l; l >>= 8; ++p;
    *p = (unsigned char)l; l >>= 8; ++p;
    *p = (unsigned char)l; l >>= 8; ++p;
    *p = (unsigned char)l; l >>= 8; ++p;
    ::memcpy(p,str.c_str(),str.length());
    p += str.length();
    return (void*)p;
  }

  static void* parse_string(std::string& str, const void* buf, uint32_t& size) {
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
  }

  static void make_string(const std::string& str, Dbt& rec) {
    rec.set_data(NULL); rec.set_size(0);
    uint32_t l = 4 + str.length();
    void* d = (void*)::malloc(l);
    if(!d) return;
    rec.set_data(d); rec.set_size(l);
    d = store_string(str,d);
  }

  static void make_link(const std::string& lock_id,const std::string& id, const std::string& owner, Dbt& rec) {
    rec.set_data(NULL); rec.set_size(0);
    uint32_t l = 4 + lock_id.length() + 4 + id.length() + 4 + owner.length();
    void* d = (void*)::malloc(l);
    if(!d) return;
    rec.set_data(d); rec.set_size(l);
    d = store_string(lock_id,d);
    d = store_string(id,d);
    d = store_string(owner,d);
  }

  static void make_key(const std::string& id, const std::string& owner, Dbt& key) {
    key.set_data(NULL); key.set_size(0);
    uint32_t l = 4 + id.length() + 4 + owner.length();
    void* d = (void*)::malloc(l);
    if(!d) return;
    key.set_data(d); key.set_size(l);
    d = store_string(id,d);
    d = store_string(owner,d);
  }
 
  static void make_record(const std::string& uid, const std::string& id, const std::string& owner, const std::list<std::string>& meta, Dbt& key, Dbt& data) {
    key.set_data(NULL); key.set_size(0);
    data.set_data(NULL); data.set_size(0);
    uint32_t l = 4 + uid.length();
    for(std::list<std::string>::const_iterator m = meta.begin(); m != meta.end(); ++m) {
      l += 4 + m->length();
    };
    make_key(id,owner,key);
    void* d = (void*)::malloc(l);
    if(!d) {
      ::free(key.get_data());
      key.set_data(NULL); key.set_size(0);
      return;
    };
    data.set_data(d); data.set_size(l);
    d = store_string(uid,d);
    for(std::list<std::string>::const_iterator m = meta.begin(); m != meta.end(); ++m) {
      d = store_string(*m,d);
    };
  }
 
  static void parse_record(std::string& uid, std::string& id, std::string& owner, std::list<std::string>& meta, const Dbt& key, const Dbt& data) {
    uint32_t size = 0;
    void* d = NULL;

    d = (void*)key.get_data();
    size = (uint32_t)key.get_size();
    d = parse_string(id,d,size);
    d = parse_string(owner,d,size);

    d = (void*)data.get_data();
    size = (uint32_t)data.get_size();
    d = parse_string(uid,d,size);
    for(;size;) {
      std::string s;
      d = parse_string(s,d,size);
      meta.push_back(s);
    };
  }

  std::string FileRecord::uid_to_path(const std::string& uid) {
    std::string path = basepath_;
    std::string::size_type p = 0;
    for(;uid.length() > (p+4);) {
      path = path + G_DIR_SEPARATOR_S + uid.substr(p,3);
      p += 3;
    };
    return path + G_DIR_SEPARATOR_S + uid.substr(p);
  }

  int FileRecord::locked_callback(Db * secondary, const Dbt * key, const Dbt * data, Dbt * result) {
    const void* p = data->get_data();
    uint32_t size = data->get_size();
    std::string str;
    p = parse_string(str,p,size);
    result->set_data((void*)p);
    result->set_size(size);
    return 0;
  }

  int FileRecord::lock_callback(Db * secondary, const Dbt * key, const Dbt * data, Dbt * result) {
    const void* p = data->get_data();
    uint32_t size = data->get_size();
    uint32_t rest = size;
    std::string str;
    parse_string(str,p,rest);
    result->set_data((void*)p);
    result->set_size(size-rest);
    return 0;
  }

  std::string FileRecord::Add(std::string& id, const std::string& owner, const std::list<std::string>& meta) {
    if(!valid_) return "";
    Glib::Mutex::Lock lock(lock_);
    Dbt key;
    Dbt data;
    std::string uid = rand_uid64().substr(4);
    make_record(uid,(id.empty())?uid:id,owner,meta,key,data);
    void* pkey = key.get_data();
    void* pdata = data.get_data();
    if(db_rec_.put(NULL,&key,&data,DB_NOOVERWRITE) != 0) {
      ::free(pkey); ::free(pdata);
      return "";
    };
    db_rec_.sync(0);
    ::free(pkey); ::free(pdata);
    if(id.empty()) id = uid;
    return uid_to_path(uid);
  }

  std::string FileRecord::Find(const std::string& id, const std::string& owner, std::list<std::string>& meta) {
    if(!valid_) return "";
    Glib::Mutex::Lock lock(lock_);
    Dbt key;
    Dbt data;
    make_key(id,owner,key);
    void* pkey = key.get_data();
    if(dberr("find:get",db_rec_.get(NULL,&key,&data,0)) != 0) {
      ::free(pkey);
      return "";
    };
    std::string uid;
    std::string id_tmp;
    std::string owner_tmp;
    parse_record(uid,id_tmp,owner_tmp,meta,key,data);
    ::free(pkey);
    return uid_to_path(uid);
  }

  bool FileRecord::Modify(const std::string& id, const std::string& owner, const std::list<std::string>& meta) {
    if(!valid_) return false;
    Glib::Mutex::Lock lock(lock_);
    Dbt key;
    Dbt data;
    make_key(id,owner,key);
    void* pkey = key.get_data();
    if(dberr("modify:get",db_rec_.get(NULL,&key,&data,0)) != 0) {
      ::free(pkey);
      return false;
    };
    std::string uid;
    std::string id_tmp;
    std::string owner_tmp;
    std::list<std::string> meta_tmp;
    parse_record(uid,id_tmp,owner_tmp,meta_tmp,key,data);
    ::free(pkey);
    make_record(uid,id,owner,meta,key,data);
    if(dberr("modify.put",db_rec_.put(NULL,&key,&data,0)) != 0) {
      ::free(key.get_data());
      ::free(data.get_data());
      return false;
    };
    db_rec_.sync(0);
    ::free(key.get_data());
    ::free(data.get_data());
    return true;
  }

  bool FileRecord::Remove(const std::string& id, const std::string& owner) {
    if(!valid_) return false;
    Glib::Mutex::Lock lock(lock_);
    Dbt key;
    Dbt data;
    make_key(id,owner,key);
    void* pkey = key.get_data();
    if(dberr("remove:get1",db_locked_.get(NULL,&key,&data,0)) == 0) {
      ::free(pkey);
      return false; // have locks
    };
    if(dberr("remove:get2",db_rec_.get(NULL,&key,&data,0)) != 0) {
      ::free(pkey);
      return ""; // No such record?
    };
    std::string uid;
    std::string id_tmp;
    std::string owner_tmp;
    std::list<std::string> meta;
    parse_record(uid,id_tmp,owner_tmp,meta,key,data);
    if(!uid.empty()) {
      ::unlink(uid_to_path(uid).c_str()); // TODO: handle error
    };
    if(db_rec_.del(NULL,&key,0) != 0) {
      // TODO: handle error
      ::free(pkey);
      return false;
    };
    db_rec_.sync(0);
    ::free(pkey);
    return true;
  }

  bool FileRecord::AddLock(const std::string& lock_id, const std::list<std::string>& ids, const std::string& owner) {
    if(!valid_) return false;
    Glib::Mutex::Lock lock(lock_);
    Dbt key;
    Dbt data;
    for(std::list<std::string>::const_iterator id = ids.begin(); id != ids.end(); ++id) {
      make_link(lock_id,*id,owner,data);
      void* pdata = data.get_data();
      if(dberr("addlock:put",db_link_.put(NULL,&key,&data,DB_APPEND)) != 0) {
        ::free(pdata);
        return false;
      };
      db_link_.sync(0);
      ::free(pdata);
    };
    return true;
  }

  bool FileRecord::RemoveLock(const std::string& lock_id) {
    std::list<std::pair<std::string,std::string> > ids;
    return RemoveLock(lock_id,ids);
  }

  bool FileRecord::RemoveLock(const std::string& lock_id, std::list<std::pair<std::string,std::string> >& ids) {
    if(!valid_) return false;
    Glib::Mutex::Lock lock(lock_);
    Dbc* cur = NULL;
    if(dberr("removelock:cursor",db_lock_.cursor(NULL,&cur,0)) != 0) return false;
    Dbt key;
    Dbt data;
    make_string(lock_id,key);
    void* pkey = key.get_data();
    if(dberr("removelock:get1",cur->get(&key,&data,DB_SET)) != 0) { // TODO: handle errors
      ::free(pkey);
      cur->close(); return false;
    };
    for(;;) {
      std::string id;
      std::string owner;
      uint32_t size = data.get_size();
      void* buf = data.get_data();
      buf = parse_string(id,buf,size);
      buf = parse_string(owner,buf,size);
      ids.push_back(std::pair<std::string,std::string>(id,owner));
      if(dberr("removelock:del",cur->del(0)) != 0) {
        ::free(pkey);
        cur->close(); return false;
      };
      db_lock_.sync(0);
      if(dberr("removelock:get2",cur->get(&key,&data,DB_NEXT_DUP)) != 0) break;
    };
    ::free(pkey);
    cur->close();
    return true;
  }

  bool FileRecord::ListLocks(std::list<std::string>& locks) {
    if(!valid_) return false;
    Glib::Mutex::Lock lock(lock_);
    Dbc* cur = NULL;
    if(db_lock_.cursor(NULL,&cur,0)) return false;
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
    return true;
  }

  FileRecord::Iterator::Iterator(FileRecord& frec):frec_(frec),cur_(NULL) {
    if(dberr("Iterator:cursor",frec_.db_rec_.cursor(NULL,&cur_,0)) != 0) {
      if(cur_) {
        cur_->close(); cur_=NULL;
      };
      return;
    };
    Dbt key;
    Dbt data;
    if(dberr("Iterator:first",cur_->get(&key,&data,DB_FIRST)) != 0) {
      cur_->close(); cur_=NULL;
      return;
    };
    parse_record(uid_,id_,owner_,meta_,key,data);
  }

  FileRecord::Iterator::~Iterator(void) {
    if(cur_) {
      cur_->close(); cur_=NULL;
    };
  }

  FileRecord::Iterator& FileRecord::Iterator::operator++(void) {
    if(!cur_) return *this;
    Dbt key;
    Dbt data;
    if(dberr("Iterator:first",cur_->get(&key,&data,DB_NEXT)) != 0) {
      cur_->close(); cur_=NULL;
      return *this;
    };
    parse_record(uid_,id_,owner_,meta_,key,data);
    return *this;
  }

  FileRecord::Iterator& FileRecord::Iterator::operator--(void) {
    if(!cur_) return *this;
    Dbt key;
    Dbt data;
    if(dberr("Iterator:first",cur_->get(&key,&data,DB_PREV)) != 0) {
      cur_->close(); cur_=NULL;
      return *this;
    };
    parse_record(uid_,id_,owner_,meta_,key,data);
    return *this;
  }

} // namespace ARex

