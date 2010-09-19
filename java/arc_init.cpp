#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
#include <dlfcn.h>

#include <string>

#include <glibmm/module.h>
#include <glibmm/miscutils.h>
#include <arc/ArcLocation.h>
#include <arc/Utils.h>
#include <arc/Logger.h>


__attribute__((constructor)) void __arc_init(void) {

  Glib::Module* module = NULL;
  std::string modulepath;
  modulepath = std::string("libjarc.")+G_MODULE_SUFFIX;
#ifdef HAVE_GLIBMM_BIND_LOCAL
  module = new Glib::Module(modulepath,Glib::ModuleFlags(0));
#else
  module = new Glib::Module(modulepath);
#endif
  if(module != NULL) {
    if(*module) return;
    delete module;
  };
  Arc::Logger::getRootLogger().msg(Arc::WARNING,"Failed to export symbols of ARC java module");
}

