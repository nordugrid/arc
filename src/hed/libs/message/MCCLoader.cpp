#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/StringConv.h>

#include "MCCLoader.h"

namespace Arc {

  MCCLoader::MCCLoader(Config& cfg):Loader(cfg) {
    context_ = new ChainContext(*this);
    make_elements(cfg);
  }

  MCCLoader::~MCCLoader(void) {
    // TODO: stop any processing on those MCCs or mark them for
    // self-destruction or break links first or use semaphors in
    // MCC destructors
    // Unlink all objects
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
    for(mcc_container_t::iterator mcc_i = mccs_.begin();
	mcc_i != mccs_.end(); mcc_i = mccs_.begin()) {
      MCC* mcc = mcc_i->second;
      mccs_.erase(mcc_i);
      if(mcc) delete mcc;
    }
    for(service_container_t::iterator service_i = services_.begin();
	service_i != services_.end(); service_i = services_.begin()) {
      Service* service = service_i->second;
      services_.erase(service_i);
      if(service) delete service;
    }
    for(plexer_container_t::iterator plexer_i = plexers_.begin();
        plexer_i != plexers_.end(); plexer_i = plexers_.begin()) {
      Plexer* plexer = plexer_i->second;
      plexers_.erase(plexer_i);
      if(plexer) delete plexer;
    }
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
    std::string name;
    std::map<std::string, std::string> nexts;
    plexer_connector_t(MCCLoader::plexer_container_t::iterator plexer) :
      plexer(plexer) {};
  };

  std::ostream& operator<<(std::ostream& o, const plexer_connector_t& plexer) {
    o << plexer.name;
    o << "(" << plexer.plexer->first << ")";
    return o;
  }

  class mcc_connectors_t : public std::list<mcc_connector_t> {};
  class plexer_connectors_t : public std::list<plexer_connector_t> {};

  static XMLNode FindElementByID(XMLNode node, const std::string &fn, const std::string& id, const std::string& name) {
    for(int n = 0;; ++n) {
      XMLNode cn = node.Child(n);
      if(!cn) break;
      Config cfg_(cn, fn);

      if(MatchXMLName(cn, "ArcConfig")) {
        XMLNode result = FindElementByID(cn, fn, id, name);
        if(result) return result;
        continue;
      }

      if(MatchXMLName(cn, "Chain")) {
        XMLNode result = FindElementByID(cn, fn, id, name);
        if(result) return result;
	    continue;
      }

      if(MatchXMLName(cn, name)) {
	    if((std::string)(cn.Attribute("id")) == id) return cn;
      }
    }
    return XMLNode();
  }

  static ArcSec::SecHandler* MakeSecHandler(Config& cfg, ChainContext *ctx,
      MCCLoader::sechandler_container_t& sechandlers,
      PluginsFactory *factory, XMLNode& node) {
    if(!node) return NULL;
    XMLNode desc_node;
    std::string refid = node.Attribute("refid");
    if(refid.empty()) {
      desc_node = node;
      refid = (std::string)node.Attribute("id");
      if(refid.empty()) {
	    refid = "__arc_sechandler_" + tostring(sechandlers.size()) + "__";
      }
    }
    else {
      // Maybe it's already created
      MCCLoader::sechandler_container_t::iterator phandler =
	sechandlers.find(refid);
      if(phandler != sechandlers.end()) {
	    return phandler->second;
      }
      // Look for it's configuration
      desc_node = FindElementByID(cfg, cfg.getFileName(), refid, "SecHandler");
    }
    if(!desc_node) {
      MCCLoader::logger.msg(ERROR, "SecHandler has no configuration");
      return NULL;
    }
    std::string name = desc_node.Attribute("name");
    if(name.empty()) {
      MCCLoader::logger.msg(ERROR, "SecHandler has no name attribute defined");
      return NULL;
    }
    // Create new security handler
    Config cfg_(desc_node, cfg.getFileName());
    ArcSec::SecHandlerPluginArgument arg(&cfg_,ctx);
    Plugin* plugin = factory->get_instance(SecHandlerPluginKind, name, &arg);
    ArcSec::SecHandler* sechandler = plugin?dynamic_cast<ArcSec::SecHandler*>(plugin):NULL;
    if(!sechandler) {
      Loader::logger.msg(ERROR, "Security Handler %s(%s) could not be created", name, refid);
    } else {
      Loader::logger.msg(INFO, "SecHandler: %s(%s)", name, refid);
      sechandlers[refid] = sechandler;
    }
    return sechandler;
  }

