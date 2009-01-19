#ifndef __ARC_LOADER_H__
#define __ARC_LOADER_H__

#include <string>
#include <map>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/loader/Plugin.h>

namespace Arc {

  /// Plugins loader.
  /** This class processes XML configration and loads specified plugins.
     Accepted configuration is defined by XML schema mcc.xsd.
    "Plugins" elements are parsed by this class and corresponding libraries
     are loaded. */
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
