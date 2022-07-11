#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/Utils.h>

#include "MCCLoader.h"

namespace Arc {

  MCCLoader::MCCLoader(Config& cfg):Loader(cfg),valid_(false) {
    context_ = new ChainContext(*this);
    valid_ = make_elements(cfg);
    if(!valid_)
      MCCLoader::logger.msg(VERBOSE, "Chain(s) configuration failed");
  }

  MCCLoader::~MCCLoader(void) {
    // TODO: stop any processing on those MCCs or mark them for
    // self-destruction or break links first or use semaphors in
    // MCC destructors
    // Unlink all objects
    for(mcc_container_t::iterator mcc_i = mccs_unlinked_.begin();
        mcc_i != mccs_unlinked_.end(); ++mcc_i) {
      MCC* mcc = mcc_i->second;
      if(mcc) mcc->Unlink();
    }
    for(mcc_container_t::iterator mcc_i = mccs_.begin();
        mcc_i != mccs_.end(); ++mcc_i) {
      MCC* mcc = mcc_i->second;
      if(mcc) mcc->Unlink();
    }
    for(plexer_container_t::iterator plexer_i = plexers_.begin();
        plexer_i != plexers_.end(); ++plexer_i) {
      Plexer* plexer = plexer_i->second;
      if(plexer) plexer->Unlink();
    }
    // Destroy all objects
    // First unlinked MCC are destroyed because they handle spawned threads
    // processing request. After they are destroyed there should be no 
    // processing left.
    for(mcc_container_t::iterator mcc_i = mccs_unlinked_.begin();
        mcc_i != mccs_unlinked_.end(); mcc_i = mccs_unlinked_.begin()) {
      MCC* mcc = mcc_i->second;
      mccs_unlinked_.erase(mcc_i);
      if(mcc) delete mcc;
    }
    // Then ordinary MCCs and other objects
    for(mcc_container_t::iterator mcc_i = mccs_.begin();
        mcc_i != mccs_.end(); mcc_i = mccs_.begin()) {
      MCC* mcc = mcc_i->second;
      mccs_.erase(mcc_i);
      if(mcc) delete mcc;
    }
    for(plexer_container_t::iterator plexer_i = plexers_.begin();
        plexer_i != plexers_.end(); plexer_i = plexers_.begin()) {
      Plexer* plexer = plexer_i->second;
      plexers_.erase(plexer_i);
      if(plexer) delete plexer;
    }
    for(service_container_t::iterator service_i = services_.begin();
        service_i != services_.end(); service_i = services_.begin()) {
      Service* service = service_i->second;
      services_.erase(service_i);
      if(service) delete service;
    }
    // Last are SecHandlers because now there are no objects which 
    // could use them.
    for(sechandler_container_t::iterator sechandler_i = sechandlers_.begin();
        sechandler_i != sechandlers_.end();
        sechandler_i = sechandlers_.begin()) {
      ArcSec::SecHandler* sechandler = sechandler_i->second;
      sechandlers_.erase(sechandler_i);
      if(sechandler) delete sechandler;
    }

    if(context_) delete context_;
  }

  class mcc_connector_t {
  public:
    MCCLoader::mcc_container_t::iterator mcc;
    std::string name;
    std::map<std::string, std::string> nexts;
    mcc_connector_t(MCCLoader::mcc_container_t::iterator mcc) : mcc(mcc) {};
  };

  std::ostream& operator<<(std::ostream& o, const mcc_connector_t& mcc) {
    o << mcc.name;
    o << "(" << mcc.mcc->first << ")";
    return o;
  }

  class plexer_connector_t {
  public:
    MCCLoader::plexer_container_t::iterator plexer;
    std::map<std::string, std::string> nexts;
    plexer_connector_t(MCCLoader::plexer_container_t::iterator plexer) :
      plexer(plexer) {};
  };

  class mcc_connectors_t : public std::list<mcc_connector_t> {};
  class plexer_connectors_t : public std::list<plexer_connector_t> {};

