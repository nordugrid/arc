// based on:
// http://www.codeproject.com/cpp/embedpython_1.asp
// http://coding.derkeiler.com/Archive/Python/comp.lang.python/2006-11/msg01211.html
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "pythonwrapper.h"

#include <iostream>
#include <arc/message/SOAPMessage.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/MCCLoader.h>
#include <arc/Thread.h>

#ifndef WIN32
#include <dlfcn.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* SWIG Specific object SHOULD BE SYNC WITH generated SWIG CODE */
typedef void *(*swig_converter_func)(void *);
typedef struct swig_type_info *(*swig_dycast_func)(void **);

typedef struct swig_type_info {
  const char             *name;         /* mangled name of this type */
  const char             *str;          /* human readable name of this type */
  swig_dycast_func        dcast;        /* dynamic cast function down a hierarchy */
  struct swig_cast_info  *cast;         /* linked list of types that can cast into this type */
  void                   *clientdata;   /* language specific type data */
  int                    owndata;       /* flag if the structure owns the clientdata */
} swig_type_info;

/* Structure to store a type and conversion function used for casting */
typedef struct swig_cast_info {
  swig_type_info         *type;         /* pointer to type that is equivalent to this type */
  swig_converter_func     converter;    /* function to cast the void pointers */
  struct swig_cast_info  *next;         /* pointer to next cast in linked list */
  struct swig_cast_info  *prev;         /* pointer to the previous cast */
} swig_cast_info;

typedef struct {
  PyObject_HEAD
  void *ptr;
  swig_type_info *ty;
  int own;
  PyObject *next;
} PySwigObject;

#ifdef __cplusplus
}
#endif

void *extract_swig_wrappered_pointer(PyObject *obj)
{
    char this_str[] = "this";
    if (!PyObject_HasAttrString(obj, this_str)) {
        return NULL;
    }
    PyObject *thisAttr = PyObject_GetAttrString(obj, this_str);
    if (thisAttr == NULL) {
        return NULL;
    }
    void* ptr = ((PySwigObject *)thisAttr)->ptr;
    Py_DECREF(thisAttr);
    return ptr;
}

// Thread state of main python interpreter thread
static PyThreadState *tstate = NULL;
static int python_service_counter = 0;
static Glib::Mutex service_lock;
Arc::Logger Arc::Service_PythonWrapper::logger(Service::logger, "PythonWrapper");

static Arc::Plugin* get_service(Arc::PluginArgument* arg) {
    Arc::ServicePluginArgument* srvarg =
            arg?dynamic_cast<Arc::ServicePluginArgument*>(arg):NULL;
    if(!srvarg) return NULL;
    Arc::ChainContext* ctx = (Arc::ChainContext*)(*srvarg);

#ifndef WIN32
    // ((Arc::PluginsFactory*)(*ctx))->load("pythonservice",false,true); // doesn't work, why?
    ::dlopen(((Arc::PluginsFactory*)(*ctx))->findLocation("pythonservice").c_str(),RTLD_NOW | RTLD_GLOBAL);
#endif

    service_lock.lock();
    // Initialize the Python Interpreter
    if (!Py_IsInitialized()) {
        Py_InitializeEx(0); // python does not handle signals
        PyEval_InitThreads(); // Main thread created and lock acquired
        tstate = PyThreadState_Get(); // Get current thread
        if(tstate == NULL) {
            Arc::Logger::getRootLogger().msg(Arc::ERROR, "Failed to initialize main Python thread");
            return NULL;
        }
    } else {
        if(tstate == NULL) {
            Arc::Logger::getRootLogger().msg(Arc::ERROR, "Main Python thread was not initialized");
            return NULL;
        }
        PyEval_AcquireThread(tstate);
    }
    python_service_counter++;
    Arc::Logger::getRootLogger().msg(Arc::DEBUG, "Loading %u-th Python service", python_service_counter);
    service_lock.unlock();
    Arc::Service* service = new Arc::Service_PythonWrapper((Arc::Config*)(*srvarg),arg);
    PyEval_ReleaseThread(tstate); // Release current thread
    Arc::Logger::getRootLogger().msg(Arc::DEBUG, "Initialized %u-th Python service", python_service_counter);
    return service;
}

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
    { "pythonservice", "HED:SERVICE", NULL, 0, &get_service },
    { NULL, NULL, NULL, 0, NULL }
};

