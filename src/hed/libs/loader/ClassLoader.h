#ifndef __ARC_CLASSLOADER_H__
#define __ARC_CLASSLOADER_H__

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include "ModuleManager.h"
#include "LoadableClass.h"
#include <map>
#include <string>

typedef struct {
    const char* name;
    int version;
    Arc::LoadableClass *(*get_instance)(void *argument);
} loader_descriptor;

typedef loader_descriptor loader_descriptors[];

/**Some implicit idea in the ClassLoader/ModuleManager stuff:
share_lib_name (e.g. mccsoap) should be global identical
plugin_name (e.g. __arc_attrfactory_modules__) should be global identical
desc->name (e.g. attr.factory) should also be global identical
*/
typedef std::map<std::string,void*> identifier_map_t;

namespace Arc{

class ClassLoader : public ModuleManager{
  protected:
    ClassLoader(Config *cfg = NULL);
  protected:
    void load_all_instances(Config *cfg);    

  public:
    static ClassLoader* getClassLoader(Config* cfg = NULL); 

    //virtual LoadableClass *Instance(const std::string& className);
    //LoadableClass *Instance(std::string& classId, Config* cfg); 
    LoadableClass *Instance(const std::string& classId, void* arg = NULL);

    //template <class LC>
    //void Instance(const std::string className, LC *&p, Config* cfg);

    ~ClassLoader();
  
  private:
    static Logger logger;
    static ClassLoader* _instance;
    //static identifier_map_t id_map;
};

/*
template <class LC>
void ClassLoader::Instance(const std::string classId, LC *&p, Config* cfg){
  LoadableClass *object  = Instance(classId, cfg);
  //void *object = Instance(classId, cfg);
  p = dynamic_cast<LC *>(object);
  if(object && !p){
    delete object; 
    p = NULL;
  }
}
*/
} // namespace Arc

#endif /* __ARC_CLASSLOADER_H__ */
