#ifndef __ARC_CLASSLOADER_H__
#define __ARC_CLASSLOADER_H__

#include <arc/ArcConfig.h>
#include "ModuleManager.h"
#include "LoadableClass.h"
#include <map>
#include <string>

typedef struct {
    const char* name;
    int version;
    Arc::LoadableClass *(*get_instance)(Arc::Config *cfg);//,Arc::ChainContext* ctx);
} loader_descriptor;

typedef loader_descriptor loader_descriptors[];

typedef std::list<void*> identifier_list_t;
identifier_list_t id_list;

namespace Arc{

class ClassLoader : public ModuleManager{
  public:
    ClassLoader(Config *cfg);
  protected:
    void load_all_instances(Config *cfg);    

  public:
    //virtual LoadableClass *Instance(const std::string& className);
    LoadableClass *Instance(std::string& classId, Config* cfg);

    template <class LC>
    void Instance(const std::string className, LC *&p, Config* cfg);

    ~ClassLoader();

};

template <class LC>
void ClassLoader::Instance(const std::string classId, LC *&p, Config* cfg){
  LoadableClass *object  = Instance(classId);
  //void *object = Instance(classId, cfg);
  p = dynamic_cast<LC *>(object);
  if(object && !p){
    delete object; 
    p = NULL;
  }
}

} // namespace Arc

#endif /* __ARC_CLASSLOADER_H__ */
