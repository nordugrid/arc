#ifndef SWIGEXPORT
#   if defined(__GNUC__) && defined(GCC_HASCLASSVISIBILITY)
#     define SWIGEXPORT __attribute__ ((visibility("default")))
#   else
#     define SWIGEXPORT
#   endif
#endif

#include <Python.h>

#define SWIG_init(NAME) PyInit__##NAME
#define PyMOD_RETURN(NAME) return NAME
#define PyMODVAL PyObject*

PyMODINIT_FUNC SWIG_init(common)(void);
PyMODINIT_FUNC SWIG_init(loader)(void);
PyMODINIT_FUNC SWIG_init(message)(void);
PyMODINIT_FUNC SWIG_init(communication)(void);
PyMODINIT_FUNC SWIG_init(compute)(void);
PyMODINIT_FUNC SWIG_init(credential)(void);
PyMODINIT_FUNC SWIG_init(data)(void);
PyMODINIT_FUNC SWIG_init(delegation)(void);
PyMODINIT_FUNC SWIG_init(security)(void);

/* Legacy init based on https://peps.python.org/pep-0489/ */
static PyObject *module_legacy_init(PyModuleDef *def) {
  PyModuleDef_Slot *slots = def->m_slots;
  def->m_slots = NULL;
  PyObject *mod = PyModule_Create(def);
  while (mod && slots->slot) {
    if (slots->slot == Py_mod_exec) {
      int (*mod_exec)(PyObject *) = (int (*)(PyObject *))slots->value;
      if (mod_exec(mod) != 0) {
        Py_DECREF(mod);
        mod = NULL;
      }
    }
    ++slots;
  }
  return mod;
}

static PyMODVAL init_extension_module(PyObject* package, const char *modulename,
PyMODVAL (*initfunction)(void)) {
  // swig-4.4.0 implements PEP-489 multi-phase initialization.
  // Handle both old single-phase and new multi-phase initialization.
  // Modules that use multi-phase initialization will return a PyModuleDef, so then we force a legacy single-phase initialization.
  PyObject *module_or_module_def = initfunction();
  if (!module_or_module_def) {
    fprintf(stderr, "Failed first phase initializing Python module '%s', through Python C API\n", modulename);
    PyMOD_RETURN(NULL);
  }
  PyObject *module = NULL;
  if (PyObject_TypeCheck(module_or_module_def, &PyModuleDef_Type)) {
    module = module_legacy_init((PyModuleDef *)module_or_module_def);
    if (!module) {
      fprintf(stderr, "Failed second phase initializing Python module '%s', through Python C API\n", modulename);
      PyMOD_RETURN(NULL);
    }
  } else {
    module = module_or_module_def;
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
  PyObject* module = PyModule_Create(&moduledef);
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
