#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm.h>

#include <arc/Logger.h>
#include <arc/StringConv.h>

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

  class ARCModuleDescriptor {
   private:
    bool valid;
    class ARCPluginDescriptor {
     public:
      std::string name;
      std::string kind;
      uint32_t version;
      bool valid;
      ARCPluginDescriptor(std::ifstream& in):valid(false) {
        if(!in) return;
        std::string line;
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
          } else if(tag == "version") {
            if(!stringto(line,version)) return;
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
        pd.version = desc->version;
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


  static ARCModuleDescriptor* probe_descriptor(std::string name,ModuleManager& manager) {
    std::string::size_type p = 0;
    for(;;) {
      p=name.find(':',p);
      if(p == std::string::npos) break;
      name.replace(p,1,"_");
      ++p;
    };
    std::string path = manager.find(name);
    if(path.empty()) return NULL;
    replace_file_suffix(path,"apd");
    std::ifstream in(path.c_str());
    ARCModuleDescriptor* md = new ARCModuleDescriptor(in);
    if(!(*md)) {
      delete md;
      return NULL;
    };
    return md;
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

  Plugin* PluginsFactory::get_instance(const std::string& kind,PluginArgument* arg,bool search) {
    return get_instance(kind,0,INT_MAX,arg,search);
  }

  Plugin* PluginsFactory::get_instance(const std::string& kind,int version,PluginArgument* arg,bool search) {
    return get_instance(kind,version,version,arg,search);
  }

  Plugin* PluginsFactory::get_instance(const std::string& kind,int min_version,int max_version,PluginArgument* arg,bool search) {
    if(arg) arg->set_factory(this);
    Glib::Mutex::Lock lock(lock_);
    descriptors_t_::iterator i = descriptors_.begin();
    for(;i != descriptors_.end();++i) {
      PluginDescriptor* desc = i->second;
      for(;;) {
        desc=find_constructor(desc,kind,min_version,max_version);
        if(!desc) break;
        if(arg) {
          modules_t_::iterator m = modules_.find(i->first);
          if(m != modules_.end()) {
            arg->set_module(m->second);
          } else {
            arg->set_module(NULL);
          };
        };
        lock.release();
        Plugin* plugin = desc->instance(arg);
        if(plugin) return plugin;
        lock.acquire();
        ++desc;
      };
    };
    if(!search) return NULL;
    // Try to load module of plugin
    std::string mname = kind;
    ARCModuleDescriptor* mdesc = probe_descriptor(mname,*this);
    if(mdesc) {
      if(!mdesc->contains(kind)) {
        delete mdesc;
        return NULL;
      };
      delete mdesc;
      mdesc = NULL;
    };
    // Descriptor not found or indicates presence of requested kinds.
    if(!try_load_) {
      logger.msg(ERROR, "Could not find loadable module descriptor by name %s",kind);
      return NULL;
    };
    // Now try to load module directly
    Glib::Module* module = probe_module(kind,*this);
    if (module == NULL) {
      logger.msg(ERROR, "Could not find loadable module by name %s (%s)",kind,strip_newline(Glib::Module::get_last_error()));
      return NULL;
    };
    // Identify table of descriptors
    void *ptr = NULL;
    if(!module->get_symbol(plugins_table_name,ptr)) {
      logger.msg(VERBOSE, "Module %s is not an ARC plugin (%s)",kind,strip_newline(Glib::Module::get_last_error()));
      unload_module(module,*this);
      return NULL;
    };
    // Try to find plugin in new table
    PluginDescriptor* desc = (PluginDescriptor*)ptr;
    for(;;) {
      desc=find_constructor(desc,kind,min_version,max_version);
      if(!desc) break;
      if(arg) arg->set_module(module);
      lock.release();
      Plugin* plugin = desc->instance(arg);
      lock.acquire();
      if(plugin) {
        // Keep plugin loaded and registered
        Glib::Module* nmodule = reload_module(module,*this);
        if(!nmodule) {
          logger.msg(VERBOSE, "Module %s failed to reload (%s)",mname,strip_newline(Glib::Module::get_last_error()));
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

  Plugin* PluginsFactory::get_instance(const std::string& kind,const std::string& name,PluginArgument* arg,bool search) {
    return get_instance(kind,name,0,INT_MAX,arg,search);
  }

  Plugin* PluginsFactory::get_instance(const std::string& kind,const std::string& /* name */,int version,PluginArgument* arg,bool search) {
    return get_instance(kind,version,version,arg,search);
  }

  Plugin* PluginsFactory::get_instance(const std::string& kind,const std::string& name,int min_version,int max_version,PluginArgument* arg,bool search) {
    if(arg) arg->set_factory(this);
    Glib::Mutex::Lock lock(lock_);
    descriptors_t_::iterator i = descriptors_.begin();
    for(;i != descriptors_.end();++i) {
      PluginDescriptor* desc = find_constructor(i->second,kind,name,min_version,max_version);
      if(arg) {
        modules_t_::iterator m = modules_.find(i->first);
        if(m != modules_.end()) {
          arg->set_module(m->second);
        } else {
          arg->set_module(NULL);
        };
      };
      if(desc) {
        lock.release();
        return desc->instance(arg);
      }
    };
    if(!search) return NULL;
    // Try to load module - first by name of plugin
    std::string mname = name;
    ARCModuleDescriptor* mdesc = probe_descriptor(mname,*this);
    if(mdesc) {
      if(!mdesc->contains(kind,name)) {
        delete mdesc;
        logger.msg(ERROR, "Loadable module %s contains no requested plugin %s of kind %s",mname,name,kind);
        return NULL;
      };
      delete mdesc;
      mdesc = NULL;
    };
    // Descriptor not found or indicates presence of requested kinds.
    // Now try to load module directly
    Glib::Module* module = try_load_?probe_module(name,*this):NULL;
    if (module == NULL) {
      // Then by kind of plugin
      mname=kind;
      mdesc = probe_descriptor(mname,*this);
      if(mdesc) {
        if(!mdesc->contains(kind,name)) {
          delete mdesc;
          logger.msg(ERROR, "Loadable module %s contains no requested plugin %s of kind %s",mname,name,kind);
          return NULL;
        };
        delete mdesc;
        mdesc = NULL;
      };
      if(!try_load_) {
        logger.msg(ERROR, "Could not find loadable module descriptor by names %s and %s",name,kind);
        return NULL;
      };
      // Descriptor not found or indicates presence of requested kinds.
      // Now try to load module directly
      module=probe_module(kind,*this);
      logger.msg(ERROR, "Could not find loadable module by names %s and %s (%s)",name,kind,strip_newline(Glib::Module::get_last_error()));
      return NULL;
    };
    // Identify table of descriptors
    void *ptr = NULL;
    if(!module->get_symbol(plugins_table_name,ptr)) {
      logger.msg(VERBOSE, "Module %s is not an ARC plugin (%s)",mname,strip_newline(Glib::Module::get_last_error()));
      unload_module(module,*this);
      return NULL;
    };
    // Try to find plugin in new table
    PluginDescriptor* desc = find_constructor((PluginDescriptor*)ptr,kind,name,min_version,max_version);
    if(desc) {
      // Keep plugin loaded and registered
      Glib::Module* nmodule = reload_module(module,*this);
      if(!nmodule) {
        logger.msg(VERBOSE, "Module %s failed to reload (%s)",mname,strip_newline(Glib::Module::get_last_error()));
        unload_module(module,*this);
        return false;
      };
      descriptors_[mname]=(PluginDescriptor*)ptr;
      modules_[mname]=nmodule;
      //descriptors_.push_back((PluginDescriptor*)ptr);
      //modules_.push_back(module);
      if(arg) arg->set_module(nmodule);
      lock.release();
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
    std::list<std::string> pnames;
    return load(name,kinds,pnames);
  }

  bool PluginsFactory::load(const std::string& name,const std::list<std::string>& kinds,const std::list<std::string>& /* pnames */) {
    // In real use-case all combinations of kinds and pnames
    // have no sense. So normally if both are defined each contains
    // only onr item.
    if(name.empty()) return false;
    Glib::Module* module = NULL;
    PluginDescriptor* desc = NULL;
    void *ptr = NULL;
    std::string mname;
    Glib::Mutex::Lock lock(lock_);
    // Check if module already loaded
    descriptors_t_::iterator d = descriptors_.find(name);
    if(d != descriptors_.end()) {
      desc = d->second;
      if(!desc) return false;
    } else {
      // Try to load module by specified name
      mname = name;
      // First try to find descriptor of module
      ARCModuleDescriptor* mdesc = probe_descriptor(mname,*this);
      if(mdesc) {
        if(!mdesc->contains(kinds)) {
          //logger.msg(VERBOSE, "Module %s does not contain plugin(s) of specified kind(s)",mname);
          delete mdesc;
          return false;
        };
        delete mdesc;
        mdesc = NULL;
      };
      if(!try_load_) {
        logger.msg(ERROR, "Could not find loadable module descriptor by name %s",name);
        return false;
      };
      // Descriptor not found or indicates presence of requested kinds.
      // Descriptor not found or indicates presence of requested kinds.
      // Now try to load module directly
      module = probe_module(mname,*this);
      if (module == NULL) {
        logger.msg(ERROR, "Could not find loadable module by name %s (%s)",name,strip_newline(Glib::Module::get_last_error()));
        return false;
      };
      // Identify table of descriptors
      if(!module->get_symbol(plugins_table_name,ptr)) {
        logger.msg(VERBOSE, "Module %s is not an ARC plugin (%s)",mname,strip_newline(Glib::Module::get_last_error()));
        unload_module(module,*this);
        return false;
      };
      desc = (PluginDescriptor*)ptr;
    };
    if(kinds.size() > 0) {
      for(std::list<std::string>::const_iterator kind = kinds.begin();
          kind != kinds.end(); ++kind) {
        if(kind->empty()) continue;
        desc=find_constructor(desc,*kind,0,INT_MAX);
        if(desc) break;
      };
      if(!desc) {
        //logger.msg(VERBOSE, "Module %s does not contain plugin(s) of specified kind(s)",mname);
        if(module) unload_module(module,*this);
        return false;
      };
    };
    if(!mname.empty()) {
      Glib::Module* nmodule=reload_module(module,*this);
      if(!nmodule) {
        logger.msg(VERBOSE, "Module %s failed to reload (%s)",mname,strip_newline(Glib::Module::get_last_error()));
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
      return true;
    }
    // Descriptor not found
    if(!try_load_) return false;
    // Now try to load module directly
    Glib::Module* module = probe_module(name,*this);
    if (module == NULL) return false;
    // Identify table of descriptors
    void *ptr = NULL;
    if(!module->get_symbol(plugins_table_name,ptr)) {
      unload_module(module,*this);
      return false;
    };
    PluginDescriptor* d = (PluginDescriptor*)ptr;
    if(!d) {
      unload_module(module,*this);
      return false;
    };
    for(;(d->kind) && (d->name);++d) {
      PluginDesc pd;
      pd.name = d->name;
      pd.kind = d->kind;
      pd.version = d->version;
      desc.plugins.push_back(pd);
    };
    return true;
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
    for(descriptors_t_::iterator ds = descriptors_.begin();
               ds != descriptors_.end(); ++ds) {
      PluginDescriptor* d = ds->second;
      if(!d) continue;
      ModuleDesc md;
      md.name = ds->first;
      for(;(d->kind) && (d->name);++d) {
        PluginDesc pd;
        pd.name = d->name;
        pd.kind = d->kind;
        pd.version = d->version;
        md.plugins.push_back(pd);
      };
      descs.push_back(md);
    }
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

  PluginsFactory::PluginsFactory(XMLNode cfg): ModuleManager(cfg),
                                               try_load_(true) {
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

