#ifndef __ARC_LOADER_H__
#define __ARC_LOADER_H__

#include <string>
#include <map>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/loader/Plugin.h>

namespace Arc {

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
  class Loader {
   public:
    static Logger logger;

   protected:
    /** Link to Factory responsible for loading and creation of
       Plugin and derived objects */
    PluginsFactory *factory_;

   public:
    Loader() {};
    /** Constructor that takes whole XML configuration and performs
       common configuration part */
    Loader(Config& cfg);
    /** Destructor destroys all components created by constructor */
    ~Loader();
 };

} // namespace Arc

#endif /* __ARC_LOADER_H__ */
