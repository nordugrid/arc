#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/SecHandlerLoader.h>
#include <arc/loader/Loader.h>
#include <arc/Logger.h>

#include "SimpleMap.h"

#include "IdentityMap.h"

static ArcSec::SecHandler* get_sechandler(Arc::Config *cfg, Arc::ChainContext* ctx) {
    return new ArcSec::IdentityMap(cfg,ctx);
}

sechandler_descriptors ARC_SECHANDLER_LOADER = {
    { "identity.map", 0, &get_sechandler},
    { NULL, 0, NULL }
};

namespace ArcSec {

// --------------------------------------------------------------------------------------
class LocalMapDirect: public LocalMap {
 private:
  std::string id_;
 public:
  LocalMapDirect(const std::string& id):id_(id) {};
  virtual ~LocalMapDirect(void) {};
  virtual std::string ID(Arc::Message* msg) { return id_; };
};

// --------------------------------------------------------------------------------------
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

// --------------------------------------------------------------------------------------
static LocalMap* MakeLocalMap(Arc::XMLNode pdp) {
  Arc::XMLNode p;
  p=pdp["LocalName"];
  if(p) {
    std::string name = p;
    if(name.empty()) return NULL;
    return new LocalMapDirect(name);
  };
  p=pdp["LocalSimplePool"];
  if(p) {
    std::string dir = p;
    if(dir.empty()) return NULL;
    return new LocalMapPool(dir);
  };
  return NULL;
}

// --------------------------------------------------------------------------------------
IdentityMap::IdentityMap(Arc::Config *cfg,Arc::ChainContext* ctx):ArcSec::SecHandler(cfg){
  Arc::PDPFactory* pdp_factory = (Arc::PDPFactory*)(*ctx);
  if(pdp_factory) {
    Arc::XMLNode plugins = (*cfg)["Plugins"];
    for(int n = 0;;++n) {
      Arc::XMLNode p = plugins[n];
      if(!p) break;
      std::string name = p["Name"];
      if(name.empty()) continue; // Nameless plugin?
      pdp_factory->load_all_instances(name);
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
      ArcSec::PDP* pdp = pdp_factory->get_instance(name,&cfg_,NULL);
      if(!pdp) {
        logger.msg(Arc::ERROR, "PDP: %s can not be loaded", name.c_str());
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
      logger.msg(Arc::INFO,"Grid identity is mapped to local identity '%s'",id.c_str());
      msg->Attributes()->set("SEC:LOCALID",id);
      return true;  
    };
  }
  return true;
}

}

