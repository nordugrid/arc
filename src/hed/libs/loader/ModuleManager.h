#ifndef __ARC_MODULEMANAGER_H__
#define

#include <glibmm/gmodule.h>

namespace Loader {
class ModuleManager
{
    public:
        ModuleManager();
        ~ModuleManager();
        bool load(const std::string& path);
    private:
       std::map<name, Glib::Module *> _plugin;
       std::list<Glib::ustring> _plugin_dirs;
}

}; // namespace Loader

#endif /* __ARC_MESSAGE_H__ */