  static XMLNode FindElementByID(XMLNode node, const std::string& id, const std::string& name) {
    for(int n = 0;; ++n) {
      XMLNode cn = node.Child(n);
      if(!cn) break;

      if(MatchXMLName(cn, "ArcConfig")) {
        XMLNode result = FindElementByID(cn, id, name);
        if(result) return result;
        continue;
      }

      if(MatchXMLName(cn, "Chain")) {
        XMLNode result = FindElementByID(cn, id, name);
        if(result) return result;
      continue;
      }

      if(MatchXMLName(cn, name)) {
        if((std::string)(cn.Attribute("id")) == id) return cn;
      }
    }
    return XMLNode();
  }

  ArcSec::SecHandler* MCCLoader::make_sec_handler(Config& cfg, XMLNode& node) {
    if(!node) {
      // Normally should not happen
      MCCLoader::logger.msg(ERROR, "SecHandler configuration is not defined");
      error_description_ = "Security plugin configuration is not defined";
      return NULL;
    }
    XMLNode desc_node;
    std::string refid = node.Attribute("refid");
    if(refid.empty()) {
      desc_node = node;
      refid = (std::string)node.Attribute("id");
      if(refid.empty()) {
        refid = "__arc_sechandler_" + tostring(sechandlers_.size()) + "__";
      }
    }
    else {
      // Maybe it's already created
      MCCLoader::sechandler_container_t::iterator phandler = sechandlers_.find(refid);
      if(phandler != sechandlers_.end()) {
        return phandler->second;
      }
      // Look for it's configuration
      desc_node = FindElementByID(cfg, refid, "SecHandler");
    }
    if(!desc_node) {
      MCCLoader::logger.msg(ERROR, "SecHandler has no configuration");
      error_description_ = "Security plugin configuration is not defined";
      return NULL;
    }
    std::string name = desc_node.Attribute("name");
    if(name.empty()) {
      MCCLoader::logger.msg(ERROR, "SecHandler has no name attribute defined");
      error_description_ = "Security plugin has no name defined";
      return NULL;
    }
    // Create new security handler
    Config cfg_(desc_node, cfg.getFileName());
    ArcSec::SecHandlerPluginArgument arg(&cfg_,context_);
    Plugin* plugin = factory_->get_instance(SecHandlerPluginKind, name, &arg);
    ArcSec::SecHandler* sechandler = plugin?dynamic_cast<ArcSec::SecHandler*>(plugin):NULL;
    if(!sechandler) {
      Loader::logger.msg(ERROR, "Security Handler %s(%s) could not be created", name, refid);
      error_description_ = "Security plugin component "+name+" could not be created";
      // TODO: need a way to propagate error description from factory.
    } else {
      Loader::logger.msg(INFO, "SecHandler: %s(%s)", name, refid);
      sechandlers_[refid] = sechandler;
    }
    return sechandler;
  }

