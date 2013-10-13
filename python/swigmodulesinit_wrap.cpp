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

#if PY_MAJOR_VERSION >= 3
#define SWIG_init(NAME) PyInit__##NAME
#else
#define SWIG_init(NAME) init_##NAME
#endif

extern "C" void SWIG_init(common)(void);
extern "C" void SWIG_init(loader)(void);
extern "C" void SWIG_init(message)(void);
extern "C" void SWIG_init(communication)(void);
extern "C" void SWIG_init(compute)(void);
extern "C" void SWIG_init(credential)(void);
extern "C" void SWIG_init(data)(void);
extern "C" void SWIG_init(delegation)(void);
extern "C" void SWIG_init(security)(void);

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


#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef moduledef = {
  PyModuleDef_HEAD_INIT,
  "_arc",              /* m_name */
  NULL,                /* m_doc */
  -1,                  /* m_size */
  NULL,                /* m_methods */
  NULL,                /* m_reload */
  NULL,                /* m_traverse */
  NULL,                /* m_clear */
  NULL,                /* m_free */
};
#endif

extern "C"
SWIGEXPORT void SWIG_init(arc)(void) {
  // Initialise this module
#if PY_MAJOR_VERSION >= 3
  PyObject* module = PyModule_Create(&moduledef);
#else
  PyObject* module = Py_InitModule("_arc", NULL); // NULL only works for Python >= 2.3
#endif
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
  
  init_extension_module(package, "_common", SWIG_init(common));
  init_extension_module(package, "_loader", SWIG_init(loader));
  init_extension_module(package, "_message", SWIG_init(message));
  init_extension_module(package, "_communication", SWIG_init(communication));
  init_extension_module(package, "_compute", SWIG_init(compute));
  init_extension_module(package, "_credential", SWIG_init(credential));
  init_extension_module(package, "_data", SWIG_init(data));
  init_extension_module(package, "_delegation", SWIG_init(delegation));
  init_extension_module(package, "_security", SWIG_init(security));
}
