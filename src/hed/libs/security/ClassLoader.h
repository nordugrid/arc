#ifndef __ARC_CLASSLOADER_H__
#define __ARC_CLASSLOADER_H__

#include <map>
#include <string>

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/loader/Plugin.h>

/**Some implicit idea in the ClassLoader/ModuleManager stuff:
share_lib_name (e.g. mccsoap) should be global identical
plugin_name (e.g. __arc_attrfactory_modules__) should be global identical
desc->name (e.g. attr.factory) should also be global identical
*/
namespace Arc{

typedef Plugin LoadableClass;

// TODO: Unify  with Loader
class ClassLoader : public PluginsFactory {
  protected:
    ClassLoader(Config *cfg = NULL);
  protected:
    void load_all_instances(Config *cfg);    

  public:
    static ClassLoader* getClassLoader(Config* cfg = NULL); 

    LoadableClass *Instance(const std::string& classId, XMLNode* arg = NULL, const std::string& className = "");
    LoadableClass *Instance(XMLNode* arg = NULL, const std::string& className = "");

    ~ClassLoader();
  
  private:
    static Logger logger;
    static ClassLoader* _instance;
};

class ClassLoaderPluginArgument: public PluginArgument {
  private:
    XMLNode* xml_;
  public:
    ClassLoaderPluginArgument(XMLNode* xml):xml_(xml) { };
    virtual ~ClassLoaderPluginArgument(void) { };
    operator XMLNode* (void) { return xml_; };
};

} // namespace Arc

#endif /* __ARC_CLASSLOADER_H__ */