  MCC* MCCLoader::make_component(Config& cfg, XMLNode cn, mcc_connectors_t *mcc_connectors) {
    Config cfg_(cn, cfg.getFileName());
    std::string name = cn.Attribute("name");
    std::string id = cn.Attribute("id");
    if(name.empty()) {
      error_description_ = "Component "+id+" has no name defined";
      logger.msg(ERROR, "Component has no name attribute defined");
      return NULL;
    }
    if(id.empty()) {
      error_description_ = "Component "+name+" has no id defined";
      logger.msg(ERROR, "Component has no ID attribute defined");
      return NULL;
    }
    MCCPluginArgument arg(&cfg_,context_);
    AutoPointer<Plugin> plugin(factory_->get_instance(MCCPluginKind ,name, &arg));
    MCC* mcc = dynamic_cast<MCC*>(plugin.Ptr());
    if(!mcc) {
      error_description_ = "Component "+name+" could not be created";
      // TODO: need a way to propagate error description from factory.
      logger.msg(VERBOSE, "Component %s(%s) could not be created", name, id);
      return NULL;
    }

    // Configure security plugins
    XMLNode an = cn["SecHandler"];
    for(int n = 0;; ++n) {
      XMLNode can = an[n];
      if(!can) break;
      ArcSec::SecHandler* sechandler = make_sec_handler(cfg, can);
      if(!sechandler) {
        return NULL;
      };
      std::string event = can.Attribute("event");
      mcc->AddSecHandler(&cfg_, sechandler, event);
    }

    mcc_container_t::iterator mccp = mccs_.find(id);
    mcc_connector_t mcc_connector(mccp);
    if(mcc_connectors) {
      // Add to chain list
      for(int nn = 0;; ++nn) {
        XMLNode cnn = cn["next"][nn];
        if(!cnn) break;
        std::string nid = cnn.Attribute("id");
        if(nid.empty()) {
          logger.msg(ERROR, "Component's %s(%s) next has no ID "
                            "attribute defined", name, id);
          error_description_ = "Component "+name+" has no id defined in next target";
          return NULL;
        }
        std::string label = cnn;
        mcc_connector.nexts[label] = nid;
      }
      mcc_connector.name = name;
    }

    if(mccp == mccs_.end()) {
      mccp = mccs_.insert(std::make_pair(id,(MCC*)NULL)).first;
      mcc_connector.mcc = mccp;
    }

    delete mccp->second;
    mccp->second = mcc;
    plugin.Release();

    if(mcc_connectors) mcc_connectors->push_back(mcc_connector);

    std::string entry = cn.Attribute("entry");
    if(!entry.empty()) mccs_exposed_[entry] = mccp->second;

    return mccp->second;
  }

