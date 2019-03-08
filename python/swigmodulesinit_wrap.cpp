#ifndef SWIGEXPORT
#   if defined(__GNUC__) && defined(GCC_HASCLASSVISIBILITY)
#     define SWIGEXPORT __attribute__ ((visibility("default")))
#   else
#     define SWIGEXPORT
#   endif
#endif

#include <Python.h>

// Python 2vs3 differences
#if PY_MAJOR_VERSION >= 3
#define SWIG_init(NAME) PyInit__##NAME
#define PyMOD_RETURN(NAME) return NAME
#define PyMODVAL PyObject*
#else
#define SWIG_init(NAME) init_##NAME
#define PyMOD_RETURN(NAME) return
#define PyMODVAL void
#endif

PyMODINIT_FUNC SWIG_init(common)(void);
PyMODINIT_FUNC SWIG_init(loader)(void);
PyMODINIT_FUNC SWIG_init(message)(void);
PyMODINIT_FUNC SWIG_init(communication)(void);
PyMODINIT_FUNC SWIG_init(compute)(void);
PyMODINIT_FUNC SWIG_init(credential)(void);
PyMODINIT_FUNC SWIG_init(data)(void);
PyMODINIT_FUNC SWIG_init(delegation)(void);
PyMODINIT_FUNC SWIG_init(security)(void);

static PyMODVAL init_extension_module(PyObject* package, const char *modulename,
PyMODVAL (*initfunction)(void)) {
#if PY_MAJOR_VERSION >= 3
  PyObject *module = initfunction();
#else
  initfunction();
  PyObject *module = PyImport_AddModule((char *)modulename);
#endif
  if(!module) {
    fprintf(stderr, "Failed initialising Python module '%s', through Python C API\n", modulename);
    PyMOD_RETURN(NULL);
  }
  if(PyModule_AddObject(package, (char *)modulename, module)) {
    fprintf(stderr, "Failied adding Python module '%s' to package 'arc', through Python C API\n", modulename);
    PyMOD_RETURN(NULL);
  }

  PyObject *sys_modules = PyImport_GetModuleDict();
  if (!sys_modules) {
    fprintf(stderr, "Failed to locate sys.modules.\n");
    PyMOD_RETURN(NULL);
  }
  if (PyMapping_SetItemString(sys_modules, const_cast<char *>(modulename),
      module) == -1) {
    fprintf(stderr, "Failed to add %s to sys.modules.\n", modulename);
    PyMOD_RETURN(NULL);  
  }
  
  Py_INCREF(module);
  PyMOD_RETURN(module);
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

// We can probably change
//   extern "C" SWIGEXPORT to PyMODINIT_FUNC
// and thus remove SWIGEXPORT since it is no longer used and PyMODINIT_FUNC
// does most of what SWIGEXPORT does. One thing however would be missing:
//   __attribute__ ((visibility("default")))
// but that seems not to have any effect since -fvisibility*
// is not used during compilation.
//
extern "C"
SWIGEXPORT PyMODVAL SWIG_init(arc)(void) {
  // Initialise this module
#if PY_MAJOR_VERSION >= 3
  PyObject* module = PyModule_Create(&moduledef);
#else
  PyObject* module = Py_InitModule("_arc", NULL); // NULL only works for Python >= 2.3
#endif
  if(!module) {
   fprintf(stderr, "initialisation failed\n");
   PyMOD_RETURN(NULL);
  }
  
  // Initialise all the SWIG low level modules
  PyObject *package = PyImport_AddModule((char *)"arc"); // a means to get a handle to the package, not sure if this is a great idea but it works
  if(!package) {
   fprintf(stderr, "initialisation failed\n");
   PyMOD_RETURN(NULL);
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

  Py_INCREF(module);
  PyMOD_RETURN(module);
}
