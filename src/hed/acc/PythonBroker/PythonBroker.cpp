// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "PythonBroker.h"

extern "C" {
typedef struct {
  PyObject_HEAD
  void *ptr;
  // There are more members in this stuct, but they are not needed here...
} PySwigObject;
}

namespace Arc {

  Logger PythonBroker::logger(Broker::logger, "PythonBroker");
  PyThreadState*PythonBroker::tstate = NULL;
  int PythonBroker::refcount = 0;
  Glib::Mutex PythonBroker::lock;

  class PythonLock {
  public:
    PythonLock() {
      gstate = PyGILState_Ensure();
    }
    ~PythonLock() {
      PyGILState_Release(gstate);
    }
  private:
    PyGILState_STATE gstate;
  };

  class PyObjectP {
  public:
    PyObjectP(PyObject *obj)
      : obj(obj) {}
    ~PyObjectP() {
      if (obj) {
        Py_DECREF(obj);
      }
    }
    operator bool() {
      return obj;
    }
    bool operator!() {
      return !obj;
    }
    operator PyObject*() {
      return obj;
    }
  private:
    PyObject *obj;
  };

  Plugin* PythonBroker::Instance(PluginArgument *arg) {

    BrokerPluginArgument *brokerarg = dynamic_cast<BrokerPluginArgument*>(arg);
    if (!brokerarg)
      return NULL;

    lock.lock();

    // Initialize the Python Interpreter
    if (!Py_IsInitialized()) {
#ifdef HAVE_PYTHON_INITIALIZE_EX
      Py_InitializeEx(0);            // Python does not handle signals
#endif
      PyEval_InitThreads();          // Main thread created and lock acquired
      tstate = PyThreadState_Get();  // Get current thread
      if (!tstate) {
        logger.msg(ERROR, "Failed to initialize main Python thread");
        return NULL;
      }
    }
    else {
      if (!tstate) {
        logger.msg(ERROR, "Main Python thread was not initialized");
        return NULL;
      }
      PyEval_AcquireThread(tstate);
    }

    refcount++;

    lock.unlock();

    logger.msg(DEBUG, "Loading Python broker (%i)", refcount);

    Broker *broker = new PythonBroker(*brokerarg);

    PyEval_ReleaseThread(tstate); // Release current thread

    return broker;
  }

  PythonBroker::PythonBroker(const UserConfig& usercfg)
    : Broker(usercfg),
      arc_module(NULL),
      arc_userconfig_klass(NULL),
      arc_jobrepr_klass(NULL),
      arc_xtarget_klass(NULL),
      module(NULL),
      klass(NULL),
      object(NULL) {

    if (!tstate) {
      logger.msg(ERROR, "Main Python thread is not initialized");
      return;
    }

    logger.msg(VERBOSE, "PythonBroker init");

    std::string args = usercfg.Broker().second;
    std::string::size_type pos = args.find(':');
    if (pos != std::string::npos)
      args.resize(pos);
    pos = args.rfind('.');
    if (pos == std::string::npos) {
      logger.msg(ERROR, "Invalid class name");
      return;
    }
    std::string module_name = args.substr(0, pos);
    std::string class_name = args.substr(pos + 1);
    logger.msg(VERBOSE, "Class name: %s", class_name);
    logger.msg(VERBOSE, "Module name: %s", module_name);

    // Import arc python module
#if PY_MAJOR_VERSION >= 3
    PyObjectP py_arc_module_name = PyUnicode_FromString("arc");
#else
    PyObjectP py_arc_module_name = PyString_FromString("arc");
#endif
    if (!py_arc_module_name) {
      logger.msg(ERROR, "Cannot convert ARC module name to Python string");
      if (PyErr_Occurred())
        PyErr_Print();
      return;
    }

    arc_module = PyImport_Import(py_arc_module_name);
    if (!arc_module) {
      logger.msg(ERROR, "Cannot import ARC module");
      if (PyErr_Occurred())
        PyErr_Print();
      return;
    }

    // Get dictionary of arc module content (borrowed reference)
    PyObject *arc_dict = PyModule_GetDict(arc_module);
    if (!arc_dict) {
      logger.msg(ERROR, "Cannot get dictionary of ARC module");
      if (PyErr_Occurred())
        PyErr_Print();
      return;
    }

    // Get the Config class (borrowed reference)
    arc_userconfig_klass = PyDict_GetItemString(arc_dict, "UserConfig");
    if (!arc_userconfig_klass) {
      logger.msg(ERROR, "Cannot find ARC UserConfig class");
      if (PyErr_Occurred())
        PyErr_Print();
      return;
    }

    // check is it really a class
    if (!PyCallable_Check(arc_userconfig_klass)) {
      logger.msg(ERROR, "UserConfig class is not an object");
      return;
    }

    // Get the JobDescription class (borrowed reference)
    arc_jobrepr_klass = PyDict_GetItemString(arc_dict, "JobDescription");
    if (!arc_jobrepr_klass) {
      logger.msg(ERROR, "Cannot find ARC JobDescription class");
      if (PyErr_Occurred())
        PyErr_Print();
      return;
    }

    // check is it really a class
    if (!PyCallable_Check(arc_jobrepr_klass)) {
      logger.msg(ERROR, "JobDescription class is not an object");
      return;
    }

    // Get the ExecutionTarget class (borrowed reference)
    arc_xtarget_klass = PyDict_GetItemString(arc_dict, "ExecutionTarget");
    if (!arc_xtarget_klass) {
      logger.msg(ERROR, "Cannot find ARC ExecutionTarget class");
      if (PyErr_Occurred())
        PyErr_Print();
      return;
    }