  bool MCCLoader::make_elements(Config& cfg, int level,
                                mcc_connectors_t *mcc_connectors,
                                plexer_connectors_t *plexer_connectors) {
    bool success = true;
    if(mcc_connectors == NULL) mcc_connectors = new mcc_connectors_t;
    if(plexer_connectors == NULL) plexer_connectors = new plexer_connectors_t;
    // 1st stage - creating all elements.
    // Configuration is parsed recursively - going deeper at ArcConfig
    // and Chain elements
    for(int n = 0;; ++n) {
      XMLNode cn = cfg.Child(n);
      if(!cn) break;
      Config cfg_(cn, cfg.getFileName());

      if(MatchXMLName(cn, "ArcConfig")) {
        if(!make_elements(cfg_, level + 1, mcc_connectors, plexer_connectors))
          success = false;
        continue;
      }

      if(MatchXMLName(cn, "Chain")) {
        if(!make_elements(cfg_, level + 1, mcc_connectors, plexer_connectors))
          success = false;
        continue;
      }

      if(MatchXMLName(cn, "Component")) {
        // Create new MCC
        MCC* mcc = make_component(cfg,cn,mcc_connectors);
        if(!mcc) {
          success = false;
          continue;
        }
        logger.msg(DEBUG, "Loaded MCC %s(%s)",
                   (std::string)cn.Attribute("name"),
                   (std::string)cn.Attribute("id"));
        continue;
      }

      if(MatchXMLName(cn, "Plexer")) {
        std::string id = cn.Attribute("id");
        if(id.empty()) id = "plexer";
        MCCPluginArgument arg(&cfg_,context_);
        Plexer* plexer = new Plexer(&cfg_,&arg);
        plexers_[id] = plexer;
        plexer_connector_t plexer_connector(plexers_.find(id));
        for(int nn = 0;; ++nn) {
          XMLNode cnn = cn["next"][nn];
          if(!cnn) break;
          std::string nid = cnn.Attribute("id");
          if(nid.empty()) {
            logger.msg(ERROR, "Plexer's (%s) next has no ID "
                 "attribute defined", id);
            error_description_ = "MCC chain is broken in plexer "+id+" - no id of next component defined";
            success = false;
            continue;
          }
          std::string label = cnn;
          plexer_connector.nexts[label] = nid;
        }
        plexer_connectors->push_back(plexer_connector);
        logger.msg(INFO, "Loaded Plexer %s", id);
        continue;
      }

      if(MatchXMLName(cn, "Service")) {
        std::string name = cn.Attribute("name");
        std::string id = cn.Attribute("id");
        if(name.empty()) {
          logger.msg(ERROR, "Service has no Name attribute defined");
          error_description_ = "MCC chain is broken in service "+id+" - no name defined";
          success = false;
          continue;
        }
        if(id.empty()) {
          logger.msg(ERROR, "Service has no ID attribute defined");
          error_description_ = "MCC chain is broken in service "+name+" - no id defined";
          success = false;
          continue;
        }
        ServicePluginArgument arg(&cfg_,context_);
        Plugin* plugin = factory_->get_instance(ServicePluginKind, name, &arg);
        Service* service = plugin?dynamic_cast<Service*>(plugin):NULL;
        if(!service) {
          logger.msg(ERROR, "Service %s(%s) could not be created", name, id);
          error_description_ = "Service component "+name+" could not be created";
          // TODO: need a way to propagate error description from factory.
          success = false;
          continue;
        }
        services_[id] = service;
        logger.msg(INFO, "Loaded Service %s(%s)", name, id);

        // Configure security plugins
        XMLNode an;

        an = cn["SecHandler"];
        for(int n = 0;; ++n) {
          XMLNode can = an[n];
          if(!can) break;
          ArcSec::SecHandler* sechandler = make_sec_handler(cfg, can);
          if(!sechandler) {
            success = false;
            continue;
          }
          std::string event = can.Attribute("event");
          service->AddSecHandler(&cfg_, sechandler, event);
        }
        continue;
      }

      // Configuration processing is split to multiple functions - hence
      // ignoring all unknown elements.
      //logger.msg(WARNING, "Unknown element \"%s\" - ignoring", cn.Name());
    }

    if(level != 0) return success;

    // 2nd stage - making links between elements.

    // Making links from MCCs
    mccs_unlinked_ = mccs_;
    for(mcc_connectors_t::iterator mcc = mcc_connectors->begin();
        mcc != mcc_connectors->end(); ++mcc) {
      for(std::map<std::string, std::string>::iterator next =
          mcc->nexts.begin();
          next != mcc->nexts.end(); next = mcc->nexts.begin()) {
        std::string label = next->first;
        std::string id = next->second;
        mcc_container_t::iterator mcc_l = mccs_.find(id);
        if(mcc_l != mccs_.end()) {
          // Make link MCC->MCC
          mcc->mcc->second->Next(mcc_l->second, label);
          logger.msg(DEBUG, "Linking MCC %s(%s) to MCC (%s) under %s",
               mcc->name, mcc->mcc->first, id, label);
          mcc->nexts.erase(next);
          mcc_container_t::iterator mcc_ul = mccs_unlinked_.find(id);
          if(mcc_ul != mccs_unlinked_.end()) mccs_unlinked_.erase(mcc_ul);
          continue;
        }
        service_container_t::iterator service_l = services_.find(id);
        if(service_l != services_.end()) {
          // Make link MCC->Service
          mcc->mcc->second->Next(service_l->second, label);
          logger.msg(INFO, "Linking MCC %s(%s) to Service (%s) under %s",
               mcc->name, mcc->mcc->first, id, label);
          mcc->nexts.erase(next);
          continue;
        }
        plexer_container_t::iterator plexer_l = plexers_.find(id);
        if(plexer_l != plexers_.end()) {
          // Make link MCC->Plexer
          mcc->mcc->second->Next(plexer_l->second, label);
          logger.msg(INFO, "Linking MCC %s(%s) to Plexer (%s) under %s",
               mcc->name, mcc->mcc->first, id, label);
          mcc->nexts.erase(next);
          continue;
        }
        logger.msg(VERBOSE, "MCC %s(%s) - next %s(%s) has no target",
                   mcc->name, mcc->mcc->first, label, id);
        error_description_ = "MCC chain is broken - no "+id+" target was found for "+mcc->name+" component";
        success = false;
        mcc->nexts.erase(next);
      }
    }
    // Making links from Plexers
    for(plexer_connectors_t::iterator plexer = plexer_connectors->begin();
        plexer != plexer_connectors->end(); ++plexer) {
      for(std::map<std::string, std::string>::iterator next =
          plexer->nexts.begin();
          next != plexer->nexts.end(); next = plexer->nexts.begin()) {
        std::string label = next->first;
        std::string id = next->second;
        mcc_container_t::iterator mcc_l = mccs_.find(id);
        if(mcc_l != mccs_.end()) {
          // Make link Plexer->MCC
          plexer->plexer->second->Next(mcc_l->second, label);
          logger.msg(INFO, "Linking Plexer %s to MCC (%s) under %s",
               plexer->plexer->first, id, label);
          plexer->nexts.erase(next);
          mcc_container_t::iterator mcc_ul = mccs_unlinked_.find(id);
          if(mcc_ul != mccs_unlinked_.end()) mccs_unlinked_.erase(mcc_ul);
          continue;
        }
        service_container_t::iterator service_l = services_.find(id);
        if(service_l != services_.end()) {
          // Make link Plexer->Service
          plexer->plexer->second->Next(service_l->second, label);
          logger.msg(INFO, "Linking Plexer %s to Service (%s) under %s",
               plexer->plexer->first, id, label);
          plexer->nexts.erase(next);
          continue;
        }
        plexer_container_t::iterator plexer_l = plexers_.find(id);
        if(plexer_l != plexers_.end()) {
          // Make link Plexer->Plexer
          plexer->plexer->second->Next(plexer_l->second, label);
          logger.msg(INFO, "Linking Plexer %s to Plexer (%s) under %s",
               plexer->plexer->first, id, label);
          plexer->nexts.erase(next);
          continue;
        }

        logger.msg(ERROR, "Plexer (%s) - next %s(%s) has no target",
                   plexer->plexer->first, label, id);
        error_description_ = "MCC chain is broken - no "+id+" target was found for "+plexer->plexer->first+" plexer";
        success = false;
        plexer->nexts.erase(next);
      }
    }
    if(mcc_connectors) delete mcc_connectors;
    if(plexer_connectors) delete plexer_connectors;
    // Move all unlinked MCCs to dedicated container
    for(mcc_container_t::iterator mcc = mccs_unlinked_.begin();
                                  mcc != mccs_unlinked_.end();++mcc) {
      mcc_container_t::iterator mcc_l = mccs_.find(mcc->first);
      if(mcc_l != mccs_.end()) mccs_.erase(mcc_l);
    }
    return success;
  }

