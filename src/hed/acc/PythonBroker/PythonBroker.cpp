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
  PyThreadState *PythonBroker::tstate = NULL;
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
      if(obj)
	Py_DECREF(obj);
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

    ACCPluginArgument *accarg = dynamic_cast<ACCPluginArgument*>(arg);
    if (!accarg)
      return NULL;

    lock.lock();

    // Initialize the Python Interpreter
    if (!Py_IsInitialized()) {
      Py_InitializeEx(0);            // Python does not handle signals
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

    logger.msg(VERBOSE, "Loading python broker (%i)", refcount);

    ACC *broker = new PythonBroker((Config*)(*accarg));

    PyEval_ReleaseThread(tstate); // Release current thread

    return broker;
  }

  PythonBroker::PythonBroker(Config *cfg)
    : Broker(cfg),
      arc_module(NULL),
      arc_config_klass(NULL),
      arc_jobrepr_klass(NULL),
      arc_xtarget_klass(NULL),
      module(NULL),
      klass(NULL),
      object(NULL) {

    if (!tstate) {
      logger.msg(ERROR, "Main python thread is not initialized");
      return;
    }

    logger.msg(DEBUG, "PyhtonBroker init");

    std::string args = (std::string)(*cfg)["Arguments"];
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
    logger.msg(DEBUG, "class name: %s", class_name);
    logger.msg(DEBUG, "module name: %s", module_name);

    // Import arc python module
    PyObjectP py_arc_module_name = PyString_FromString("arc");
    if (!py_arc_module_name) {
      logger.msg(ERROR, "Cannot convert arc module name to Python string");
      if (PyErr_Occurred())
	PyErr_Print();
      return;
    }

    arc_module = PyImport_Import(py_arc_module_name);
    if (!arc_module) {
      logger.msg(ERROR, "Cannot import arc module");
      if (PyErr_Occurred())
	PyErr_Print();
      return;
    }

    // Get dictionary of arc module content (borrowed reference)
    PyObject *arc_dict = PyModule_GetDict(arc_module);
    if (!arc_dict) {
      logger.msg(ERROR, "Cannot get dictionary of arc module");
      if (PyErr_Occurred())
	PyErr_Print();
      return;
    }

    // Get the Config class (borrowed reference)
    arc_config_klass = PyDict_GetItemString(arc_dict, "Config");
    if (!arc_config_klass) {
      logger.msg(ERROR, "Cannot find arc Config class");
      if (PyErr_Occurred())
	PyErr_Print();
      return;
    }

    // check is it really a class
    if (!PyCallable_Check(arc_config_klass)) {
      logger.msg(ERROR, "Config class is not an object");
      return;
    }

    // Get the JobInnerRepresentation class (borrowed reference)
    arc_jobrepr_klass = PyDict_GetItemString(arc_dict,
					     "JobInnerRepresentation");
    if (!arc_jobrepr_klass) {
      logger.msg(ERROR, "Cannot find arc JobInnerRepresentation class");
      if (PyErr_Occurred())
	PyErr_Print();
      return;
    }

    // check is it really a class
    if (!PyCallable_Check(arc_jobrepr_klass)) {
      logger.msg(ERROR, "JobInnerRepresentation class is not an object");
      return;
    }

    // Get the ExecutionTarget class (borrowed reference)
    arc_xtarget_klass = PyDict_GetItemString(arc_dict, "ExecutionTarget");
    if (!arc_xtarget_klass) {
      logger.msg(ERROR, "Cannot find arc ExecutionTarget class");
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
    PyObjectP py_module_name = PyString_FromString(module_name.c_str());
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

    PyObjectP carg = Py_BuildValue("(l)", (long int)cfg);
    if (!carg) {
      logger.msg(ERROR, "Cannot create config argument");
      if (PyErr_Occurred())
	PyErr_Print();
      return;
    }

    PyObject* py_cfg = PyObject_CallObject(arc_config_klass, carg);
    if (!py_cfg) {
      logger.msg(ERROR, "Cannot convert config to python object");
      if (PyErr_Occurred())
	PyErr_Print();
      return;
    }

    PyObjectP arg = Py_BuildValue("(O)", py_cfg);
    if (!arg) {
      logger.msg(ERROR, "Cannot create argument of the constructor");
      if (PyErr_Occurred())
	PyErr_Print();
      return;
    }

    // create instance of class
    object = PyObject_CallObject(klass, arg);
    if (!object) {
      logger.msg(ERROR, "Cannot create instance of python class");
      if (PyErr_Occurred())
	PyErr_Print();
      return;
    }

    logger.msg(DEBUG, "Python broker constructor called (%d)", refcount);
  }

  PythonBroker::~PythonBroker() {

    if (module)
      Py_DECREF(module);
    if (arc_module)
      Py_DECREF(arc_module);

    lock.lock();

    refcount--;

    // Finish the Python Interpreter
    if (refcount == 0) {
      PyEval_AcquireThread(tstate);
      Py_Finalize();
    }

    lock.unlock();

    logger.msg(DEBUG, "Python broker destructor called (%d)", refcount);
  }

  void PythonBroker::SortTargets() {

    PythonLock pylock;

    // Convert JobInnerRepresentation to python object
    PyObjectP arg = Py_BuildValue("(l)", &jir);
    if (!arg) {
      logger.msg(ERROR, "Cannot create JobInnerRepresentation argument");
      if (PyErr_Occurred())
	PyErr_Print();
      return;
    }

    PyObjectP py_jir = PyObject_CallObject(arc_jobrepr_klass, arg);
    if (!py_jir) {
      logger.msg(ERROR,
		 "Cannot convert JobInnerRepresentation to python object");
      if (PyErr_Occurred())
	PyErr_Print();
      return;
    }

    // Convert incoming PossibleTargets to python list
    PyObjectP py_list = PyList_New(0);
    if (!py_list) {
      logger.msg(ERROR, "Cannot create python list");
      if (PyErr_Occurred())
	PyErr_Print();
      return;
    }

    for (std::vector<ExecutionTarget>::iterator it = PossibleTargets.begin();
	 it != PossibleTargets.end(); it++) {

      PyObjectP arg = Py_BuildValue("(l)", (long int)&*it);
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
					      (PyObject*)py_jir);
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
      PossibleTargets.push_back(*((ExecutionTarget*)ptr));
      Py_DECREF(thisattr);
    }

    TargetSortingDone = true;

    return;
  }

} // namespace Arc

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "PythonBroker", "HED:ACC", 0, &Arc::PythonBroker::Instance },
  { NULL, NULL, 0, NULL }
};
