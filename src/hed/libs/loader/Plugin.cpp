#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm.h>

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/Utils.h>

#include "Plugin.h"

namespace Arc {

  static std::string strip_newline(const std::string& str) {
    std::string s(str);
    std::string::size_type p = 0;
    while((p=s.find('\r',p)) != std::string::npos) s[p]=' ';
    p=0;
    while((p=s.find('\n',p)) != std::string::npos) s[p]=' ';
    return s;
  }

  static bool issane(const char* s) {
    if(!s) return true;
    for(unsigned int n = 0;n<2048;++n) {
      if(!s[n]) return true;
    }
    return false;
  }

  bool PluginsFactory::modules_t_::add(ModuleDesc* m_i, Glib::Module* m_h, PluginDescriptor* d_h) {
    if(this->find(m_i->name)) return false;
    // TODO: too many copying - reduce
    module_t_ module;
    module.module = m_h;
    unsigned int sane_count = 1024;
    // Loop through plugins in module
    for(PluginDescriptor* p = d_h;(p->kind) && (p->name) && (p->instance);++p) {
      if(--sane_count == 0) break;
      if(!issane(p->kind)) break;
      if(!issane(p->name)) break;
      if(!issane(p->description)) break;
      // Find matching description and prepare object to store
      descriptor_t_ d;
      d.desc_m = p;
      d.desc_i.name = p->name; d.desc_i.kind = p->kind; d.desc_i.version = p->version;
      if(p->description) d.desc_i.description = p->description;
      d.desc_i.priority = ARC_PLUGIN_DEFAULT_PRIORITY;
      for(std::list<PluginDesc>::iterator pd = m_i->plugins.begin();
                         pd != m_i->plugins.end();++pd) {
        if((pd->name == d.desc_i.name) &&
           (pd->kind == d.desc_i.kind) &&
           (pd->version == d.desc_i.version)) {
          d.desc_i = *pd; break;
        };
      };
      // Store obtained description
      module.plugins.push_back(d);
    };
    // Store new module
    module_t_& module_r = ((*this)[m_i->name] = module);

    // Add new descriptions to plugins list sorted by priority
    for(std::list<descriptor_t_>::iterator p = module_r.plugins.begin();
                             p != module_r.plugins.end(); ++p) {
      // Find place
      std::list< std::pair<descriptor_t_*,module_t_*> >::iterator pp = plugins_.begin();
      for(; pp != plugins_.end(); ++pp) {
        if((*p).desc_i.priority > (*pp).first->desc_i.priority) break;
      };
      plugins_.insert(pp,std::pair<descriptor_t_*,module_t_*>(&(*p),&module_r));
    };
    return true;
  }

  bool PluginsFactory::modules_t_::remove(PluginsFactory::modules_t_::miterator& module) {
    // Remove links from descriptors/plugins list
    for(std::list<descriptor_t_>::iterator p = module->second.plugins.begin();
                             p != module->second.plugins.end(); ++p) {
      // Find it
      for(std::list< std::pair<descriptor_t_*,module_t_*> >::iterator pp = plugins_.begin();
                               pp != plugins_.end(); ++pp) {
        if((*pp).first == &(*p)) { // or compare by module?
          plugins_.erase(pp); break;
        };
      };
    };
    // Remove module itself
    this->erase(module);
    module = PluginsFactory::modules_t_::miterator(*this,this->end());
    return true;
  }

  static PluginDescriptor* find_constructor(PluginDescriptor* desc,const std::string& kind,int min_version,int max_version,int dsize = -1) {
    if(!desc) return NULL;
    unsigned int sane_count = 1024;
    for(;(desc->kind) && (desc->name) && (desc->instance);++desc) {
      if(dsize == 0) break;
      if(--sane_count == 0) break;
      if(!issane(desc->kind)) break;
      if(!issane(desc->name)) break;
      if(!issane(desc->description)) break;
      if((kind == desc->kind) || (kind.empty())) {
        if((min_version <= desc->version) && (max_version >= desc->version)) {
          if(desc->instance) return desc;
        };
      };
      if(dsize >= 0) --dsize;
    };
    return NULL;
  }

