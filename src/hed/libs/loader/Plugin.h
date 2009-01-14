#ifndef __ARC_PLUGIN_H__

#define __ARC_PLUGIN_H__

#include <string>
#include <map>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include <arc/Logger.h>
#include <arc/loader/ModuleManager.h>

namespace Arc {

  /// Base class for loadable ARC components.
  /** All classes representing loadable ARC components must be either 
     descendants of this class or be wrapped by its offspring. */
  class Plugin {
    protected:
      Plugin(void);
    public:
      virtual ~Plugin(void);
  };

  /// Base class for passing arguments to loadable ARC components.
  /** During its creation constructor function of ARC loadable component
     expects instance of class inherited from this one or wrapped in it.
     Then dynamic type casting is used for obtaining class of expected kind. */
  class PluginArgument {
    protected:
      PluginArgument(void);
    public:
      virtual ~PluginArgument(void);
  };

  /// Name of symbol refering to table of plugins.
  /** This C null terminated string specifies name of symbol which 
     shared library should export to give an access to an array 
     of PluginDescriptor elements. The array is terminated by 
     element with all components set to NULL. */
  extern const char* plugins_table_name;

  #define PLUGINS_TABLE_NAME __arc_plugins_table__
  #define PLUGINS_TABLE_SYMB "__arc_plugins_table__"

  /// Constructor function of ARC lodable component
  /** This function is called with plugin-specific argument and 
     should produce and return valid instance of plugin.
     If plugin can't be produced by any reason (for example
     because passed argument is not applicable) then NULL
     is returned. No exceptions should be raised. */
  typedef Plugin* (*get_plugin_instance)(PluginArgument* arg);

  /// Description of ARC lodable component
  typedef struct {
    const char* name; // Unique name of plugin in scope of its kind
    const char* kind; // Type/kind of plugin
    uint32_t version; // Version of plugin (0 if not applicable)
    get_plugin_instance instance; // Pointer to constructor function
  } PluginDescriptor;

  /// Generic ARC plugins loader
  /** The instance of this class provides functionality
     of loading pluggable ARC components stored in shared 
     libraries. For more information please check HED 
     documentation. */
  class PluginsFactory: public ModuleManager {
    private:
      typedef std::map<std::string,PluginDescriptor*> descriptors_t_;
      typedef std::map<std::string,Glib::Module*> modules_t_;
      descriptors_t_ descriptors_;
      modules_t_ modules_;
      static Logger logger;
    public:
      /** Constructor - accepts  configuration (not yet used) meant to
        tune loading of modules. */
      PluginsFactory(const Config& cfg);
      /** These methods load shared library named lib'name', locate plugin
         constructor functions of specified 'kind' and 'name' (if specified) 
         and call it. Supplied argument affects way plugin instance is created
         in plugin-specific way.
         If name of plugin is not specified then all plugins of specified kind 
         are tried with supplied argument till valid instance is created.
         All loaded plugins are also registered in internal list of this
         instance of PluginsFactory class.
         Returns created instance. */
      Plugin* get_instance(const std::string& kind,PluginArgument* arg);
      Plugin* get_instance(const std::string& kind,int version,PluginArgument* arg);
      Plugin* get_instance(const std::string& kind,int min_version,int max_version,PluginArgument* arg);
      Plugin* get_instance(const std::string& kind,const std::string& name,PluginArgument* arg);
      Plugin* get_instance(const std::string& kind,const std::string& name,int version,PluginArgument* arg);
      Plugin* get_instance(const std::string& kind,const std::string& name,int min_version,int max_version,PluginArgument* arg);
      /** These methods load shared library named lib'name' and check if it
        contains ARC plugins of specified 'kind'. If there are no specified
        plugins or if library does not contain any plugins it is unloaded.
        All loaded plugins are also registered in internal list of this
        instance of PluginsFactory class.
        Returns true if library was loaded. */ 
      bool load(const std::string& name);
      bool load(const std::string& name,const std::string& kind);
      bool load(const std::string& name,const std::list<std::string>& kinds);
      template<class P>
      P* GetInstance(const std::string& kind,PluginArgument* arg) {
        Plugin* plugin = get_instance(kind,arg);
        if(!plugin) return NULL;
        P* p = dynamic_cast<P*>(plugin);
        if(!p) delete plugin;
        return p;
      };
      template<class P>
      P* GetInstance(const std::string& kind,const std::string& name,PluginArgument* arg) {
        Plugin* plugin = get_instance(kind,name,arg);
        if(!plugin) return NULL;
        P* p = dynamic_cast<P*>(plugin);
        if(!p) delete plugin;
        return p;
      };
  };

  template<class P>
  P* PluginCast(PluginArgument* p) {
    if(p == NULL) return NULL;
    P* pp = dynamic_cast<P*>(p);
    if(pp != NULL) return pp;
    // Workaround for g++ and loadable modules
    if(strcmp(typeid(P).name(),typeid(*p).name()) != 0) return NULL;
    return static_cast<P*>(p);
  }

  template<class P>
  P* PluginCast(Plugin* p) {
    if(p == NULL) return NULL;
    P* pp = dynamic_cast<P*>(p);
    if(pp != NULL) return pp;
    // Workaround for g++ and loadable modules
    if(strcmp(typeid(P).name(),typeid(*p).name()) != 0) return NULL;
    return static_cast<P*>(p);
  }

} // namespace Arc

#endif /* __ARC_PLUGIN_H__ */

