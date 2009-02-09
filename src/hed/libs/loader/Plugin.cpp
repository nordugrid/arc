#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/StringConv.h>

#include "Plugin.h"

namespace Arc {

  static PluginDescriptor* find_constructor(PluginDescriptor* desc,const std::string& kind,int min_version,int max_version) {
    if(!desc) return NULL;
    for(;(desc->kind) && (desc->name);++desc) {
      if((kind == desc->kind) || (kind.empty())) {
        if((min_version <= desc->version) && (max_version >= desc->version)) {
          if(desc->instance) return desc;
        };
      };
    };
    return NULL;
  }

  static PluginDescriptor* find_constructor(PluginDescriptor* desc,const std::string& kind,const std::string& name,int min_version,int max_version) {
    if(!desc) return NULL;
    for(;(desc->kind) && (desc->name);++desc) {
      if(((kind == desc->kind) || (kind.empty())) && 
         ((name == desc->name) || (name.empty()))) {
        if((min_version <= desc->version) && (max_version >= desc->version)) {
          if(desc->instance) return desc;
        };
      };
    };
    return NULL;
  }

  static Glib::Module* probe_module(std::string name,ModuleManager& manager) {
    std::string::size_type p = 0;
    for(;;) {
      p=name.find(':',p);
      if(p == std::string::npos) break;
      name.replace(p,1,"_");
      ++p;
    };
    return manager.load(name,true);
  }

  inline static Glib::Module* reload_module(Glib::Module* module,ModuleManager& manager) {
    if(!module) return NULL;
    return manager.reload(module);
  }

  inline static void unload_module(Glib::Module* module,ModuleManager& manager) {
    if(!module) return;
    manager.unload(module);
  }

  const char* plugins_table_name = PLUGINS_TABLE_SYMB;

  Logger PluginsFactory::logger(Logger::rootLogger, "Plugin");

  Plugin::Plugin(void) { }

  Plugin::~Plugin(void) { }

  Plugin* PluginsFactory::get_instance(const std::string& kind,PluginArgument* arg) {
    return get_instance(kind,0,INT_MAX,arg);
  }

  Plugin* PluginsFactory::get_instance(const std::string& kind,int version,PluginArgument* arg) {
    return get_instance(kind,version,version,arg);
  }

  Plugin* PluginsFactory::get_instance(const std::string& kind,int min_version,int max_version,PluginArgument* arg) {
    descriptors_t_::iterator i = descriptors_.begin();
    for(;i != descriptors_.end();++i) {
      PluginDescriptor* desc = i->second;
      for(;;) {
        desc=find_constructor(desc,kind,min_version,max_version);
        if(!desc) break;
        Plugin* plugin = desc->instance(arg);
        if(plugin) return plugin;
        ++desc;
      };
    };
    // Try to load module of plugin
    std::string mname = kind;
    Glib::Module* module = probe_module(kind,*this);
    if (module == NULL) {
      logger.msg(ERROR, "Could not find loadable module by name %s (%s)",kind,Glib::Module::get_last_error());
      return NULL;
    };
    // Identify table of descriptors
    void *ptr = NULL;
    if(!module->get_symbol(plugins_table_name,ptr)) {
      logger.msg(DEBUG, "Module %s is not an ARC plugin (%s)",kind,Glib::Module::get_last_error());
      unload_module(module,*this);
      return NULL;
    };
    // Try to find plugin in new table
    PluginDescriptor* desc = (PluginDescriptor*)ptr;
    for(;;) {
      desc=find_constructor(desc,kind,min_version,max_version);
      if(!desc) break;
      Plugin* plugin = desc->instance(arg);
      if(plugin) {
        // Keep plugin loaded and registered
        Glib::Module* nmodule = reload_module(module,*this);
        if(!nmodule) {
          logger.msg(DEBUG, "Module %s failed to reload (%s)",mname,Glib::Module::get_last_error());
          unload_module(module,*this);
          return false;
        };
        descriptors_[mname]=(PluginDescriptor*)ptr;
        modules_[mname]=nmodule;
        //descriptors_.push_back((PluginDescriptor*)ptr);
        //modules_.push_back(module);
        return plugin;
      };
      ++desc;
    };
    unload_module(module,*this);
    return NULL;
  }

  Plugin* PluginsFactory::get_instance(const std::string& kind,const std::string& name,PluginArgument* arg) {
    return get_instance(kind,name,0,INT_MAX,arg);
  }