  static PluginDescriptor* find_constructor(PluginDescriptor* desc,const std::string& kind,const std::string& name,int min_version,int max_version,int dsize = -1) {
    if(!desc) return NULL;
    unsigned int sane_count = 1024;
    for(;(desc->kind) && (desc->name) && (desc->instance);++desc) {
      if(dsize == 0) break;
      if(--sane_count == 0) break;
      if(!issane(desc->kind)) break;
      if(!issane(desc->name)) break;
      if(!issane(desc->description)) break;
      if(((kind == desc->kind) || (kind.empty())) &&
         ((name == desc->name) || (name.empty()))) {
        if((min_version <= desc->version) && (max_version >= desc->version)) {
          if(desc->instance) return desc;
        };
      };
      if(dsize >= 0) --dsize;
    };
    return NULL;
  }

  // TODO: Merge with ModuleDesc and PluginDesc. That would reduce code size and
  // make manipulation of *.apd files exposed through API.
  class ARCModuleDescriptor {
   private:
    bool valid;
    class ARCPluginDescriptor {
     public:
      std::string name;
      std::string kind;
      std::string description;
      uint32_t version;
      uint32_t priority;
      bool valid;
      ARCPluginDescriptor(std::ifstream& in):valid(false) {
        if(!in) return;
        std::string line;
        version = 0;
        priority = ARC_PLUGIN_DEFAULT_PRIORITY;
        // Protect against insane line length?
        while(std::getline(in,line)) {
          line = trim(line);
          if(line.empty()) break; // end of descripton
          std::string::size_type p = line.find('=');
          std::string tag = line.substr(0,p);
          line.replace(0,p+1,"");
          line = trim(line);
          if(line.length() < 2) return;
          if(line[0] != '"') return;
          if(line[line.length()-1] != '"') return;
          line=line.substr(1,line.length()-2);
          p=0;
          while((p = line.find('\\',p)) != std::string::npos) {
            line.replace(p,1,""); ++p;
          }
          if(tag == "name") {
            name = line;
          } else if(tag == "kind") {
            kind = line;
          } else if(tag == "description") {
            description = line;
          } else if(tag == "version") {
            if(!stringto(line,version)) return;
          } else if(tag == "priority") {
            if(!stringto(line,priority)) return;
          }
        }
        if(name.empty()) return;
        if(kind.empty()) return;
        valid = true;
      };
    };
    std::list<ARCPluginDescriptor> descriptors;
   public:
    ARCModuleDescriptor(std::ifstream& in):valid(false) {
      if(!in) return;
      for(;;) {
        ARCPluginDescriptor plg(in);
        if(!plg.valid) break;
        descriptors.push_back(plg);
      };
      valid = true;
    }

    operator bool(void) const { return valid; };
    bool operator!(void) const { return !valid; };

    bool contains(const std::list<std::string>& kinds) const {
      if(kinds.size() == 0) return valid;
      for(std::list<std::string>::const_iterator kind = kinds.begin();
               kind != kinds.end(); ++kind) {
        if(contains(*kind)) return true;
      };
      return false;
    }

    bool contains(const std::string& kind) const {
      for(std::list<ARCPluginDescriptor>::const_iterator desc =
                descriptors.begin(); desc != descriptors.end(); ++desc) {
        if(desc->kind == kind) return true;
      };
      return false;
    };

    bool contains(const std::string& kind, const std::string& pname) {
      for(std::list<ARCPluginDescriptor>::const_iterator desc =
                descriptors.begin(); desc != descriptors.end(); ++desc) {
        if((desc->name == pname) && (desc->kind == kind)) return true;
      };
      return false;
    };

    void get(std::list<PluginDesc>& descs) {
      for(std::list<ARCPluginDescriptor>::const_iterator desc =
                descriptors.begin(); desc != descriptors.end(); ++desc) {
        PluginDesc pd;
        pd.name = desc->name;
        pd.kind = desc->kind;
        pd.description = desc->description;
        pd.version = desc->version;
        pd.priority = desc->priority;
        descs.push_back(pd);
      };
    };
  };

  static void replace_file_suffix(std::string& path,const std::string& suffix) {
    std::string::size_type name_p = path.rfind(G_DIR_SEPARATOR_S);
    if(name_p == std::string::npos) {
      name_p = 0;
    } else {
      ++name_p;
    }
    std::string::size_type suffix_p = path.find('.',name_p);
    if(suffix_p != std::string::npos) {
      path.resize(suffix_p);
    }
    path += "." + suffix;
  }


