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
      if(kind == desc->kind) {
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
      if((kind == desc->kind) && (name == desc->name)) {
        if((min_version <= desc->version) && (max_version >= desc->version)) {
          if(desc->instance) return desc;
        };
      };
    };
    return NULL;
  }

  static Glib::Module* load_module(std::string name,ModuleManager& manager) {
    std::string::size_type p = 0;
    for(;;) {
      p=name.find(':',p);
      if(p == std::string::npos) break;
      name.replace(p,1,"_");
      ++p;
    };
    return manager.load(name);
  }

  const char* plugins_table_name = "__arc_plugins_table__";

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
      PluginDescriptor* desc = *i;
      for(;;) {
        desc=find_constructor(desc,kind,min_version,max_version);
        if(!desc) break;
        Plugin* plugin = desc->instance(arg);
        if(plugin) return plugin;
      };
    };
    // Try to load module of plugin
    Glib::Module* module = load_module(kind,*this);
    if (module == NULL) {
      logger.msg(ERROR, "Could not find loadable module by name %s (%s)",kind,Glib::Module::get_last_error());
      return NULL;
    };
    // Identify table of descriptors
    void *ptr = NULL;
    if(!module->get_symbol(plugins_table_name,ptr)) {
      logger.msg(DEBUG, "Module %s is not an ARC plugin (%s)",kind,Glib::Module::get_last_error());
      return NULL;
    };
    // Once loaded we keep module anyway. Add it's table to list.
    descriptors_.push_back((PluginDescriptor*)ptr);
    // Try to find plugin in new table
    PluginDescriptor* desc = (PluginDescriptor*)ptr;
    for(;;) {
      desc=find_constructor(desc,kind,min_version,max_version);
      if(!desc) break;
      Plugin* plugin = desc->instance(arg);
      if(plugin) return plugin;
    };
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
      PluginDescriptor* desc = find_constructor(*i,kind,name,min_version,max_version);
      if(desc) return desc->instance(arg);
    };
    // Try to load module - first by name of plugin
    std::string mname = name;
    Glib::Module* module = load_module(name,*this);
    if (module == NULL) {
      // Then by kind of plugin
      mname=kind;
      module=load_module(kind,*this);
      logger.msg(ERROR, "Could not find loadable module by name %s and %s (%s)",name,kind,Glib::Module::get_last_error());
      return NULL;
    };
    // Identify table of descriptors
    void *ptr = NULL;
    if(!module->get_symbol(plugins_table_name,ptr)) {
      logger.msg(DEBUG, "Module %s is not an ARC plugin (%s)",mname,Glib::Module::get_last_error());
      return NULL;
    };
    // Once loaded we keep module anyway. Add it's table to list.
    descriptors_.push_back((PluginDescriptor*)ptr);
    // Try to find plugin in new table
    PluginDescriptor* desc = find_constructor((PluginDescriptor*)ptr,kind,name,min_version,max_version);
    if(desc) return desc->instance(arg);
    return NULL;
  }

  PluginsFactory::PluginsFactory(const Config& cfg): ModuleManager(&cfg) {
  }
 
} // namespace Arc