namespace Arc {

Service_PythonWrapper::Service_PythonWrapper(Arc::Config *cfg,Arc::PluginArgument* parg):Service(cfg,parg), initialized(false)
{
    PyObject *py_module_name = NULL;
    PyObject *py_arc_module_name = NULL;
    PyObject *dict = NULL;
    PyObject *arc_dict = NULL;
    PyObject *arc_cfg_klass = NULL;
    PyObject *arg = NULL;
    PyObject *py_cfg = NULL;
    PyObject *klass = NULL;

    arc_module = NULL;
    module = NULL;
    object = NULL;

    if (tstate == NULL) {
        logger.msg(Arc::ERROR, "Main Python thread is not initialized");
        return;
    }
    //PyEval_AcquireThread(tstate);

    std::string path = (std::string)(*cfg)["ClassName"];
    std::size_t p = path.rfind(".");
    if (p == std::string::npos) {
        logger.msg(Arc::ERROR, "Invalid class name");
        return;
    }
    std::string module_name = path.substr(0, p);
    std::string class_name = path.substr(p+1, path.length());
    logger.msg(Arc::VERBOSE, "class name: %s", class_name);
    logger.msg(Arc::VERBOSE, "module name: %s", module_name);

    // Convert module name to Python string
#if PY_MAJOR_VERSION >= 3
    py_module_name = PyUnicode_FromString(module_name.c_str());
#else
    py_module_name = PyString_FromString(module_name.c_str());
#endif
    if (py_module_name == NULL) {
        logger.msg(Arc::ERROR, "Cannot convert module name to Python string");
        if (PyErr_Occurred()) PyErr_Print();
        return;
    }
    // Load module
    module = PyImport_Import(py_module_name);
    if (module == NULL) {
        logger.msg(Arc::ERROR, "Cannot import module");
        if (PyErr_Occurred()) PyErr_Print();
        Py_DECREF(py_module_name);
        return;
    }
    Py_DECREF(py_module_name);

    // Import ARC python wrapper
#if PY_MAJOR_VERSION >= 3
    py_arc_module_name = PyUnicode_FromString("arc");
#else
    py_arc_module_name = PyString_FromString("arc");
#endif
    if (py_arc_module_name == NULL) {
        logger.msg(Arc::ERROR, "Cannot convert ARC module name to Python string");
        if (PyErr_Occurred()) PyErr_Print();
        return;
    }

    // Load arc module
    arc_module = PyImport_Import(py_arc_module_name);
    if (arc_module == NULL) {
        logger.msg(Arc::ERROR, "Cannot import ARC module");
        if (PyErr_Occurred()) PyErr_Print();
        Py_DECREF(py_arc_module_name);
        return;
    }
    Py_DECREF(py_arc_module_name);

    // arc_dict is a borrowed reference
    arc_dict = PyModule_GetDict(arc_module);
    if (arc_dict == NULL) {
        logger.msg(Arc::ERROR, "Cannot get dictionary of ARC module");
        if (PyErr_Occurred()) PyErr_Print();
        return;
    }

    // Get the arc config class
    // arc_cfg_klass is a borrowed reference
    arc_cfg_klass = PyDict_GetItemString(arc_dict, "Config");
    if (arc_cfg_klass == NULL) {
        logger.msg(Arc::ERROR, "Cannot find ARC Config class");
        if (PyErr_Occurred()) PyErr_Print();
        return;
    }

    // check is it really a class
    if (!PyCallable_Check(arc_cfg_klass)) {
        logger.msg(Arc::ERROR, "Config class is not an object");
        return;
    }

    // Get dictionary of module content
    // dict is a borrowed reference
    dict = PyModule_GetDict(module);
    if (dict == NULL) {
        logger.msg(Arc::ERROR, "Cannot get dictionary of module");
        if (PyErr_Occurred()) PyErr_Print();
        return;
    }

    // Get the class
    // klass is a borrowed reference
    klass = PyDict_GetItemString(dict, (char*)class_name.c_str());
    if (klass == NULL) {
        logger.msg(Arc::ERROR, "Cannot find service class");
        if (PyErr_Occurred()) PyErr_Print();
        return;
    }

    // check is it really a class
    if (PyCallable_Check(klass)) {
        arg = Py_BuildValue("(l)", (long int)cfg);
        if (arg == NULL) {
            logger.msg(Arc::ERROR, "Cannot create config argument");
            if (PyErr_Occurred()) PyErr_Print();
            return;
        }

        py_cfg = PyObject_CallObject(arc_cfg_klass, arg);
        if (py_cfg == NULL) {
            logger.msg(Arc::ERROR, "Cannot convert config to Python object");
            if (PyErr_Occurred()) PyErr_Print();
            Py_DECREF(arg);
            return;
        }
        Py_DECREF(arg);
        arg = Py_BuildValue("(O)", py_cfg);
        if (arg == NULL) {
            logger.msg(Arc::ERROR, "Cannot create argument of the constructor");
            if (PyErr_Occurred()) PyErr_Print();
            return;
        }

        // create instance of class
        object = PyObject_CallObject(klass, arg);
        if (object == NULL) {
            logger.msg(Arc::ERROR, "Cannot create instance of Python class");
            if (PyErr_Occurred()) PyErr_Print();
            return;
        }
        Py_DECREF(arg);

    } else {
        logger.msg(Arc::ERROR, "%s is not an object", class_name);
        return;
    }

    // check is it really a class
    if (!PyCallable_Check(klass)) {
        logger.msg(Arc::ERROR, "Message class is not an object");
        return;
    }
    //tstate = PyGILState_GetThisThreadState();
    //PyEval_ReleaseThread(tstate);

    logger.msg(Arc::VERBOSE, "Python Wrapper constructor succeeded");
    initialized = true;
}

Service_PythonWrapper::~Service_PythonWrapper(void)
{
    service_lock.lock();
    PyEval_AcquireThread(tstate);
    // Release python objects - it is needed for Python
    // destructors to be called
    if(arc_module) { Py_DECREF(arc_module); }
    if(module) { Py_DECREF(module); }
    if(object) { Py_DECREF(object); }
    // Finish the Python Interpreter
    python_service_counter--;
    logger.msg(Arc::VERBOSE, "Python Wrapper destructor (%d)", python_service_counter);
    if (python_service_counter == 0) {
        Py_Finalize();
    } else {
        PyEval_ReleaseThread(tstate);
    }
    service_lock.unlock();
}

Arc::MCC_Status Service_PythonWrapper::make_fault(Arc::Message& outmsg)
{
    Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(Arc::NS(),true);
    Arc::SOAPFault* fault = outpayload->Fault();
    if(fault) {
        fault->Code(Arc::SOAPFault::Sender);
        fault->Reason("Failed processing request");
    };
    outmsg.Payload(outpayload);
    return Arc::MCC_Status();
}

/*
Arc::MCC_Status Service_PythonWrapper::python_error(const char *str) {
    return Arc::MCC_Status(Arc::GENERIC_ERROR);
}*/

class PythonLock {
  private:
    PyGILState_STATE gstate_;
    Arc::Logger& logger_;
  public:
    PythonLock(Arc::Logger& logger):logger_(logger) {
        gstate_ = PyGILState_Ensure();
        logger_.msg(Arc::VERBOSE, "Python interpreter locked");
    };
    ~PythonLock(void) {
        PyGILState_Release(gstate_);
        logger_.msg(Arc::VERBOSE, "Python interpreter released");
    };
};

class XMLNodeP {
  private:
    Arc::XMLNode* obj_;
  public:
    XMLNodeP(Arc::XMLNode& node):obj_(NULL) {
        try {
            obj_ = new Arc::XMLNode(node);
        } catch(std::exception& e) { };
    };
    ~XMLNodeP(void) {
        if(obj_) delete obj_;
    };
    XMLNode& operator*(void) const { return *obj_; };
    XMLNode* operator->(void) const { return obj_; };
    operator bool(void) { return (obj_ != NULL); };
    bool operator!(void) { return (obj_ == NULL); };
    operator long int(void) { return (long int)obj_; };
};

class SOAPMessageP {
  private:
    Arc::SOAPMessage* obj_;
  public:
    SOAPMessageP(Arc::Message& msg):obj_(NULL) {
        try {
            obj_ = new Arc::SOAPMessage(msg);
        } catch(std::exception& e) { };
    };
    ~SOAPMessageP(void) {
        if(obj_) delete obj_;
    };
    SOAPMessage& operator*(void) const { return *obj_; };
    SOAPMessage* operator->(void) const { return obj_; };
    operator bool(void) { return (obj_ != NULL); };
    bool operator!(void) { return (obj_ == NULL); };
    operator long int(void) { return (long int)obj_; };
};

class PyObjectP {
  private:
    PyObject* obj_;
  public:
    PyObjectP(PyObject* obj):obj_(obj) { };
  ~PyObjectP(void) { if(obj_) { Py_DECREF(obj_); } };
    operator bool(void) { return (obj_ != NULL); };
    bool operator!(void) { return (obj_ == NULL); };
    operator PyObject*(void) { return obj_; };
};

Arc::MCC_Status Service_PythonWrapper::process(Arc::Message& inmsg, Arc::Message& outmsg)
{
    //PyObject *py_status = NULL;
    //PyObject *py_inmsg = NULL;
    //PyObject *py_outmsg = NULL;
    PyObject *arg = NULL;

    logger.msg(Arc::VERBOSE, "Python wrapper process called");

    if(!initialized) return Arc::MCC_Status();

    PythonLock plock(logger);

    // Convert in message to SOAP message
    Arc::SOAPMessageP inmsg_ptr(inmsg);
    if(!inmsg_ptr) {
        logger.msg(Arc::ERROR, "Failed to create input SOAP container");
        return make_fault(outmsg);
    }
    if(!inmsg_ptr->Payload()) {
        logger.msg(Arc::ERROR, "input is not SOAP");
        return make_fault(outmsg);
    }
    // Convert incoming message to python object
    arg = Py_BuildValue("(l)", (long int)inmsg_ptr);
    if (arg == NULL) {
        logger.msg(Arc::ERROR, "Cannot create inmsg argument");
        if (PyErr_Occurred()) PyErr_Print();
        return make_fault(outmsg);
    }
    // arc_dict is a borrowed reference
    PyObject *arc_dict = PyModule_GetDict(arc_module);
    if (arc_dict == NULL) {
        logger.msg(Arc::ERROR, "Cannot get dictionary of ARC module");
        if (PyErr_Occurred()) PyErr_Print();
        return make_fault(outmsg);
    }
    // arc_msg_klass is a borrowed reference
    PyObject *arc_msg_klass = PyDict_GetItemString(arc_dict, "SOAPMessage");
    if (arc_msg_klass == NULL) {
        logger.msg(Arc::ERROR, "Cannot find ARC Message class");
        if (PyErr_Occurred()) PyErr_Print();
        return make_fault(outmsg);
    }
    PyObjectP py_inmsg(PyObject_CallObject(arc_msg_klass, arg));
    if (!py_inmsg) {
        logger.msg(Arc::ERROR, "Cannot convert inmsg to Python object");
        if (PyErr_Occurred()) PyErr_Print();
        Py_DECREF(arg);
        return make_fault(outmsg);
    }
    Py_DECREF(arg);

    Arc::SOAPMessageP outmsg_ptr(outmsg);
    if(!outmsg_ptr) {
        logger.msg(Arc::ERROR, "Failed to create SOAP containers");
        return make_fault(outmsg);
    }
    // Convert incoming and outcoming messages to python objects
    arg = Py_BuildValue("(l)", (long int)outmsg_ptr);
    if (arg == NULL) {
        logger.msg(Arc::ERROR, "Cannot create outmsg argument");
        if (PyErr_Occurred()) PyErr_Print();
        return make_fault(outmsg);
    }
    PyObjectP py_outmsg = PyObject_CallObject(arc_msg_klass, arg);
    if (!py_outmsg) {
        logger.msg(Arc::ERROR, "Cannot convert outmsg to Python object");
        if (PyErr_Occurred()) PyErr_Print();
        Py_DECREF(arg);
        return make_fault(outmsg);
    }
    Py_DECREF(arg);

    // Call the process method
    PyObjectP py_status(PyObject_CallMethod(object, (char*)"process", (char*)"(OO)",
                                    (PyObject*)py_inmsg, (PyObject*)py_outmsg));
    if (!py_status) {
        if (PyErr_Occurred()) PyErr_Print();
        return make_fault(outmsg);
    }

    MCC_Status *status_ptr2 = (MCC_Status *)extract_swig_wrappered_pointer(py_status);
    Arc::MCC_Status status;
    if(status_ptr2) status=(*status_ptr2);
    {
        // std::string str = (std::string)status;
        // std::cout << "status: " << str << std::endl;
    };
    SOAPMessage *outmsg_ptr2 = (SOAPMessage *)extract_swig_wrappered_pointer(py_outmsg);
    if(outmsg_ptr2 == NULL) return make_fault(outmsg);
    SOAPEnvelope *p = outmsg_ptr2->Payload();
    if(p == NULL) return make_fault(outmsg);
    {
        // std::string xml;
        // if(p) p->GetXML(xml);
        // std::cout << "XML: " << xml << std::endl;
    };

    Arc::PayloadSOAP *pl = new Arc::PayloadSOAP(*p);
    {
        // std::string xml;
        // pl->GetXML(xml);
        // std::cout << "XML: " << xml << std::endl;
    };

    outmsg.Payload(pl);
    return status;
}

bool Service_PythonWrapper::RegistrationCollector(Arc::XMLNode& doc) {
    PyObject *arg = NULL;

    // logger.msg(Arc::VERBOSE, "Python 'RegistrationCollector' wrapper process called");

    if(!initialized) return false;

    PythonLock plock(logger);

    // Convert doc to XMLNodeP
    // logger.msg(Arc::VERBOSE, "Convert doc to XMLNodeP");
    Arc::XMLNodeP doc_ptr(doc);
    if (!doc_ptr) {
        logger.msg(Arc::ERROR, "Failed to create XMLNode container");
        return false;
    }

    // Convert doc to Python object
    // logger.msg(Arc::VERBOSE, "Convert doc to Python object");
    // logger.msg(Arc::VERBOSE, "Create Python XMLNode");
    // arc_dict is a borrowed reference
    PyObject *arc_dict = PyModule_GetDict(arc_module);
    if (arc_dict == NULL) {
        logger.msg(Arc::ERROR, "Cannot get dictionary of ARC module");
        if (PyErr_Occurred()) PyErr_Print();
        return false;
    }
    // arc_xmlnode_klass is a borrowed reference
    PyObject *arc_xmlnode_klass = PyDict_GetItemString(arc_dict, "XMLNode");
    if (arc_xmlnode_klass == NULL) {
        logger.msg(Arc::ERROR, "Cannot find ARC XMLNode class");
        if (PyErr_Occurred()) PyErr_Print();
        return false;
    }
    arg = Py_BuildValue("(l)", (long int)doc_ptr);
    if (arg == NULL) {
        logger.msg(Arc::ERROR, "Cannot create doc argument");
        if (PyErr_Occurred()) PyErr_Print();
        return false;
    }
    PyObjectP py_doc(PyObject_CallObject(arc_xmlnode_klass, arg));
        if (!py_doc) {
        logger.msg(Arc::ERROR, "Cannot convert doc to Python object");
        if (PyErr_Occurred()) PyErr_Print();
        Py_DECREF(arg);
        return false;
    }
    Py_DECREF(arg);

    // Call the RegistrationCollector method
    // logger.msg(Arc::VERBOSE, "Call the RegistrationCollector method");
    PyObjectP py_bool(PyObject_CallMethod(object, (char*)"RegistrationCollector", (char*)"(O)",
                      (PyObject*)py_doc));

    if (!py_bool) {
        if (PyErr_Occurred()) PyErr_Print();
        return false;
    }

    // Convert the return value of the function back to cpp
    // logger.msg(Arc::VERBOSE, "Convert the return value of the function back to cpp");
    bool *ret_val2 = (bool *)extract_swig_wrappered_pointer(py_bool);
    bool return_value = false;
    if (ret_val2) return_value = (*ret_val2);

    XMLNode *doc2 = (XMLNode *)extract_swig_wrappered_pointer(py_doc);
    if (doc2 == NULL) return false;
    (*doc2).New(doc);
    return true;
}

} // namespace Arc