  // Look for apd file of specified name and extract plugin descriptor
  static ARCModuleDescriptor* probe_descriptor(std::string name,ModuleManager& manager) {
    std::string::size_type p = 0;
    // Replace ':' symbol by safe '_'
    for(;;) {
      p=name.find(':',p);
      if(p == std::string::npos) break;
      name.replace(p,1,"_");
      ++p;
    };
    // Find loadable library file by name
    std::string path = manager.find(name);
    if(path.empty()) return NULL;
    // Check for presece of plugin descriptor in apd file
    replace_file_suffix(path,"apd");
    std::ifstream in(path.c_str());
    ARCModuleDescriptor* md = new ARCModuleDescriptor(in);
    if(!(*md)) {
      delete md;
      return NULL;
    };
    return md;
  }


  // Try to find and load shared library file by name
  static Glib::Module* probe_module(std::string name,ModuleManager& manager,Logger& logger) {
    std::string::size_type p = 0;
    for(;;) {
      p=name.find(':',p);
      if(p == std::string::npos) break;
      name.replace(p,1,"_");
      ++p;
    };
    Glib::Module* module = manager.load(name,true);
    return module;
  }

  inline static Glib::Module* reload_module(Glib::Module* module,ModuleManager& manager) {
    if(!module) return NULL;
    return manager.reload(module);
  }

  static void unload_module(Glib::Module* module,ModuleManager& manager) {
    if(!module) return;
    manager.unload(module);
  }

  const char* plugins_table_name = ARC_PLUGINS_TABLE_SYMB;

  Logger PluginsFactory::logger(Logger::rootLogger, "Plugin");

  Plugin::Plugin(PluginArgument* arg):
       factory_(arg?arg->get_factory():NULL),module_(arg?arg->get_module():NULL)
  {
    if(factory_ && module_) ((ModuleManager*)factory_)->use(module_);
  }

  Plugin::Plugin(const Plugin& obj):
       factory_(obj.factory_),module_(obj.module_)
  {
    if(factory_ && module_) ((ModuleManager*)factory_)->use(module_);
  }

  Plugin::~Plugin(void) {
    if(factory_ && module_) ((ModuleManager*)factory_)->unuse(module_);
  }

  Plugin* PluginsFactory::get_instance(const std::string& kind,PluginArgument* arg,bool search) {
    return get_instance(kind,0,INT_MAX,arg,search);
  }

  Plugin* PluginsFactory::get_instance(const std::string& kind,int version,PluginArgument* arg,bool search) {
    return get_instance(kind,version,version,arg,search);
  }

