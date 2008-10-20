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
  Loader::logger.msg(DEBUG, "Module Manager Init");
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

Glib::Module *ModuleManager::load(const std::string& name,bool load_local,bool reload)
{
  if (!Glib::Module::get_supported()) {
    return false;
  }
  // find name in plugin_cache 
  if(!reload) {
    if (plugin_cache.find(name) != plugin_cache.end()) {
      Loader::logger.msg(DEBUG, "Found %s in cache", name);
      return plugin_cache[name];
    }
  }
  std::string path = findLocation(name);
  if(path.empty()) {
    Loader::logger.msg(DEBUG, "Could not locate module %s", name);
    return NULL;
  };
#ifdef HAVE_GLIBMM_BIND_LOCAL
  Glib::Module *module = new Glib::Module(path,load_local?Glib::MODULE_BIND_LOCAL:(Glib::ModuleFlags(0)));
#else
  Glib::Module *module = new Glib::Module(path);
#endif
  if ((!module) || (!(*module))) {
    Loader::logger.msg(ERROR, Glib::Module::get_last_error());
    if(module) delete module;
    return NULL;
  }
  Loader::logger.msg(DEBUG, "Loaded %s", path);
  plugin_cache[name] = module;
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

