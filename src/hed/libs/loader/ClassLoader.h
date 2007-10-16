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
    Arc::LoadableClass *(*get_instance)(void **argument);//,Arc::ChainContext* ctx);
} loader_descriptor;

typedef loader_descriptor loader_descriptors[];


namespace Arc{

class ClassLoader : public ModuleManager{
  public:
    ClassLoader(Config *cfg);
  protected:
    void load_all_instances(Config *cfg);    

  public:
    //virtual LoadableClass *Instance(const std::string& className);
    //LoadableClass *Instance(std::string& classId, Config* cfg);
 
    LoadableClass *Instance(std::string& classId, void** arg = NULL);

    //template <class LC>
    //void Instance(const std::string className, LC *&p, Config* cfg);

    ~ClassLoader();
  
  private:
    static Logger logger;
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
