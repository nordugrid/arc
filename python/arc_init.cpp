#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <Python.h>

#include <cstdlib>
#include <dlfcn.h>

#include <string>

#include <glibmm/module.h>
#include <glibmm/miscutils.h>
#include <arc/ArcLocation.h>
#include <arc/Utils.h>
#include <arc/Logger.h>


static bool __arc_init_try_path(const std::string& modulepath) {
  Glib::Module *module = NULL;
#ifdef HAVE_GLIBMM_BIND_LOCAL
  module = new Glib::Module(modulepath,Glib::ModuleFlags(0));
#else
  module = new Glib::Module(modulepath);
#endif
  if(module == NULL) return false;
  if(*module) return true;
  delete module;
  return false;
}

__attribute__((constructor)) void __arc_init(void) {

/*
char* Py_GetPath ()
Return the default module search path; this is computed from the program
name (set by Py_SetProgramName() above) and some environment variables. The
returned string consists of a series of directory names separated by a
platform dependent delimiter character. The delimiter character is ":" on
Unix, ";" on DOS/Windows, and "\n" (the ASCII newline character) on
Macintosh. The returned string points into static storage; the caller should
not modify its value. The value is available to Python code as the list
sys.path, which may be modified to change the future search path for loaded
modules.
Note: it seems like Python is hiding site-packages part of path. Maybe it
is hardcoded inside Python somewhere. But at least part till site-packages
seems to be present.
*/
#if PY_MAJOR_VERSION >= 3
  std::wstring pythonwpath = Py_GetPath();
  std::string pythonpath(pythonwpath.begin(), pythonwpath.end());
#else
  std::string pythonpath = Py_GetPath();
#endif
  std::string::size_type start = 0;
  std::string::size_type end = pythonpath.find_first_of(";:\n");
  if(end == std::string::npos) end=pythonpath.length();
  for(;start<pythonpath.length();) {
    std::string path;
    std::string modulepath;
    path = pythonpath.substr(start,end-start);

    // Possible options
    //  PATH/_arc.so
    //  PATH/site-packages/_arc.so
    //  PATH/arc/_arc.so
    //  PATH/site-packages/arc/_arc.so
    modulepath = Glib::build_filename(path,std::string("_arc.")+G_MODULE_SUFFIX);
    if(__arc_init_try_path(modulepath)) return;
    modulepath = Glib::build_filename(Glib::build_filename(path,"site-packages"),std::string("_arc.")+G_MODULE_SUFFIX);
    if(__arc_init_try_path(modulepath)) return;
    modulepath = Glib::build_filename(Glib::build_filename(path,"arc"),std::string("_arc.")+G_MODULE_SUFFIX);
    if(__arc_init_try_path(modulepath)) return;
    modulepath = Glib::build_filename(Glib::build_filename(Glib::build_filename(path,"site-packages"),"arc"),std::string("_arc.")+G_MODULE_SUFFIX);
    if(__arc_init_try_path(modulepath)) return;

    start=end+1;
    end=pythonpath.find_first_of(";:\n",start);
    if(end == std::string::npos) end=pythonpath.length();
  };
}

