#include <cstdlib>
#include <dlfcn.h>

#include <iostream>
#include <string>
#include <list>

#include <glibmm/module.h>
#include <glibmm/miscutils.h>
#include <arc/ArcLocation.h>
#include <arc/Utils.h>
#include <arc/Logger.h>

#include <Python.h>


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
*/
  std::string pythonpath = Py_GetPath();
  std::string::size_type start = 0;
  std::string::size_type end = pythonpath.find_first_of(";:\n");
  if(end == std::string::npos) end=pythonpath.length();
  for(;start<pythonpath.length();) {
    std::string path = pythonpath.substr(start,end-start);
    std::string modulepath = Glib::build_filename(path,std::string("_arc.")+G_MODULE_SUFFIX);
#ifdef HAVE_GLIBMM_BIND_LOCAL
    Glib::Module *module = new Glib::Module(modulepath,Glib::MODULE_BIND_GLOBAL);
#else
    Glib::Module *module = new Glib::Module(modulepath);
#endif
    if(module != NULL) {
      if(*module) return;
      delete module;
    };
    start=end+1;
    end=pythonpath.find_first_of(";:\n",start);
    if(end == std::string::npos) end=pythonpath.length();
  };
  Arc::Logger::getRootLogger().msg(Arc::WARNING,"Failed to export symbols of ARC python module");
}

