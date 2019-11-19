#ifndef __ARC_PLUGIN_H__

#define __ARC_PLUGIN_H__

#include <string>
#include <map>
#include <typeinfo>
#include <inttypes.h>
#include <sys/types.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include <arc/Thread.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/loader/ModuleManager.h>

namespace Arc {

  #define ARC_PLUGIN_DEFAULT_PRIORITY (128)

  class PluginsFactory;

  /// Base class for passing arguments to loadable ARC components.
  /** During its creation constructor function of ARC loadable component
     expects instance of class inherited from this one or wrapped in it.
     Then dynamic type casting is used for obtaining class of expected kind. */
  class PluginArgument {
    friend class PluginsFactory;
    private:
      PluginsFactory* factory_;
      Glib::Module* module_;
      void set_factory(PluginsFactory* factory);
      void set_module(Glib::Module* module);
    protected:
      PluginArgument(void);
    public:
      virtual ~PluginArgument(void);
      /// Returns pointer to factory which instantiated plugin.
      /** Because factory usually destroys/unloads plugins in its
         destructor it should be safe to keep this pointer inside
         plugin for later use. But one must always check. */
      PluginsFactory* get_factory(void);
      /// Returns pointer to loadable module/library which contains plugin.
      /** Corresponding factory keeps list of modules till itself is destroyed.
         So it should be safe to keep that pointer. But care must be taken
         if module contains persistent plugins. Such modules stay in memory
         after factory is detroyed. So it is advisable to use obtained
         pointer only in constructor function of plugin. */
      Glib::Module* get_module(void);
  };

  /// Base class for loadable ARC components.
  /** All classes representing loadable ARC components must be either
     descendants of this class or be wrapped by its offspring. */
  class Plugin {
    private:
      Plugin(void);
      Plugin& operator=(const Plugin&);
    protected:
      PluginsFactory* factory_;
      Glib::Module* module_;
      /// Main constructor for creating new plugin object
      Plugin(PluginArgument* arg);
      /// Constructor to be used if plugin want to copy itself
      Plugin(const Plugin& obj);
    public:
      virtual ~Plugin(void);
  };

  /// Name of symbol refering to table of plugins.
  /** This C null terminated string specifies name of symbol which
     shared library should export to give an access to an array
     of PluginDescriptor elements. The array is terminated by
     element with all components set to NULL. */
  extern const char* plugins_table_name;

