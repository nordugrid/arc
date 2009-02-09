#ifndef __ARC_MCCLOADER_H__
#define __ARC_MCCLOADER_H__

#include <string>
#include <map>
#include <arc/ArcConfig.h>

#include <arc/message/MCC.h>
#include <arc/message/Service.h>
#include <arc/message/Plexer.h>
#include <arc/security/SecHandler.h>

#include <arc/loader/Plugin.h>
#include <arc/loader/Loader.h>

namespace Arc {

  // Internal classes - used in private methods
  class mcc_connectors_t;
  class plexer_connectors_t;

  class ChainContext;

  /// Creator of Message Component Chains (MCC).
  /** This class processes XML configration and creates message chains.
     Accepted configuration is defined by XML schema mcc.xsd.
    Supported components are of types MCC, Service and Plexer. MCC and
    Service are loaded from dynamic libraries. For Plexer only internal
    implementation is supported.
     This object is also a container for loaded componets. All components
    and chains are destroyed if this object is destroyed.
     Chains are created in 2 steps. First all components are loaded and
    corresponding objects are created. Constructors are supplied with
    corresponding configuration subtrees. During next step components
    are linked together by calling their Next() methods. Each call 
    creates labeled link to next component in a chain.
     2 step method has an advantage over single step because it allows 
    loops in chains and makes loading procedure more simple. But that 
    also means during short period of time components are only partly 
    configured. Components in such state must produce proper error response 
    if Message arrives.
     Note: Current implementation requires all components and links
    to be labeled. All labels must be unique. Future implementation will
    be able to assign labels automatically.
   */
  class MCCLoader: public Loader {
    friend class ChainContext;
   public:
    typedef std::map<std::string, MCC*>        mcc_container_t;
    typedef std::map<std::string, Service*>    service_container_t;
    typedef std::map<std::string, ArcSec::SecHandler*> sechandler_container_t;
    typedef std::map<std::string, Plexer*>     plexer_container_t;

   private:
    /** Set of labeled MCC objects */
    mcc_container_t mccs_;

    /** Set of MCC objects exposed to external interface */
    mcc_container_t mccs_exposed_;

    /** Set of labeled Service objects */
    service_container_t services_;

    /** Set of labeled security handlers */
    sechandler_container_t sechandlers_;

    /** Set of labeled Plexer objects */
    plexer_container_t plexers_;

    /** Internal method which performs whole stuff specific to 
       creation of Message Chains.
       It is taken out from constructor to make it easier to reconfigure
       chains in a future. */
    void make_elements(Config& cfg, int level = 0,
		       mcc_connectors_t *mcc_connectors = NULL,
		       plexer_connectors_t *plexer_connectors = NULL);

    ChainContext* context_;

   public:
    MCCLoader() {};
    /** Constructor that takes whole XML configuration and creates
       component chains */
    MCCLoader(Config& cfg);
    /** Destructor destroys all components created by constructor */
    ~MCCLoader();
    /** Access entry MCCs in chains.
       Those are components exposed for external access using 'entry'
       attribute */
    MCC* operator[](const std::string& id);

 };

  /// Interface to chain specific functionality
  /** Object of this class is associated with every MCCLoader object. It is
    accessible for MCC and Service components and provides an interface
    to manipulate chains stored in Loader. This makes it possible to 
    modify chains dynamically - like deploying new services on demand. */
  class ChainContext {
    friend class MCCLoader;
   private:
    MCCLoader& loader_;
    ChainContext(MCCLoader& loader) : loader_(loader) {};
    ~ChainContext() {};
   public:
    /** Returns associated PluginsFactory object */
    operator PluginsFactory*()    { return loader_.factory_; };
  };

} // namespace Arc

#endif /* __ARC_MCCLOADER_H__ */
