#ifndef __ARC_MODULEMANAGER_H__
#define __ARC_MODULEMANAGER_H__

#include <string>
#include <map>
#include <glibmm/module.h>

namespace Loader {

typedef std::map<std::string, Glib::Module *> plugin_cache_t;

class ModuleManager
{
    private:
        std::string plugin_dir;
        plugin_cache_t plugin_cache;
        Glib::Module *load(const std::string& name);
    public:
        ModuleManager();
        ~ModuleManager();
        void *load_mcc(const std::string& name);
};

}; // namespace Loader

#endif /* __ARC_MESSAGE_H__ */
