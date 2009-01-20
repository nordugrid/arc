#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ModuleManager.h"
#include "Loader.h"

namespace Arc {
ModuleManager::ModuleManager(const Arc::Config *cfg)
{ 
  if(cfg==NULL) return;
  if(!(*cfg)) return;
  Loader::logger.msg(VERBOSE, "Module Manager Init");
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
    std::map<std::string, Glib::Module *>::iterator i;
    for(i=plugin_cache.begin();i!=plugin_cache.end();++i) {
        delete i->second;
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
      delete p->second;
      plugin_cache.erase(p);
      break;
    }
  }
}

void ModuleManager::unload(const std::string& name)
{
  plugin_cache_t::iterator p = plugin_cache.find(name);
  if (p != plugin_cache.end()) {
    delete p->second;
    plugin_cache.erase(p);
  }
}

Glib::Module *ModuleManager::load(const std::string& name,bool probe /*,bool reload*/ )
{
  if (!Glib::Module::get_supported()) {
    return false;
  }
  // find name in plugin_cache 
  //if(!reload) {
    if (plugin_cache.find(name) != plugin_cache.end()) {
      Loader::logger.msg(VERBOSE, "Found %s in cache", name);
      return plugin_cache[name];
    }
  //}
  std::string path = findLocation(name);
  if(path.empty()) {
    Loader::logger.msg(DEBUG, "Could not locate module %s", name);
    return NULL;
  };
  Glib::ModuleFlags flags = Glib::ModuleFlags(0);
#ifdef HAVE_GLIBMM_BIND_LOCAL
  //flags|=Glib::MODULE_BIND_LOCAL;
  if(probe) flags|=Glib::MODULE_BIND_LAZY | Glib::MODULE_BIND_LOCAL;
#else
  if(probe) flags|=Glib::MODULE_BIND_LAZY;
#endif
  Glib::Module *module = new Glib::Module(path,flags);
  if ((!module) || (!(*module))) {
    Loader::logger.msg(ERROR, Glib::Module::get_last_error());
    if(module) delete module;
    return NULL;
  }
  Loader::logger.msg(VERBOSE, "Loaded %s", path);
  plugin_cache[name] = module;
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
    Loader::logger.msg(ERROR, Glib::Module::get_last_error());
    if(module) delete module;
    return NULL;
  }
  p->second=module;
  delete omodule;
  return module; 
}

void ModuleManager::setCfg (Arc::Config *cfg) {
  if(cfg==NULL) return;
  if(!(*cfg)) return;
  Loader::logger.msg(INFO, "Module Manager Init by ModuleManager::setCfg");

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

} // namespace Arc