  #define ARC_PLUGINS_TABLE_NAME __arc_plugins_table__
  #define ARC_PLUGINS_TABLE_SYMB "__arc_plugins_table__"

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
    const char* description; // Short description of plugin
    uint32_t version; // Version of plugin (0 if not applicable)
    get_plugin_instance instance; // Pointer to constructor function
  } PluginDescriptor;


  /// Description of plugin
  /** This class is used for reports */
  class PluginDesc {
   public:
    std::string name;
    std::string kind;
    std::string description;
    uint32_t version;
    uint32_t priority;
    PluginDesc(void):version(0),priority(ARC_PLUGIN_DEFAULT_PRIORITY) { };
  };

  /// Description of loadable module
  /** This class is used for reports */
  class ModuleDesc {
   public:
    std::string name;
    std::list<PluginDesc> plugins;
  };

  /// Generic ARC plugins loader
  /** The instance of this class provides functionality
     of loading pluggable ARC components stored in shared
     libraries. It also makes use of Arc Plugin Description
     (*.apd) files which contain textual plugin identfiers. 
     Arc Plugin Description files contain attributes 
     describing pluggable components stored in corresponding
     shared libraries. Using those files allows to save 
     on actually loading and resolving libraries while
     looking for specific component.

     Specifically this class uses 'priority' attribute to sort
     plugin description in internal lists. Please note
     that priority affects order in which plugins tried 
     in get_instance(...) methods. But it only works
     for plugins which were already loaded by previous 
     calls to load(...) and get_instance(...) methods.
     For plugins discovered inside get_instance priority
     in not effective.

     This class mostly handles tasks of finding, identifying,
     fitering and sorting ARC pluggable components. For loading
     shared libraries it uses functionality of ModuleManager class.
     So it is important to see documentation of ModuleManager in
     order to understand how this class works.

     For more information also please check ARC HED documentation.

     This class is thread-safe - its methods are protected from
     simultatneous use from multiple threads. Current thread
     protection implementation is suboptimal and will be revised
     in future.
   */
  class PluginsFactory: public ModuleManager {
    friend class PluginArgument;
    private:
      Glib::Mutex lock_;

      // Combined convenient description of module and 
      // its representation inside module.
      class descriptor_t_ {
       public:
        PluginDesc desc_i;
        PluginDescriptor* desc_m;
      };

      class module_t_ {
       public:
        // Handle of loaded module
        Glib::Module* module;
        // List of contained plugins. First one is also
        // pointer to plugins table.
        std::list<descriptor_t_> plugins;
        PluginDescriptor* get_table(void) { return plugins.empty()?NULL:(plugins.front().desc_m); };
      };

      //typedef std::map<std::string,module_t_> modules_t_;
      // Container for all loaded modules and their plugins
      class modules_t_: public std::map<std::string,module_t_> {
       public:
        // convenience iterator for modules
        class miterator: public std::map<std::string,module_t_>::iterator {
         private:
          std::map<std::string,module_t_>* ref_;
         public:
          miterator(std::map<std::string,module_t_>& ref, std::map<std::string,module_t_>::iterator iter):std::map<std::string,module_t_>::iterator(iter),ref_(&ref) { };
          operator bool(void) const { return (std::map<std::string,module_t_>::iterator&)(*this) != ref_->end(); };
          bool operator!(void) const { return !((bool)(*this)); };
        }; // class miterator
        // iterator for accessing plugins by priority
        class diterator: public std::list< std::pair<descriptor_t_*,module_t_*> >::iterator {
         private:
          std::list< std::pair<descriptor_t_*,module_t_*> >* ref_;
         public:
          diterator(std::list< std::pair<descriptor_t_*,module_t_*> >& ref, std::list< std::pair<descriptor_t_*,module_t_*> >::iterator iter):std::list< std::pair<descriptor_t_*,module_t_*> >::iterator(iter),ref_(&ref) { };
          operator bool(void) const { return (std::list< std::pair<descriptor_t_*,module_t_*> >::iterator&)(*this) != ref_->end(); };
          bool operator!(void) const { return !((bool)(*this)); };
        }; // class diterator
        operator miterator(void) { return miterator(*this,this->begin()); };
        miterator find(const std::string& name) { return miterator(*this,((std::map<std::string,module_t_>*)this)->find(name)); };
        operator diterator(void) { return diterator(plugins_,plugins_.begin()); };
        bool add(ModuleDesc* m_i, Glib::Module* m_h, PluginDescriptor* d_h);
        bool remove(miterator& module);
       private:
        // Plugins sorted by priority
        std::list< std::pair<descriptor_t_*,module_t_*> > plugins_;
        // TODO: potentially other sortings like by kind can be useful.
      };

      modules_t_ modules_;

      bool try_load_;
      static Logger logger;
      bool load(const std::string& name,const std::list<std::string>& kinds,const std::list<std::string>& pnames);
    public:
      /** Constructor - accepts  configuration (not yet used) meant to
        tune loading of modules. */
      PluginsFactory(XMLNode cfg);
      /** Specifies if loadable module may be loaded while looking
         for analyzing its content. If set to false only *.apd
         files are checked. Modules without corresponding *.apd
         will be ignored. Default is true; */
      void TryLoad(bool v) { try_load_ = v; };
      bool TryLoad(void) { return try_load_; };
      /** These methods load module named lib'name', locate plugin
         constructor functions of specified 'kind' and 'name' (if specified)
         and call it. Supplied argument affects way plugin instance is created
         in plugin-specific way.
         If name of plugin is not specified then all plugins of specified kind
         are tried with supplied argument till valid instance is created.
         All loaded plugins are also registered in internal list of this
         instance of PluginsFactory class.
         If search is set to false then no attempt is made to find plugins in
         loadable modules. Only plugins already loaded with previous calls to
         get_instance() and load() are checked.
         Returns created instance or NULL if failed. */
      Plugin* get_instance(const std::string& kind,PluginArgument* arg,bool search = true);
      Plugin* get_instance(const std::string& kind,int version,PluginArgument* arg,bool search = true);
      Plugin* get_instance(const std::string& kind,int min_version,int max_version,PluginArgument* arg,bool search = true);
      Plugin* get_instance(const std::string& kind,const std::string& name,PluginArgument* arg,bool search = true);
      Plugin* get_instance(const std::string& kind,const std::string& name,int version,PluginArgument* arg,bool search = true);
      Plugin* get_instance(const std::string& kind,const std::string& name,int min_version,int max_version,PluginArgument* arg,bool search = true);
      /** These methods load module named lib'name' and check if it
        contains ARC plugin(s) of specified 'kind' and 'name'. If there are no
        specified plugins or module does not contain any ARC plugins it is
        unloaded.
        All loaded plugins are also registered in internal list of this
        instance of PluginsFactory class.
        Returns true if any plugin was loaded. */
      bool load(const std::string& name);
      bool load(const std::string& name,const std::string& kind);
      bool load(const std::string& name,const std::string& kind,const std::string& pname);
      bool load(const std::string& name,const std::list<std::string>& kinds);
      bool load(const std::list<std::string>& names,const std::string& kind);
      bool load(const std::list<std::string>& names,const std::string& kind,const std::string& pname);
      bool load(const std::list<std::string>& names,const std::list<std::string>& kinds);

      /** Collect information about plugins stored in module(s)
        with specified names.
        Returns true if any of specified modules has plugins. */
      bool scan(const std::string& name, ModuleDesc& desc);
      bool scan(const std::list<std::string>& names, std::list<ModuleDesc>& descs);
      /** Provides information about currently loaded modules and plugins. */
      void report(std::list<ModuleDesc>& descs);
      /** Filter list of modules by kind. */
      static void FilterByKind(const std::string& kind, std::list<ModuleDesc>& descs);

      template<class P>
      P* GetInstance(const std::string& kind,PluginArgument* arg,bool search = true) {
        Plugin* plugin = get_instance(kind,arg,search);
        if(!plugin) return NULL;
        P* p = dynamic_cast<P*>(plugin);
        if(!p) delete plugin;
        return p;
      }
      template<class P>
      P* GetInstance(const std::string& kind,const std::string& name,PluginArgument* arg,bool search = true) {
        Plugin* plugin = get_instance(kind,name,arg,search);
        if(!plugin) return NULL;
        P* p = dynamic_cast<P*>(plugin);
        if(!p) delete plugin;
        return p;
      }
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

