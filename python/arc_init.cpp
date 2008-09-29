#include <cstdlib>
#include <dlfcn.h>

#include <iostream>
#include <string>
#include <list>

#include <glibmm/module.h>
#include <glibmm/miscutils.h>
#include <arc/ArcLocation.h>
#include <arc/Utils.h>

__attribute__((constructor)) void __arc_init(void) {
  std::string pythonpath = Arc::GetEnv("PYTHONPATH");
  std::string::size_type start = 0;
  std::string::size_type end = pythonpath.find(';');
  if(end == std::string::npos) end=pythonpath.length();
  for(;start<pythonpath.length();) {
    std::string path = pythonpath.substr(start,end-start);
//    std::string modulepath = Glib::Module::build_path(path,"_arc");
    std::string modulepath = Glib::build_filename(path,"_arc.so");
#ifdef HAVE_GLIBMM_BIND_LOCAL
    Glib::Module *module = new Glib::Module(modulepath,Glib::MODULE_BIND_GLOBAL);
#else
    Glib::Module *module = new Glib::Module(modulepath);
#endif
    if(module != NULL) {
      if(*module) break;
      delete module;
    };
    std::cerr<<"dlopen failed: "<<dlerror()<<std::endl;
    start=end+1;
    end=pythonpath.find(';',start);
    if(end == std::string::npos) end=pythonpath.length();
  };
/*
  std::list<std::string> pluglist = Arc::ArcLocation::GetPlugins();
  for(std::list<std::string>::iterator path = pluglist.begin();path!=pluglist.end();++path) {
    std::string plugpath = Glib::Module::build_path(*path,"arcinit");
std::cerr<<"!!!!!!!!! __arc_init: loading: "<<plugpath<<std::endl;
#ifdef HAVE_GLIBMM_BIND_LOCAL
    Glib::Module *module = new Glib::Module(plugpath,Glib::MODULE_BIND_GLOBAL);
#else
    Glib::Module *module = new Glib::Module(plugpath);
#endif
    if(module == NULL) {
      std::cerr<<"dlopen failed: "<<dlerror()<<std::endl;
    };
  };
*/
}

