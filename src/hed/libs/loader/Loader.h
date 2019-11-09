#ifndef __ARC_LOADER_H__
#define __ARC_LOADER_H__

#include <string>
#include <map>
#include <arc/Logger.h>
#include <arc/loader/Plugin.h>

namespace Arc {

  /// Plugins loader.
  /** This class processes XML configration and loads specified plugins.
     Accepted configuration is defined by XML schema mcc.xsd.
    "Plugins" elements are parsed by this class and corresponding libraries
     are loaded.
     Main functionality is provided by class PluginsFactory. */
  class Loader {
   public:
    static Logger logger;

   protected:
    /** Link to Factory responsible for loading and creation of
       Plugin and derived objects */
    PluginsFactory *factory_;

   public:
    Loader() : factory_(NULL) {};
    /** Constructor that takes whole XML configuration and performs
       common configuration part */
    Loader(XMLNode cfg);
    /** Destructor destroys all components created by constructor */
    ~Loader();

   private:
    Loader(const Loader&);
    Loader& operator=(const Loader&);
 };

} // namespace Arc

#endif /* __ARC_LOADER_H__ */