  Plugin* PluginsFactory::get_instance(const std::string& kind,int min_version,int max_version,PluginArgument* arg,bool search) {
    if(arg) arg->set_factory(this);
    Glib::Mutex::Lock lock(lock_);

    modules_t_::diterator d = modules_;
    for(;d;++d) {
      PluginDescriptor* desc = (*d).first->desc_m;
      desc=find_constructor(desc,kind,min_version,max_version,1);
      if(!desc) continue;
      // Suitable plugin descriptor is found ...
      if(arg) {
        arg->set_module((*d).second->module);
      };
      lock.release();
      Plugin* plugin = desc->instance(arg);
      if(plugin) return plugin;
      // ... but plugin did not instantiate with specified argument
      lock.acquire();
    };

    // Either searching for plugin is enabled
    if(!search) return NULL;

    // Looking for file and especially loading library may take
    // long time. Especially if it involves network operations.
    // So releasing lock. No opertions on modules_ are allowed
    // till lock is re-acquired.
    lock.release();

    // Try to load module of plugin
    // Look for *.apd first by requested plugin kind
    std::string mname = kind;
    AutoPointer<ARCModuleDescriptor> mdesc(probe_descriptor(mname,*this));
    if(!mdesc) {
      logger.msg(VERBOSE, "Could not find loadable module descriptor by name %s",kind);
      return NULL;
    };
    if(!mdesc->contains(kind)) return NULL;
    // Descriptor with suitable name found.
    // Now try to load module directly
    Glib::Module* module = probe_module(kind,*this,logger);
    if (module == NULL) {
      logger.msg(ERROR, "Could not find loadable module by name %s (%s)",kind,strip_newline(Glib::Module::get_last_error()));
      return NULL;
    };
    // Identify table of descriptors
    void *ptr = NULL;
    if(!module->get_symbol(plugins_table_name,ptr)) {
      logger.msg(ERROR, "Module %s is not an ARC plugin (%s)",kind,strip_newline(Glib::Module::get_last_error()));
      unload_module(module,*this);
      return NULL;
    };
    // Try to find plugin in loaded module
    PluginDescriptor* desc = (PluginDescriptor*)ptr;
    for(;;) {
      // Look for plugin descriptor of suitable kind
      desc=find_constructor(desc,kind,min_version,max_version);
      if(!desc) break; // Out of descriptors
      if(arg) arg->set_module(module);
      Plugin* plugin = desc->instance(arg);
      if(plugin) {
        // Plugin instantiated with specified argument.
        // Keep plugin loaded and registered.
        Glib::Module* nmodule = reload_module(module,*this);
        if(!nmodule) {
          logger.msg(ERROR, "Module %s failed to reload (%s)",mname,strip_newline(Glib::Module::get_last_error()));
          // clean up
          delete plugin;
          unload_module(module,*this);
          return NULL;
        };
        module = NULL; // initial handler is not valid anymore
       
        // Re-acqire lock before working with modules_
        lock.acquire();
        // Make descriptor and register it in the cache
        ModuleDesc mdesc_i;
        mdesc_i.name = mname;
        if(mdesc) mdesc->get(mdesc_i.plugins);
        // TODO: handle multiple records with same mname. Is it needed?
        modules_.add(&mdesc_i,nmodule,(PluginDescriptor*)ptr);
        return plugin;
      };
      // Proceede to next descriptor
      ++desc;
    };
    // Out of descriptors. Release module and exit.
    logger.msg(ERROR, "Module %s contains no plugin %s",kind,kind);
    unload_module(module,*this);
    return NULL;
  }

  Plugin* PluginsFactory::get_instance(const std::string& kind,const std::string& name,PluginArgument* arg,bool search) {
    return get_instance(kind,name,0,INT_MAX,arg,search);
  }

  Plugin* PluginsFactory::get_instance(const std::string& kind,const std::string& /* name */,int version,PluginArgument* arg,bool search) {
    return get_instance(kind,version,version,arg,search);
  }

