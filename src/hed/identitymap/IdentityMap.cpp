#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/message/MCCLoader.h>

#include "SimpleMap.h"

#include "IdentityMap.h"

static Arc::Plugin* get_sechandler(Arc::PluginArgument* arg) {
    ArcSec::SecHandlerPluginArgument* shcarg =
            arg?dynamic_cast<ArcSec::SecHandlerPluginArgument*>(arg):NULL;
    if(!shcarg) return NULL;
    return new ArcSec::IdentityMap((Arc::Config*)(*shcarg),(Arc::ChainContext*)(*shcarg));
}

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
    { "identity.map", "HED:SHC", 0, &get_sechandler},
    { NULL, NULL, 0, NULL }
};

namespace ArcSec {

// --------------------------------------------------------------------------
class LocalMapDirect: public LocalMap {
 private:
  std::string id_;
 public:
  LocalMapDirect(const std::string& id):id_(id) {};
  virtual ~LocalMapDirect(void) {};
  virtual std::string ID(Arc::Message*) { return id_; };
};

// --------------------------------------------------------------------------
class LocalMapPool: public LocalMap {
 private:
  std::string dir_;
 public:
  LocalMapPool(const std::string& dir);
  virtual ~LocalMapPool(void);
  virtual std::string ID(Arc::Message* msg);
};

LocalMapPool::LocalMapPool(const std::string& dir):dir_(dir) {
}

LocalMapPool::~LocalMapPool(void) {
}

std::string LocalMapPool::ID(Arc::Message* msg) {
  // Get user Grid identity.
  // So far only DN from TLS is supported.
  std::string dn = msg->Attributes()->get("TLS:IDENTITYDN");
  if(dn.empty()) return "";
  SimpleMap pool(dir_);
  if(!pool) return "";
  return pool.map(dn);
}

// --------------------------------------------------------------------------
class LocalMapList: public LocalMap {
 private:
  std::string file_;
 public:
  LocalMapList(const std::string& dir);
  virtual ~LocalMapList(void);
  virtual std::string ID(Arc::Message* msg);
};

LocalMapList::LocalMapList(const std::string& file):file_(file) {
}

LocalMapList::~LocalMapList(void) {
}

static std::string get_val(std::string& str) {
  std::string val;
  if(str[0] == '"') {
    std::string::size_type p = str.find('"',1);
    if(p == std::string::npos) return "";
    val=str.substr(1,p-1);
    str=str.substr(p+1);
    return val;
  };
  if(str[0] == '\'') {
    std::string::size_type p = str.find('\'',1);
    if(p == std::string::npos) return "";
    val=str.substr(1,p-1);
    str=str.substr(p+1);
    return val;
  };
  std::string::size_type p = str.find_first_of(" \t");
  if(p == std::string::npos) {
    val=str; str.resize(0);
  } else {
    val=str.substr(0,p); str=str.substr(p);
  };
  return val;
}

std::string LocalMapList::ID(Arc::Message* msg) {
  // Compare user Grid identity to list in file.
  // So far only DN from TLS is supported.
  std::string dn = msg->Attributes()->get("TLS:IDENTITYDN");
  if(dn.empty()) return "";
  std::ifstream f(file_.c_str());
  if(!f.is_open() ) return "";
  for(;!f.eof();) {
    std::string buf;
    std::getline(f,buf);
    buf=Arc::trim(buf);
    if(buf.empty()) continue;
    if(buf[0] == '#') continue;
    std::string val = get_val(buf);
    if(val != dn) continue;
    buf=Arc::trim(buf);
    val=get_val(buf);
    if(val.empty()) continue;
    f.close();
    return val;
  };
  f.close();
  return "";
}

// --------------------------------------------------------------------------
static LocalMap* MakeLocalMap(Arc::XMLNode pdp) {
  Arc::XMLNode p;
  p=pdp["LocalName"];
  if(p) {
    std::string name = p;
    if(name.empty()) return NULL;
    return new LocalMapDirect(name);
  };
  p=pdp["LocalList"];
  if(p) {
    std::string file = p;
    if(file.empty()) return NULL;
    return new LocalMapList(file);
  };
  p=pdp["LocalSimplePool"];
  if(p) {
    std::string dir = p;
    if(dir.empty()) return NULL;
    return new LocalMapPool(dir);
  };
  return NULL;
}

// ---------------------------------------------------------------------------
IdentityMap::IdentityMap(Arc::Config *cfg,Arc::ChainContext* ctx):ArcSec::SecHandler(cfg){
  Arc::PluginsFactory* pdp_factory = (Arc::PluginsFactory*)(*ctx);
  if(pdp_factory) {
    Arc::XMLNode plugins = (*cfg)["Plugins"];
    for(int n = 0;;++n) {
      Arc::XMLNode p = plugins[n];
      if(!p) break;
      std::string name = p["Name"];
      if(name.empty()) continue; // Nameless plugin?
      pdp_factory->load(name,PDPPluginKind);
    };
    Arc::XMLNode pdps = (*cfg)["PDP"];
    for(int n = 0;;++n) {
      Arc::XMLNode p = pdps[n];
      if(!p) break;
      std::string name = p.Attribute("name");
      if(name.empty()) continue; // Nameless?
      LocalMap* local_id = MakeLocalMap(p);
      if(!local_id) continue; // No mapping?
      Arc::Config cfg_(p);
      PDPPluginArgument arg(&cfg_);
      ArcSec::PDP* pdp = pdp_factory->GetInstance<PDP>(PDPPluginKind,name,&arg);
      if(!pdp) {
        logger.msg(Arc::ERROR, "PDP: %s can not be loaded", name);
        continue;
      };
      map_pair_t m;
      m.pdp=pdp;
      m.uid=local_id;
      maps_.push_back(m);
    };
  };
}

IdentityMap::~IdentityMap(void) {
  for(std::list<map_pair_t>::iterator p = maps_.begin();p!=maps_.end();++p) {
    if(p->pdp) delete p->pdp;
    if(p->uid) delete p->uid;
  };
}

bool IdentityMap::Handle(Arc::Message* msg){
  for(std::list<map_pair_t>::iterator p = maps_.begin();p!=maps_.end();++p) {
    if(p->pdp->isPermitted(msg)) {
      std::string id = p->uid->ID(msg);
      logger.msg(Arc::INFO,"Grid identity is mapped to local identity '%s'",id);
      msg->Attributes()->set("SEC:LOCALID",id);
      return true;  
    };
  }
  return true;
}

}

