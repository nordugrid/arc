#ifndef __ARC_MODULEMANAGER_H__
#define __ARC_MODULEMANAGER_H__

#include <string>
#include <map>
#include <glibmm/module.h>
#include "common/ArcConfig.h"

namespace Arc {

typedef std::map<std::string, Glib::Module *> plugin_cache_t;

class ModuleManager
{
    private:
        std::string plugin_dir;
        plugin_cache_t plugin_cache;
    public:
        ModuleManager(Arc::Config *cfg);
        ~ModuleManager();
        Glib::Module *load(const std::string& name);
};

}; // namespace Arc

#endif /* __ARC_MESSAGE_H__ */
