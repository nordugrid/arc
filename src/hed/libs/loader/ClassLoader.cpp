#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <limits.h>

#include "ClassLoader.h"

namespace Arc {

static identifier_map_t id_map;

ClassLoader* ClassLoader::_instance = NULL;

static void freeClassLoader() {
  delete ClassLoader::getClassLoader();
}

ClassLoader::ClassLoader(Config * cfg) : ModuleManager(cfg){
  if(cfg!=NULL)
    load_all_instances(cfg); 
}

ClassLoader::~ClassLoader(){
  //Delete the list (not delete the element), gurantee the caller will not need to delete the elements. If the list is automatically deleted by caller 
  //(e.g. main function), there will be "double free or corruption (fasttop)", because ModuleManager also delete the same memory space of the element.
  if(!id_map.empty())
    id_map.clear();
}

ClassLoader* ClassLoader::getClassLoader(Config* cfg) {
  if(_instance == NULL && cfg == NULL) {
    std::cout<<"Configuration should not be NULL at the initiation step of singleton ClassLoader"<<std::endl;
    return NULL;
  }
  if(_instance == NULL && cfg != NULL) {
    _instance = new ClassLoader(cfg);
    atexit(freeClassLoader);
  }
  if(_instance != NULL && cfg !=NULL) {
    _instance->setCfg(cfg);
    _instance->load_all_instances(cfg);
  }
  return _instance;
}

void ClassLoader::load_all_instances(Config *cfg){
  XMLNode root = (*cfg).GetRoot();

  if(!MatchXMLName(root,"ArcConfig")) return;
  for (int i = 0;;++i) {
    XMLNode plugins = root["Plugins"][i];
    if (!plugins) {
      break;
    }
    std::string share_lib_name = (std::string)(plugins.Attribute("Name"));
    
    Glib::Module *module = NULL;
    if(!(share_lib_name.empty())){
      module = ModuleManager::load(share_lib_name);
      if(module == NULL){
        std::cout<<"Loadable module "<<share_lib_name<<" could not be loaded"<<std::endl;
        return;
      }
    }

    for(int j = 0;;++j){
      XMLNode plugin = plugins.Child(j);
      if(!plugin){
        break;
      }
      if (MatchXMLName(plugin, "Plugin")) {
        std::string plugin_name = (std::string)(plugin.Attribute("Name"));
        void *ptr = NULL;
        if (module->get_symbol(plugin_name.c_str(), ptr)) {
          //If there is a existing "Plugin" with the same plugin_name as the inserting one, the original one
          //will be covered.
          id_map[plugin_name]=ptr;
        }
        else 
          std::cout<<"There is no " << plugin_name <<" type plugin"<<std::endl;
           
      }
    }
  }

}

LoadableClass* ClassLoader::Instance(const std::string& classId, void* arg){
  identifier_map_t::iterator it;
  void* ptr;
  for(it=id_map.begin(); it!=id_map.end(); ++it){
    ptr =(*it).second;
    for(loader_descriptor* desc = (loader_descriptor*)ptr; !((desc->name)==NULL); ++desc) {
      //std::cout<<"size:"<<id_map.size()<<" name:"<<desc->name<<"------classid:"<<classId<<std::endl;
      if(desc->name == classId){ 
        loader_descriptor &descriptor =*desc; 
        LoadableClass * res = NULL;
        res = (*descriptor.get_instance)(arg);
        return res;
      }
    } 
  }
  return NULL;
}

} // namespace Arc

