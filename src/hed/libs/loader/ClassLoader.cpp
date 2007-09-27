#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <limits.h>

#include "ClassLoader.h"

namespace Arc {

typedef std::list<void*> identifier_list_t;
static identifier_list_t id_list;

ClassLoader::ClassLoader(Config * cfg) : ModuleManager(cfg){
  if(cfg!=NULL)
    load_all_instances(cfg); 
}

ClassLoader::~ClassLoader(){
  //Delete the list (not delete the element), gurantee the caller will not need to delete the elements. If the list is automatically deleted by caller 
  //(e.g. main function), there will be "double free or corruption (fasttop)", because ModuleManager also delete the same memory space of the element.
  while (!id_list.empty())
  {
    id_list.pop_back();
  }
}

void ClassLoader::load_all_instances(Config *cfg){

  XMLNode root = (*cfg)["ArcConfig"];
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
        std::cout<<"Share library"<<share_lib_name<<"could not be loaded"<<std::endl;
        return;
      }
    }

    for(int j = 0;;++j){
      XMLNode plugin = plugins.Child(j);
      if(!plugin){
        break;
      }
      if (MatchXMLName(plugin, "Plugin")) {
         
        std::string tmp;
        plugin.GetXML(tmp);
        std::cout<<tmp<<std::endl;         
 
        std::string plugin_name = (std::string)(plugin.Attribute("Name"));

        std::cout<<plugin_name<<std::endl;
            
        void *ptr = NULL;
        if (module->get_symbol(plugin_name.c_str(), ptr)) {
          id_list.push_back(ptr);
        }
        else 
          std::cout<<"There is no" << plugin_name <<"type plugin"<<std::endl;
           
      }
    }
  }

}

LoadableClass* ClassLoader::Instance(std::string& classId, Config* cfg){
  identifier_list_t::iterator it;
  void* ptr;

  for(it=id_list.begin(); it!=id_list.end(); ++it){
    ptr =(*it);
    for(loader_descriptor* desc = (loader_descriptor*)ptr; !((desc->name)==NULL); ++desc) {
      if(desc->name == classId){ 
        loader_descriptor &descriptor =*desc; 
        LoadableClass * res = NULL;
        
        res = (*descriptor.get_instance)(cfg);
        return res;
      }
    } 
  }
  return NULL;
}

} // namespace Arc

