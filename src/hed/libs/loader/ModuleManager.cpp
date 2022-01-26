#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <arc/ArcLocation.h>
#include <arc/loader/ModuleManager.h>

namespace Arc {
  Logger ModuleManager::logger(Logger::rootLogger, "ModuleManager");

static std::string strip_newline(const std::string& str) {
  std::string s(str);
  std::string::size_type p = 0;
  while((p=s.find('\r',p)) != std::string::npos) s[p]=' ';
  p=0;
  while((p=s.find('\n',p)) != std::string::npos) s[p]=' ';
  return s;
}

ModuleManager::ModuleManager(XMLNode cfg)
{
  if(!cfg) return;
  ModuleManager::logger.msg(DEBUG, "Module Manager Init");
  if(!MatchXMLName(cfg,"ArcConfig")) return;
  XMLNode mm = cfg["ModuleManager"];
  for (int n = 0;;++n) {
    XMLNode path = mm.Child(n);
    if (!path) {
      break;
    }
    if (MatchXMLName(path, "Path")) {
      plugin_dir.push_back((std::string)path);
    }
  }
  if (plugin_dir.empty()) {
    plugin_dir = ArcLocation::GetPlugins();
  }
}

ModuleManager::~ModuleManager(void)
{
  Glib::Mutex::Lock lock(mlock);
  // Try to unload all modules
  // Remove unloaded plugins from cache
  for(plugin_cache_t::iterator i = plugin_cache.begin(); i != plugin_cache.end();) {
    while(i->second.unload(this) > 0) { };
    if(i->second) {
      // module is not unloaded only if it is in use according to usage counter
      ++i;
    } else {
      plugin_cache.erase(i);
      i = plugin_cache.begin(); // for map erase does not return iterator
    }
  }
  for(plugin_trash_t::iterator i = plugin_trash.begin(); i != plugin_trash.end();) {
    while(i->unload(this) > 0) { };
    if(*i) {
      ++i;
    } else {
      i = plugin_trash.erase(i);
    }
  }
  // exit only when all plugins unloaded
  if(plugin_cache.empty() && plugin_trash.empty()) return;
  // otherwise wait for plugins to be released
  logger.msg(WARNING, "Busy plugins found while unloading Module Manager. Waiting for them to be released.");
  for(;;) {
    // wait for plugins to be released
    lock.release();
    sleep(1);
    lock.acquire();
    // Check again
    // Just in case something called load() - unloading them again
    for(plugin_cache_t::iterator i = plugin_cache.begin(); i != plugin_cache.end();) {
      while(i->second.unload(this) > 0) { };
      if(i->second) {
        ++i;
      } else {
        plugin_cache.erase(i);
        i = plugin_cache.begin(); // for map erase does not return iterator
      }
    }
    for(plugin_trash_t::iterator i = plugin_trash.begin(); i != plugin_trash.end();) {
      while(i->unload(this) > 0) { };
      if(*i) {
        ++i;
      } else {
        i = plugin_trash.erase(i);
      }
    }
    if(plugin_cache.empty() && plugin_trash.empty()) return;
  };
}

std::string ModuleManager::findLocation(const std::string& name)
{
  Glib::Mutex::Lock lock(mlock);
  std::string path;
  std::list<std::string>::const_iterator i = plugin_dir.begin();
  for (; i != plugin_dir.end(); i++) {
    path = Glib::Module::build_path(*i, name);
    // Loader::logger.msg(VERBOSE, "Try load %s", path);
    int file = open(path.c_str(), O_RDONLY);
    if (file == -1) {
      continue;
    } else {
      close(file);
      break;
    }
  }
  if(i == plugin_dir.end()) path="";
  return path;
}

void ModuleManager::unload(Glib::Module *module)
{
  Glib::Mutex::Lock lock(mlock);
  for(plugin_cache_t::iterator p = plugin_cache.begin();
                               p!=plugin_cache.end();++p) {
    if(p->second == module) {
      p->second.unload(this);
      if(!(p->second)) {
        plugin_cache.erase(p);
      }
      return;
    }
  }
  for(plugin_trash_t::iterator p = plugin_trash.begin();
                               p!=plugin_trash.end();++p) {
    if(*p == module) {
      p->unload(NULL); // Do not call destructor for trashed module
      if(!(*p)) {
        plugin_trash.erase(p);
      }
      return;
    }
  }
}


void ModuleManager::use(Glib::Module *module)
{
  Glib::Mutex::Lock lock(mlock);
  for(plugin_cache_t::iterator p = plugin_cache.begin();
                               p!=plugin_cache.end();++p) {
    if(p->second == module) {
      p->second.use();
      return;
    }
  }
  for(plugin_trash_t::iterator p = plugin_trash.begin();
                               p!=plugin_trash.end();++p) {
    if(*p == module) {
      p->use();
      return;
    }
  }
}

void ModuleManager::unuse(Glib::Module *module)
{
  Glib::Mutex::Lock lock(mlock);
  for(plugin_cache_t::iterator p = plugin_cache.begin();
                               p!=plugin_cache.end();++p) {
    if(p->second == module) {
      p->second.unuse();
      if(!(p->second)) {
        plugin_cache.erase(p);
      }
      return;
    }
  }
  for(plugin_trash_t::iterator p = plugin_trash.begin();
                               p!=plugin_trash.end();++p) {
    if(*p == module) {
      p->unuse();
      if(!(*p)) {
        plugin_trash.erase(p);
      }
      return;
    }
  }
}

std::string ModuleManager::find(const std::string& name)
{
  return findLocation(name);
}

Glib::Module* ModuleManager::load(const std::string& name,bool probe)
{
  if (!Glib::Module::get_supported()) {
    return NULL;
  }
  // find name in plugin_cache
  {
    Glib::Mutex::Lock lock(mlock);
    plugin_cache_t::iterator p = plugin_cache.find(name);
    if (p != plugin_cache.end()) {
      ModuleManager::logger.msg(DEBUG, "Found %s in cache", name);
      p->second.load();
      return static_cast<Glib::Module*>(p->second);
    }
  }
  std::string path = findLocation(name);
  if(path.empty()) {
    ModuleManager::logger.msg(VERBOSE, "Could not locate module %s in following paths:", name);
    Glib::Mutex::Lock lock(mlock);
    std::list<std::string>::const_iterator i = plugin_dir.begin();
    for (; i != plugin_dir.end(); i++) {
      ModuleManager::logger.msg(VERBOSE, "\t%s", *i);
    }
    return NULL;
  };
  // race!
  Glib::Mutex::Lock lock(mlock);
  Glib::ModuleFlags flags = Glib::ModuleFlags(0);
  if(probe) flags|=Glib::MODULE_BIND_LAZY;
  Glib::Module *module = new Glib::Module(path,flags);
  if ((!module) || (!(*module))) {
    ModuleManager::logger.msg(ERROR, strip_newline(Glib::Module::get_last_error()));
    if(module) delete module;
    return NULL;
  }
  ModuleManager::logger.msg(DEBUG, "Loaded %s", path);
  (plugin_cache[name] = module).load();
  void* func = NULL;
  if(!module->get_symbol(ARC_MODULE_CONSTRUCTOR_SYMB,func)) func = NULL;
  if(func) {
    plugin_cache[name].use();
    lock.release(); // Avoid deadlock if manager called from module constructor
    (*(arc_module_constructor_func)func)(module,this);
    lock.acquire();
    plugin_cache[name].unuse();
  }
  return module;
}

Glib::Module* ModuleManager::reload(Glib::Module* omodule)
{
  Glib::Mutex::Lock lock(mlock);
  plugin_cache_t::iterator p = plugin_cache.begin();
  for(;p!=plugin_cache.end();++p) {
    if(p->second == omodule) break;
  }
  if(p==plugin_cache.end()) return NULL;
  // TODO: avoid reloading modules which are already properly loaded
  Glib::ModuleFlags flags = Glib::ModuleFlags(0);
  //flags|=Glib::MODULE_BIND_LOCAL;
  Glib::Module *module = new Glib::Module(omodule->get_name(),flags);
  if ((!module) || (!(*module))) {
    ModuleManager::logger.msg(ERROR, strip_newline(Glib::Module::get_last_error()));
    if(module) delete module;
    return NULL;
  }
  // Move existing module into trash list for later removal and
  // store handle in current entry.
  // Trashed module keeps load counter. But usage counter stays in cached entry.
  LoadableModuleDescription trashed;
  p->second.shift(module, trashed);
  if (trashed) {
    plugin_trash.push_back(trashed);
  }
  return module;
}

void ModuleManager::setCfg (XMLNode cfg) {
  if(!cfg) return;
  ModuleManager::logger.msg(DEBUG, "Module Manager Init by ModuleManager::setCfg");

  if(!MatchXMLName(cfg,"ArcConfig")) return;
  XMLNode mm = cfg["ModuleManager"];
  for (int n = 0;;++n) {
    XMLNode path = mm.Child(n);
    if (!path) {
      break;
    }
    if (MatchXMLName(path, "Path")) {
      Glib::Mutex::Lock lock(mlock);
      //std::cout<<"Size:"<<plugin_dir.size()<<"plugin cache size:"<<plugin_cache.size()<<std::endl;
      std::list<std::string>::const_iterator it;
      for( it = plugin_dir.begin(); it != plugin_dir.end(); it++){
        //std::cout<<(std::string)path<<"*********"<<(*it)<<std::endl;
        if(((*it).compare((std::string)path)) == 0)break;
      }
      if(it == plugin_dir.end()) plugin_dir.push_back((std::string)path);
    }
  }
  Glib::Mutex::Lock lock(mlock);
  if (plugin_dir.empty()) {
    plugin_dir = ArcLocation::GetPlugins();
  }
}

bool ModuleManager::makePersistent(const std::string& name) {
  if (!Glib::Module::get_supported()) {
    return false;
  }
  // find name in plugin_cache
  {
    Glib::Mutex::Lock lock(mlock);
    plugin_cache_t::iterator p = plugin_cache.find(name);
    if (p != plugin_cache.end()) {
      p->second.makePersistent();
      ModuleManager::logger.msg(DEBUG, "%s made persistent", name);
      return true;
    }
  }
  ModuleManager::logger.msg(DEBUG, "Not found %s in cache", name);
  return false;
}

bool ModuleManager::makePersistent(Glib::Module* module) {
  Glib::Mutex::Lock lock(mlock);
  for(plugin_cache_t::iterator p = plugin_cache.begin();
                               p!=plugin_cache.end();++p) {
    if(p->second == module) {
      ModuleManager::logger.msg(DEBUG, "%s made persistent", p->first);
      p->second.makePersistent();
      return true;
    }
  }
  ModuleManager::logger.msg(DEBUG, "Specified module not found in cache");
  return false;
}


ModuleManager::LoadableModuleDescription::LoadableModuleDescription(void):
                                            module(NULL),count(0),usage_count(0) {
}

ModuleManager::LoadableModuleDescription::LoadableModuleDescription(Glib::Module* m):
                                            module(m),count(0),usage_count(0) {
}

void ModuleManager::LoadableModuleDescription::check_unload(ModuleManager* manager) {
  if((count <= 0) && (usage_count <= 0) && module) {
    void* func = NULL;
    if(!module->get_symbol(ARC_MODULE_DESTRUCTOR_SYMB,func)) func = NULL;
    if(func && manager) {
      use();
      manager->mlock.unlock();
      (*(arc_module_destructor_func)func)(module,manager);
      manager->mlock.lock();
      unuse();
    }
    delete module;
    module=NULL;
  }
}

ModuleManager::LoadableModuleDescription& ModuleManager::LoadableModuleDescription::operator=(Glib::Module* m) {
  module=m;
  return *this;
}

ModuleManager::LoadableModuleDescription& ModuleManager::LoadableModuleDescription::operator=(const LoadableModuleDescription& d) {
  module=d.module; count=d.count; usage_count=d.usage_count;
  return *this;
}

int ModuleManager::LoadableModuleDescription::load(void) {
  ++count;
  return count;
}

int ModuleManager::LoadableModuleDescription::unload(ModuleManager* manager) {
  if(count > 0) --count;
  check_unload(manager);
  return count;
}

void ModuleManager::LoadableModuleDescription::shift(Glib::Module* source, LoadableModuleDescription& target) {
  target.module = module;
  target.count = count;
  module = source;
  count = 0;  
  load(); // accepting new module handler
  target.unload(NULL); // removing reference taken by new module
}

} // namespace Arc

