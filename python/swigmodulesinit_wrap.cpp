#ifndef SWIGEXPORT
# if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
#   if defined(STATIC_LINKED)
#     define SWIGEXPORT
#   else
#     define SWIGEXPORT __declspec(dllexport)
#   endif
# else
#   if defined(__GNUC__) && defined(GCC_HASCLASSVISIBILITY)
#     define SWIGEXPORT __attribute__ ((visibility("default")))
#   else
#     define SWIGEXPORT
#   endif
# endif
#endif

#include <Python.h>

extern "C" void init_common(void);
extern "C" void init_loader(void);
extern "C" void init_message(void);
extern "C" void init_communication(void);
extern "C" void init_compute(void);
extern "C" void init_credential(void);
extern "C" void init_data(void);
extern "C" void init_delegation(void);
extern "C" void init_security(void);

static void init_extension_module(PyObject* package, const char *modulename,
void (*initfunction)(void)) {
  PyObject *module = PyImport_AddModule((char *)modulename);
  if(!module) {
    fprintf(stderr, "initialisation in PyImport_AddModule failed for module %s\n", modulename);
    return;
  }
  if(PyModule_AddObject(package, (char *)modulename, module)) {
    fprintf(stderr, "initialisation in PyModule_AddObject failed for module %s\n", modulename);
    return;
  }
  Py_INCREF(module);
  initfunction();
}

extern "C"
SWIGEXPORT void init_arc(void) {
  // Initialise this module
  PyObject* module = Py_InitModule("_arc", NULL); // NULL only works for Python >= 2.3
  if(!module) {
   fprintf(stderr, "initialisation failed\n");
   return;
  }
  
  // Initialise all the SWIG low level modules
  PyObject *package = PyImport_AddModule((char *)"arc"); // a means to get a handle to the package, not sure if this is a great idea but it works
  if(!package) {
   fprintf(stderr, "initialisation failed\n");
   return;
  }
  
  init_extension_module(package, "_common", init_common);
  init_extension_module(package, "_loader", init_loader);
  init_extension_module(package, "_message", init_message);
  init_extension_module(package, "_communication", init_communication);
  init_extension_module(package, "_compute", init_compute);
  init_extension_module(package, "_credential", init_credential);
  init_extension_module(package, "_data", init_data);
  init_extension_module(package, "_delegation", init_delegation);
  init_extension_module(package, "_security", init_security);
}