  Plugin* PluginsFactory::get_instance(const std::string& kind,const std::string& name,int version,PluginArgument* arg) {
    return get_instance(kind,version,version,arg);
  }

  Plugin* PluginsFactory::get_instance(const std::string& kind,const std::string& name,int min_version,int max_version,PluginArgument* arg) {
    descriptors_t_::iterator i = descriptors_.begin();
    for(;i != descriptors_.end();++i) {
      PluginDescriptor* desc = find_constructor(i->second,kind,name,min_version,max_version);
      if(desc) return desc->instance(arg);
    };
    // Try to load module - first by name of plugin
    std::string mname = name;
    Glib::Module* module = probe_module(name,*this);
    if (module == NULL) {
      // Then by kind of plugin
      mname=kind;
      module=probe_module(kind,*this);
      logger.msg(ERROR, "Could not find loadable module by name %s and %s (%s)",name,kind,Glib::Module::get_last_error());
      return NULL;
    };
    // Identify table of descriptors
    void *ptr = NULL;
    if(!module->get_symbol(plugins_table_name,ptr)) {
      logger.msg(DEBUG, "Module %s is not an ARC plugin (%s)",mname,Glib::Module::get_last_error());
      unload_module(module,*this);
      return NULL;
    };
    // Try to find plugin in new table
    PluginDescriptor* desc = find_constructor((PluginDescriptor*)ptr,kind,name,min_version,max_version);
    if(desc) {
      // Keep plugin loaded and registered
      Glib::Module* nmodule = reload_module(module,*this);
      if(!nmodule) {
        logger.msg(DEBUG, "Module %s failed to reload (%s)",mname,Glib::Module::get_last_error());
        unload_module(module,*this);
        return false;
      };
      descriptors_[mname]=(PluginDescriptor*)ptr;
      modules_[mname]=nmodule;
      //descriptors_.push_back((PluginDescriptor*)ptr);
      //modules_.push_back(module);
      return desc->instance(arg);
    };
    unload_module(module,*this);
    return NULL;
  }

  bool PluginsFactory::load(const std::string& name) {
    std::list<std::string> kinds;
    return load(name,kinds);
  }

  bool PluginsFactory::load(const std::string& name,const std::string& kind) {
    std::list<std::string> kinds;
    kinds.push_back(kind);
    return load(name,kinds);
  }

  bool PluginsFactory::load(const std::string& name,const std::list<std::string>& kinds) {
    Glib::Module* module = NULL;
    void *ptr = NULL;
    std::string mname;
    // Check if module already loaded
    descriptors_t_::iterator d = descriptors_.find(name);
    if(d != descriptors_.end()) {
      ptr = d->second;
      if(!ptr) return false;
    } else {
      // Try to load module by specified name
      mname = name;
      module = probe_module(name,*this);
      if (module == NULL) {
        logger.msg(WARNING, "Could not find loadable module by name %s (%s)",name,Glib::Module::get_last_error());
        return false;
      };
      // Identify table of descriptors
      if(!module->get_symbol(plugins_table_name,ptr)) {
        logger.msg(DEBUG, "Module %s is not an ARC plugin (%s)",mname,Glib::Module::get_last_error());
        unload_module(module,*this);
        return false;
      };
    };
    if(kinds.size() > 0) {
      PluginDescriptor* desc = NULL;
      for(std::list<std::string>::const_iterator kind = kinds.begin();
          kind != kinds.end(); ++kind) {
        desc=find_constructor((PluginDescriptor*)ptr,*kind,0,INT_MAX);
        if(desc) break;
      };
      if(!desc) {
        //logger.msg(DEBUG, "Module %s does not contain plugin(s) of specified kind(s)",mname);
        if(module) unload_module(module,*this);
        return false;
      };
    };
    if(!mname.empty()) {
      Glib::Module* nmodule=reload_module(module,*this);
      if(!nmodule) {
        logger.msg(DEBUG, "Module %s failed to reload (%s)",mname,Glib::Module::get_last_error());
        unload_module(module,*this);
        return false;
      };
      descriptors_[mname]=(PluginDescriptor*)ptr;
      modules_[mname]=nmodule;
      //descriptors_.push_back((PluginDescriptor*)ptr);
      //modules_.push_back(module);
    };
    return true;
  }

  PluginsFactory::PluginsFactory(const Config& cfg): ModuleManager(&cfg) {
  }

  PluginArgument::PluginArgument(void) {
  }
 
  PluginArgument::~PluginArgument(void) {
  }
 
} // namespace Arc