    // check is it really a class
    if (!PyCallable_Check(arc_xtarget_klass)) {
      logger.msg(ERROR, "ExecutionTarget class is not an object");
      return;
    }

    // Import custom broker module
#if PY_MAJOR_VERSION >= 3
    PyObjectP py_module_name = PyUnicode_FromString(module_name.c_str());
#else
    PyObjectP py_module_name = PyString_FromString(module_name.c_str());
#endif
    if (!py_module_name) {
      logger.msg(ERROR, "Cannot convert module name to Python string");
      if (PyErr_Occurred())
        PyErr_Print();
      return;
    }

    module = PyImport_Import(py_module_name);
    if (!module) {
      logger.msg(ERROR, "Cannot import module");
      if (PyErr_Occurred())
        PyErr_Print();
      return;
    }

    // Get dictionary of module content (borrowed reference)
    PyObject *dict = PyModule_GetDict(module);
    if (!dict) {
      logger.msg(ERROR, "Cannot get dictionary of custom broker module");
      if (PyErr_Occurred())
        PyErr_Print();
      return;
    }

    // Get the class (borrowed reference)
    klass = PyDict_GetItemString(dict, (char*)class_name.c_str());
    if (!klass) {
      logger.msg(ERROR, "Cannot find custom broker class");
      if (PyErr_Occurred())
        PyErr_Print();
      return;
    }

    // check is it really a class
    if (!PyCallable_Check(klass)) {
      logger.msg(ERROR, "%s class is not an object", class_name);
      return;
    }

    PyObjectP usercfgarg = Py_BuildValue("(l)", (long int)usercfg);
    if (!usercfgarg) {
      logger.msg(ERROR, "Cannot create UserConfig argument");
      if (PyErr_Occurred())
        PyErr_Print();
      return;
    }

    PyObject *py_usercfg = PyObject_CallObject(arc_userconfig_klass, usercfgarg);
    if (!py_usercfg) {
      logger.msg(ERROR, "Cannot convert UserConfig to Python object");
      if (PyErr_Occurred())
        PyErr_Print();
      return;
    }

    PyObjectP arg = Py_BuildValue("(O)", py_usercfg);
    if (!arg) {
      logger.msg(ERROR, "Cannot create argument of the constructor");
      if (PyErr_Occurred())
        PyErr_Print();
      return;
    }

    // create instance of class
    object = PyObject_CallObject(klass, arg);
    if (!object) {
      logger.msg(ERROR, "Cannot create instance of Python class");
      if (PyErr_Occurred())
        PyErr_Print();
      return;
    }

    logger.msg(VERBOSE, "Python broker constructor called (%d)", refcount);
  }

  PythonBroker::~PythonBroker() {

    if (module) {
      Py_DECREF(module);
    }
    if (arc_module) {
      Py_DECREF(arc_module);
    }

    lock.lock();

    refcount--;

    // Finish the Python Interpreter
    if (refcount == 0) {
      PyEval_AcquireThread(tstate);
      Py_Finalize();
    }

    lock.unlock();

    logger.msg(VERBOSE, "Python broker destructor called (%d)", refcount);
  }

  void PythonBroker::SortTargets() {

    PythonLock pylock;

    // Convert JobDescription to python object
    PyObjectP arg = Py_BuildValue("(l)", job);
    if (!arg) {
      logger.msg(ERROR, "Cannot create JobDescription argument");
      if (PyErr_Occurred())
        PyErr_Print();
      return;
    }

    PyObjectP py_job = PyObject_CallObject(arc_jobrepr_klass, arg);
    if (!py_job) {
      logger.msg(ERROR,
                 "Cannot convert JobDescription to python object");
      if (PyErr_Occurred())
        PyErr_Print();
      return;
    }

    // Convert incoming PossibleTargets to python list
    PyObjectP py_list = PyList_New(0);
    if (!py_list) {
      logger.msg(ERROR, "Cannot create Python list");
      if (PyErr_Occurred())
        PyErr_Print();
      return;
    }

    for (std::list<ExecutionTarget*>::iterator it = PossibleTargets.begin();
         it != PossibleTargets.end(); it++) {

      PyObjectP arg = Py_BuildValue("(l)", (long int)&**it);
      if (!arg) {
        logger.msg(ERROR, "Cannot create ExecutionTarget argument");
        if (PyErr_Occurred())
          PyErr_Print();
        return;
      }

      PyObject *py_xtarget = PyObject_CallObject(arc_xtarget_klass, arg);
      if (!py_xtarget) {
        logger.msg(ERROR, "Cannot convert ExecutionTarget to python object");
        if (PyErr_Occurred())
          PyErr_Print();
        return;
      }

      PyList_Append(py_list, py_xtarget);
    }

    PyObjectP py_status = PyObject_CallMethod(object, (char*)"SortTargets",
                                              (char*)"(OO)",
                                              (PyObject*)py_list,
                                              (PyObject*)py_job);
    if (!py_status) {
      if (PyErr_Occurred())
        PyErr_Print();
      return;
    }

    PossibleTargets.clear();

    for (int i = 0; i < PyList_Size(py_list); i++) {
      PyObject *obj = PyList_GetItem(py_list, i);
      char this_str[] = "this";
      if (!PyObject_HasAttrString(obj, this_str))
        return;
      PyObject *thisattr = PyObject_GetAttrString(obj, this_str);
      if (!thisattr)
        return;
      void *ptr = ((PySwigObject*)thisattr)->ptr;
      PossibleTargets.push_back(((ExecutionTarget*)ptr));
      Py_DECREF(thisattr);
    }

    TargetSortingDone = true;

    return;
  }

} // namespace Arc

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "PythonBroker", "HED:Broker", 0, &Arc::PythonBroker::Instance },
  { NULL, NULL, 0, NULL }
};
