#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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
  
ModuleManager::ModuleManager(const Config *cfg)
{ 
  if(cfg==NULL) return;
  if(!(*cfg)) return;
  ModuleManager::logger.msg(VERBOSE, "Module Manager Init");
  if(!MatchXMLName(*cfg,"ArcConfig")) return;
  XMLNode mm = (*cfg)["ModuleManager"];
  for (int n = 0;;++n) {
    XMLNode path = mm.Child(n);
    if (!path) {
      break;
    }
    if (MatchXMLName(path, "Path")) {
      plugin_dir.push_back((std::string)path);
    }
  }
}

ModuleManager::~ModuleManager(void)
{
    // removes all element from cache
    plugin_cache_t::iterator i;
    for(i=plugin_cache.begin();i!=plugin_cache.end();++i) {
        while(i->second.unload() > 0) { };
    };
    plugin_cache.clear();
}

std::string ModuleManager::findLocation(const std::string& name)
{
  std::string path;
  std::vector<std::string>::const_iterator i = plugin_dir.begin();
  for (; i != plugin_dir.end(); i++) {
    path = Glib::Module::build_path(*i, name);
    // Loader::logger.msg(DEBUG, "Try load %s", path);
    FILE *file = fopen(path.c_str(), "r");
    if (file == NULL) {
      continue;
    } else {
      fclose(file);
      break;
    }
  }
  if(i == plugin_dir.end()) path="";
  return path;
}

void ModuleManager::unload(Glib::Module *module)
{
  for(plugin_cache_t::iterator p = plugin_cache.begin();
                               p!=plugin_cache.end();++p) {
    if(p->second == module) {
      if(p->second.unload() <= 0) {
        plugin_cache.erase(p);
      }      
      break;
    }
  }
}

void ModuleManager::unload(const std::string& name)
{
  plugin_cache_t::iterator p = plugin_cache.find(name);
  if (p != plugin_cache.end()) {
    if(p->second.unload() <= 0) {
      plugin_cache.erase(p);
    }
  }
}

Glib::Module *ModuleManager::load(const std::string& name,bool probe /*,bool reload*/ )
{
  if (!Glib::Module::get_supported()) {
    return false;
  }
  // find name in plugin_cache 
  {
    plugin_cache_t::iterator p = plugin_cache.find(name);
    if (p != plugin_cache.end()) {
      ModuleManager::logger.msg(VERBOSE, "Found %s in cache", name);
      p->second.load();
      return static_cast<Glib::Module*>(p->second);
    }
  }
  std::string path = findLocation(name);
  if(path.empty()) {
    ModuleManager::logger.msg(DEBUG, "Could not locate module %s", name);
    return NULL;
  };
  Glib::ModuleFlags flags = Glib::ModuleFlags(0);
  if(probe) flags|=Glib::MODULE_BIND_LAZY;
  Glib::Module *module = new Glib::Module(path,flags);
  if ((!module) || (!(*module))) {
    ModuleManager::logger.msg(ERROR, strip_newline(Glib::Module::get_last_error()));
    if(module) delete module;
    return NULL;
  }
  ModuleManager::logger.msg(VERBOSE, "Loaded %s", path);
  (plugin_cache[name] = module).load();
  return module;
}

Glib::Module* ModuleManager::reload(Glib::Module* omodule)
{
  plugin_cache_t::iterator p = plugin_cache.begin();
  for(;p!=plugin_cache.end();++p) {
    if(p->second == omodule) break;
  }
  if(p==plugin_cache.end()) return NULL;
  Glib::ModuleFlags flags = Glib::ModuleFlags(0);
  //flags|=Glib::MODULE_BIND_LOCAL;
  Glib::Module *module = new Glib::Module(omodule->get_name(),flags);
  if ((!module) || (!(*module))) {
    ModuleManager::logger.msg(ERROR, strip_newline(Glib::Module::get_last_error()));
    if(module) delete module;
    return NULL;
  }
  p->second=module;
  delete omodule;
  return module; 
}

void ModuleManager::setCfg (Config *cfg) {
  if(cfg==NULL) return;
  if(!(*cfg)) return;
  ModuleManager::logger.msg(INFO, "Module Manager Init by ModuleManager::setCfg");

  if(!MatchXMLName(*cfg,"ArcConfig")) return;
  XMLNode mm = (*cfg)["ModuleManager"];
  for (int n = 0;;++n) {
    XMLNode path = mm.Child(n);
    if (!path) {
      break;
    }
    if (MatchXMLName(path, "Path")) {
      //std::cout<<"Size:"<<plugin_dir.size()<<"plugin cache size:"<<plugin_cache.size()<<std::endl;
      std::vector<std::string>::const_iterator it;
      for( it = plugin_dir.begin(); it != plugin_dir.end(); it++){
        //std::cout<<(std::string)path<<"*********"<<(*it)<<std::endl;
        if(((*it).compare((std::string)path)) == 0)break;
      }
      if(it == plugin_dir.end())
        plugin_dir.push_back((std::string)path);
    }
  }
}

bool ModuleManager::makePersistent(const std::string& name) {
  if (!Glib::Module::get_supported()) {
    return false;
  }
  // find name in plugin_cache
  {
    plugin_cache_t::iterator p = plugin_cache.find(name);
    if (p != plugin_cache.end()) {
      p->second.makePersistent();
      ModuleManager::logger.msg(VERBOSE, "%s made persistent", name);
      return true;
    }
  }
  ModuleManager::logger.msg(VERBOSE, "Not found %s in cache", name);
  return false;
}

bool ModuleManager::makePersistent(Glib::Module* module) {
  for(plugin_cache_t::iterator p = plugin_cache.begin();
                               p!=plugin_cache.end();++p) {
    if(p->second == module) {
      ModuleManager::logger.msg(VERBOSE, "%s made persistent", p->first);
      p->second.makePersistent();
      return true;
    }
  }
  ModuleManager::logger.msg(VERBOSE, "Specified module not found in cache");
  return false;
}

} // namespace Arc