  Plugin* PluginsFactory::get_instance(const std::string& kind,const std::string& name,int min_version,int max_version,PluginArgument* arg,bool search) {
    if(arg) arg->set_factory(this);
    Glib::Mutex::Lock lock(lock_);

    modules_t_::diterator d = modules_;
    for(;d;++d) {
      PluginDescriptor* desc = (*d).first->desc_m;
      desc=find_constructor(desc,kind,name,min_version,max_version,1);
      if(!desc) continue;
      if(arg) {
        arg->set_module((*d).second->module);
      };
      lock.release();
      // If both name and kind are supplied no probing is done
      return desc->instance(arg);
    };

    if(!search) return NULL;

    // Looking for file and especially loading library may take
    // long time. Especially if it involves network operations.
    // So releasing lock. No opertions on modules_ are allowed
    // till lock is re-acquired.
    lock.release();

    // Try to load module - first by name of plugin
    std::string mname = name;
    AutoPointer<ARCModuleDescriptor> mdesc(probe_descriptor(mname,*this));
    if(!mdesc) {
      mname=kind;
      mdesc = probe_descriptor(mname,*this);
      if (!mdesc) {
        logger.msg(VERBOSE, "Could not find loadable module descriptor by name %s or kind %s",name, kind);
        return NULL;
      };
    };
    if(!mdesc->contains(kind,name)) {
      logger.msg(ERROR, "Loadable module %s contains no requested plugin %s of kind %s",mname,name,kind);
      return NULL;
    };
    // Descriptor not found or indicates presence of requested kinds.
    // Now try to load module directly
    Glib::Module* module = probe_module(mname,*this,logger);
    if (module == NULL) {
      logger.msg(ERROR, "Could not find loadable module by names %s and %s (%s)",name,kind,strip_newline(Glib::Module::get_last_error()));
      return NULL;
    };
    // Identify table of descriptors
    void *ptr = NULL;
    if(!module->get_symbol(plugins_table_name,ptr)) {
      logger.msg(ERROR, "Module %s is not an ARC plugin (%s)",mname,strip_newline(Glib::Module::get_last_error()));
      unload_module(module,*this);
      return NULL;
    };
    // Try to find plugin in new table
    PluginDescriptor* desc = find_constructor((PluginDescriptor*)ptr,kind,name,min_version,max_version);
    if(desc) {
      // Keep plugin loaded and registered
      Glib::Module* nmodule = reload_module(module,*this);
      if(!nmodule) {
        logger.msg(ERROR, "Module %s failed to reload (%s)",mname,strip_newline(Glib::Module::get_last_error()));
        unload_module(module,*this);
        return NULL;
      };
      lock.acquire();
      ModuleDesc mdesc_i;
      mdesc_i.name = mname;
      if(mdesc) mdesc->get(mdesc_i.plugins);
      modules_.add(&mdesc_i,nmodule,(PluginDescriptor*)ptr);
      if(arg) arg->set_module(nmodule);
      lock.release();
      return desc->instance(arg);
    };
    logger.msg(ERROR, "Module %s contains no requested plugin %s of kind %s",mname,name,kind);
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

  bool PluginsFactory::load(const std::string& name,const std::string& kind,const std::string& pname) {
    std::list<std::string> kinds;
    std::list<std::string> pnames;
    kinds.push_back(kind);
    pnames.push_back(pname);
    return load(name,kinds,pnames);
  }

  bool PluginsFactory::load(const std::string& name,const std::list<std::string>& kinds) {
    std::list<std::string> pnames;
    return load(name,kinds,pnames);
  }

  bool PluginsFactory::load(const std::string& name,const std::list<std::string>& kinds,const std::list<std::string>& /* pnames */) {
    // In real use-case all combinations of kinds and pnames
    // have no sense. So normally if both are defined each contains
    // only one item.
    if(name.empty()) return false;
    Glib::Module* module = NULL;
    PluginDescriptor* desc = NULL;
    void *ptr = NULL;
    std::string mname;
    Glib::Mutex::Lock lock(lock_);
    // Check if module already loaded
    modules_t_::miterator m = modules_.find(name);
    // Releasing lock in order to avoid locking while loading new module.
    // The iterator stays valid because modules are not unloaded from cache.
    lock.release();
    AutoPointer<ARCModuleDescriptor> mdesc;
    if(m) {
      desc = m->second.get_table();
      if(!desc) return false;
    } else {
      // Try to load module by specified name
      mname = name;
      // First try to find descriptor of module
      mdesc = probe_descriptor(mname,*this);
      if(!mdesc) {
        logger.msg(VERBOSE, "Could not find loadable module descriptor by name %s",name);
        return false;
      };
      if(!mdesc->contains(kinds)) {
        //logger.msg(VERBOSE, "Module %s does not contain plugin(s) of specified kind(s)",mname);
        return false;
      };
      // Now try to load module directly
      module = probe_module(mname,*this,logger);
      if (module == NULL) {
        logger.msg(ERROR, "Could not find loadable module by name %s (%s)",name,strip_newline(Glib::Module::get_last_error()));
        return false;
      };
      // Identify table of descriptors
      if(!module->get_symbol(plugins_table_name,ptr)) {
        logger.msg(ERROR, "Module %s is not an ARC plugin (%s)",mname,strip_newline(Glib::Module::get_last_error()));
        unload_module(module,*this);
        return false;
      };
      desc = (PluginDescriptor*)ptr;
    };
    if(kinds.size() > 0) {
      PluginDescriptor* ndesc = NULL;
      for(std::list<std::string>::const_iterator kind = kinds.begin();
          kind != kinds.end(); ++kind) {
        if(kind->empty()) continue;
        ndesc = find_constructor(desc,*kind,0,INT_MAX);
        if(ndesc) break;
      };
      if(!ndesc) {
        if(module) {
          logger.msg(ERROR, "Module %s does not contain plugin(s) of specified kind(s)",mname);
          unload_module(module,*this);
        };
        return false;
      };
      desc = ndesc;
    };
    if(!mname.empty()) { // this indicates new module is loaded
      Glib::Module* nmodule=reload_module(module,*this);
      if(!nmodule) {
        logger.msg(VERBOSE, "Module %s failed to reload (%s)",mname,strip_newline(Glib::Module::get_last_error()));
        unload_module(module,*this);
        return false;
      };
      // Re-acquire lock before registering new module in cache
      lock.acquire();
      ModuleDesc mdesc_i;
      mdesc_i.name = mname;
      if(mdesc) mdesc->get(mdesc_i.plugins);
      modules_.add(&mdesc_i,nmodule,(PluginDescriptor*)ptr);
    };
    return true;
  }

  bool PluginsFactory::load(const std::list<std::string>& names,const std::string& kind) {
    std::list<std::string> kinds;
    kinds.push_back(kind);
    return load(names,kinds);
  }

  bool PluginsFactory::load(const std::list<std::string>& names,const std::string& kind,const std::string& pname) {
    bool r = false;
    std::list<std::string> kinds;
    std::list<std::string> pnames;
    kinds.push_back(kind);
    pnames.push_back(pname);
    for(std::list<std::string>::const_iterator name = names.begin();
                                name != names.end();++name) {
      if(load(*name,kinds,pnames)) r=true;
    }
    return r;
  }

  bool PluginsFactory::load(const std::list<std::string>& names,const std::list<std::string>& kinds) {
    bool r = false;
    for(std::list<std::string>::const_iterator name = names.begin();
                                name != names.end();++name) {
      if(load(*name,kinds)) r=true;
    }
    return r;
  }

  bool PluginsFactory::scan(const std::string& name, ModuleDesc& desc) {
    ARCModuleDescriptor* mod = probe_descriptor(name,*this);
    if(mod) {
      desc.name = name;
      mod->get(desc.plugins);
      delete mod;
      return true;
    }
    // Descriptor not found
    return false;
  }

  bool PluginsFactory::scan(const std::list<std::string>& names, std::list<ModuleDesc>& descs) {
    bool r = false;
    for(std::list<std::string>::const_iterator name = names.begin();
                                name != names.end();++name) {
      ModuleDesc desc;
      if(scan(*name,desc)) {
        r=true;
        descs.push_back(desc);
      }
    }
    return r;
  }

  void PluginsFactory::report(std::list<ModuleDesc>& descs) {
    modules_t_::miterator m = modules_;
    for(;m;++m) {
      ModuleDesc md;
      md.name = m->first;
      for(std::list<descriptor_t_>::iterator d = m->second.plugins.begin();
                        d != m->second.plugins.end();++m) {
        md.plugins.push_back(d->desc_i);
      };
      descs.push_back(md);
    };
  }

  void PluginsFactory::FilterByKind(const std::string& kind, std::list<ModuleDesc>& mdescs) {
    for (std::list<ModuleDesc>::iterator mdesc = mdescs.begin();
         mdesc != mdescs.end();) {
      for (std::list<PluginDesc>::iterator pdesc = mdesc->plugins.begin(); pdesc != mdesc->plugins.end();) {
        if (pdesc->kind != kind) {
          // Remove plugins from module not of kind.
          pdesc = mdesc->plugins.erase(pdesc);
        }
        else {
          pdesc++;
        }
      }

      if (mdesc->plugins.empty()) {
        // If list is empty, remove module.
        mdesc = mdescs.erase(mdesc);
      }
      else {
        mdesc++;
      }
    }
  }

  PluginsFactory::PluginsFactory(XMLNode cfg): ModuleManager(cfg) {
  }

  PluginArgument::PluginArgument(void): factory_(NULL), module_(NULL) {
  }

  PluginArgument::~PluginArgument(void) {
  }

  PluginsFactory* PluginArgument::get_factory(void) {
    return factory_;
  }

  Glib::Module* PluginArgument::get_module(void) {
    return module_;
  }

  void PluginArgument::set_factory(PluginsFactory* factory) {
    factory_=factory;
  }

  void PluginArgument::set_module(Glib::Module* module) {
    module_=module;
  }


} // namespace Arc