  bool MCCLoader::ReloadElement(Config& cfg) {
    XMLNode cn = cfg;
    if(MatchXMLName(cn, "Component")) {
      std::string id = cn.Attribute("id");
      if(id.empty()) return false;
      // Look for element to replace
      mcc_container_t::iterator mccp = mccs_.find(id);
      if(mccp == mccs_.end()) return false;
      MCC* oldmcc = mccp->second;
      // Replace element
      MCC* mcc = make_component(cfg,cn,NULL);
      if(!mcc) return false;
      // Replace references to old element

      // Create 'next' references

      // Remove old element and all references
      for(mccp = mccs_exposed_.begin();mccp != mccs_exposed_.end();) {
        if(mccp->second == oldmcc) {
          mccs_exposed_.erase(mccp);
          mccp = mccs_exposed_.begin();
        } else ++mccp;
      }
      if(oldmcc) delete oldmcc;
      return true;
    } else if(MatchXMLName(cn, "Service")) {


      return false;
    } else if(MatchXMLName(cn, "Plexer")) {


      return false;
    }
    return false;
  }

  MCC* MCCLoader::operator[](const std::string& id) {
    mcc_container_t::iterator mcc = mccs_exposed_.find(id);
    if(mcc != mccs_exposed_.end()) return mcc->second;
    return NULL;
  }

} // namespace Arc