  void MCCLoader::make_elements(Config& cfg, int level,
			     mcc_connectors_t *mcc_connectors,
			     plexer_connectors_t *plexer_connectors) {
    if(mcc_connectors == NULL) mcc_connectors = new mcc_connectors_t;
    if(plexer_connectors == NULL) plexer_connectors = new plexer_connectors_t;
    // 1st stage - creating all elements.
    // Configuration is parsed recursively - going deeper at ArcConfig
    // and Chain elements
    for(XMLNode cn = cfg.Child();(bool)cn; ++cn) {
      Config cfg_(cn, cfg.getFileName());

      if(MatchXMLName(cn, "ArcConfig")) {
	make_elements(cfg_, level + 1, mcc_connectors, plexer_connectors);
	continue;
      }

      if(MatchXMLName(cn, "Chain")) {
	make_elements(cfg_, level + 1, mcc_connectors, plexer_connectors);
	continue;
      }

      if(MatchXMLName(cn, "Component")) {
	// Create new MCC
	std::string name = cn.Attribute("name");
	if(name.empty()) {
	  logger.msg(ERROR, "Component has no name attribute defined");
	  continue;
	}
	std::string id = cn.Attribute("id");
	if(id.empty()) {
	  logger.msg(ERROR, "Component has no id attribute defined");
	  continue;
	}
        MCCPluginArgument arg(&cfg_,context_);
	Plugin* plugin = factory_->get_instance(MCCPluginKind ,name, &arg);
	MCC* mcc = plugin?dynamic_cast<MCC*>(plugin):NULL;
	if(!mcc) {
	  logger.msg(ERROR, "Component %s(%s) could not be created", name, id);
	  continue;
	}
	mccs_[id] = mcc;

	// Configure security plugins
	XMLNode an = cn["SecHandler"];
	for(int n = 0;; ++n) {
	  XMLNode can = an[n];
	  if(!can) break;
	  ArcSec::SecHandler* sechandler = MakeSecHandler(cfg, context_,
                                  sechandlers_, factory_, can);
	  if(!sechandler) continue;
	  std::string event = can.Attribute("event");
	  mcc->AddSecHandler(&cfg_, sechandler, event);
	}

	// Add to chain list
	std::string entry = cn.Attribute("entry");
	if(!entry.empty()) mccs_exposed_[entry] = mcc;
	mcc_connector_t mcc_connector(mccs_.find(id));
	for(int nn = 0;; ++nn) {
	  XMLNode cnn = cn["next"][nn];
	  if(!cnn) break;
	  std::string nid = cnn.Attribute("id");
	  if(nid.empty()) {
	    logger.msg(ERROR, "Component's %s(%s) next has no id "
		       "attribute defined", name, id);
	    continue;
	  }
	  std::string label = cnn;
	  mcc_connector.nexts[label] = nid;
	}
	mcc_connector.name = name;
	mcc_connectors->push_back(mcc_connector);
	logger.msg(INFO, "Loaded MCC %s(%s)", name, id);
	continue;
      }

      if(MatchXMLName(cn, "Plexer")) {
	std::string name = cn.Attribute("name");
	std::string id = cn.Attribute("id");
	if(id.empty()) id = "plexer";
	Plexer* plexer = new Plexer(&cfg_);
	plexers_[id] = plexer;
	plexer_connector_t plexer_connector(plexers_.find(id));
	for(int nn = 0;; ++nn) {
	  XMLNode cnn = cn["next"][nn];
	  if(!cnn) break;
	  std::string nid = cnn.Attribute("id");
	  if(nid.empty()) {
	    logger.msg(ERROR, "Plexer's (%s) next has no id "
		       "attribute defined", id);
	    continue;
	  }
	  std::string label = cnn;
	  plexer_connector.nexts[label] = nid;
	}
	plexer_connector.name = name;
	plexer_connectors->push_back(plexer_connector);
	logger.msg(INFO, "Loaded Plexer %s(%s)", name, id);
	continue;
      }

      if(MatchXMLName(cn, "Service")) {
	std::string name = cn.Attribute("name");
	if(name.empty()) {
	  logger.msg(ERROR, "Service has no name attribute defined");
	  continue;
	}
	std::string id = cn.Attribute("id");
	if(id.empty()) {
	  logger.msg(ERROR, "Service has no id attribute defined");
	  continue;
	}
        ServicePluginArgument arg(&cfg_,context_);
	Plugin* plugin = factory_->get_instance(ServicePluginKind, name, &arg);
        Service* service = plugin?dynamic_cast<Service*>(plugin):NULL;
	if(!service) {
	  logger.msg(ERROR, "Service %s(%s) could not be created", name, id);
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
	  ArcSec::SecHandler* sechandler = MakeSecHandler(cfg, context_,
                                  sechandlers_, factory_, can);
	  if(!sechandler) continue;
	  std::string event = can.Attribute("event");
	  service->AddSecHandler(&cfg_, sechandler, event);
	}

	continue;
      }

      // Configuration processing is split to multiple functions - hence
      // ignoring all unknown elements.
      logger.msg(WARNING, "Unknown element \"%s\" - ignoring", cn.Name());
    }

    if(level != 0) return;

    // 2nd stage - making links between elements.

    // Making links from MCCs
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
	  logger.msg(INFO, "Linking MCC %s(%s) to MCC (%s) under %s",
		     mcc->name, mcc->mcc->first, id, label);
	  mcc->nexts.erase(next);
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
	logger.msg(ERROR, "MCC %s(%s) - next %s(%s) has no target",
		   mcc->name, mcc->mcc->first, label, id);
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
	plexer->nexts.erase(next);
      }
    }
    if(mcc_connectors) delete mcc_connectors;
    if(plexer_connectors) delete plexer_connectors;
  }

  MCC* MCCLoader::operator[](const std::string& id) {
    mcc_container_t::iterator mcc = mccs_exposed_.find(id);
    if(mcc != mccs_exposed_.end()) return mcc->second;
    return NULL;
  }

} // namespace Arc
