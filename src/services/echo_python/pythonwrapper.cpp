// based on: http://www.codeproject.com/cpp/embedpython_1.asp
#ifdef HAVE_CONFIG
#include <config.h>
#endif

#include <iostream>
#include <arc/loader/Loader.h>
#include <arc/loader/ServiceLoader.h>
#include <arc/message/SOAPMessage.h>
#include "pythonwrapper.h"

static Arc::Service* get_service(Arc::Config *cfg,Arc::ChainContext *ctx) {
    return new Arc::Service_PythonWrapper(cfg);
}

service_descriptors ARC_SERVICE_LOADER = {
    { "arcservice_pythonwrapper", 0, &get_service },
    { NULL, 0, NULL }
};

namespace Arc {

Arc::Logger Service_PythonWrapper::logger(Service::logger, "PythonWrapper");

Service_PythonWrapper::Service_PythonWrapper(Arc::Config *cfg):Service(cfg) 
{
    PyObject *module_name = NULL;
    PyObject *dict = NULL;
    
    // Initialize the Python Interpreter
    Py_Initialize();
    // Convert module name to Python string
    module_name = PyString_FromString("EchoService");
    if (module_name == NULL) {
        logger.msg(Arc::ERROR, "Cannot convert module name to Python string");
        if (PyErr_Occurred() != NULL) {
            PyErr_Print();
        }
        return;
    }
    // Load module
    module = PyImport_Import(module_name);
    if (module == NULL) {
        logger.msg(Arc::ERROR, "Cannot import module");
        if (PyErr_Occurred() != NULL) {
            PyErr_Print();
        }
        Py_DECREF(module_name);
        return;
    }
    Py_DECREF(module_name);
    
    // Get dictionary of module content
    // dict is a borrowed reference 
    dict = PyModule_GetDict(module);
    if (dict == NULL) {
        logger.msg(Arc::ERROR, "Cannot get dictionary of module");
        if (PyErr_Occurred() != NULL) {
            PyErr_Print();
        }
        return;
    }
    
    // Get the class 
    klass = PyDict_GetItemString(dict, "EchoService");
    if (klass == NULL) {
        logger.msg(Arc::ERROR, "Cannot find service class");
        if (PyErr_Occurred() != NULL) {
            PyErr_Print();
        }
        return;
    }
    
    // check is it really a class
    if (PyCallable_Check(klass)) {
        // create instance of class
        object = PyObject_CallObject(klass, NULL);
        if (object == NULL) {
            logger.msg(Arc::ERROR, "Cannot create instance of python class");
            if (PyErr_Occurred() != NULL) {
                PyErr_Print();
            }
            return;
        }
    } else {
        logger.msg(Arc::ERROR, "%s is not an object", "EchoService");
        return;
    }

    // Import ARC python wrapper
    module_name = PyString_FromString("arc");
    if (module_name == NULL) {
        logger.msg(Arc::ERROR, "Cannot convert arc odule name to Python string");
        if (PyErr_Occurred() != NULL) {
            PyErr_Print();
        }
        return;
    }

    // Load arc module
    arc_module = PyImport_Import(module_name);
    if (arc_module == NULL) {
        logger.msg(Arc::ERROR, "Cannot import arc module");
        if (PyErr_Occurred() != NULL) {
            PyErr_Print();
        }
        Py_DECREF(module_name);
        return;
    }
    Py_DECREF(module_name);
    
    dict = PyModule_GetDict(arc_module);
    if (dict == NULL) {
        logger.msg(Arc::ERROR, "Cannot get dictionary of arc module");
        if (PyErr_Occurred() != NULL) {
            PyErr_Print();
        }
        return;
    }
    
    // Get the class 
    arc_msg_klass = PyDict_GetItemString(dict, "SOAPMessage");
    if (arc_msg_klass == NULL) {
        logger.msg(Arc::ERROR, "Cannot find arc Message class");
        if (PyErr_Occurred() != NULL) {
            PyErr_Print();
        }
        return;
    }
    
    // check is it really a class
    if (!PyCallable_Check(klass)) {
        logger.msg(Arc::ERROR, "Message klass is not an object");
        return;
    }

    logger.msg(Arc::DEBUG, "Python Wrapper constructur called");
}

Service_PythonWrapper::~Service_PythonWrapper(void) 
{
    // Finish the Python Interpreter
    Py_Finalize();
    logger.msg(Arc::DEBUG, "Python Wrapper destructor called");
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

Arc::MCC_Status Service_PythonWrapper::process(Arc::Message& inmsg, Arc::Message& outmsg) 
{
    PyObject *py_status = NULL;
    PyObject *py_inmsg = NULL;
    PyObject *py_outmsg = NULL;
    PyObject *inmsg_addr = NULL;
    PyObject *outmsg_addr = NULL;
    PyObject *arg = NULL;

    logger.msg(Arc::DEBUG, "Python wrapper process called");
    // Convert in and out messages to SOAP messages
    Arc::SOAPMessage *inmsg_ptr = NULL;
    Arc::SOAPMessage *outmsg_ptr = NULL;
    try {
        inmsg_ptr = new Arc::SOAPMessage(inmsg);
        outmsg_ptr = new Arc::SOAPMessage(outmsg);
    } catch(std::exception& e) { };
    if(!inmsg_ptr) {
        logger.msg(Arc::ERROR, "input is not SOAP");
        return make_fault(outmsg);
    };
    if(!outmsg_ptr) {
        logger.msg(Arc::ERROR, "output is not SOAP");
        return make_fault(outmsg);
    };

    // Convert address of C++ class to Python object 
    inmsg_addr = PyLong_FromVoidPtr(inmsg_ptr);
    if (inmsg_addr == NULL) {
        logger.msg(Arc::ERROR, "Cannot convert inmsg to address");
        if (PyErr_Occurred() != NULL) {
            PyErr_Print();
        }
        return Arc::MCC_Status(Arc::GENERIC_ERROR);
    }

    outmsg_addr = PyLong_FromVoidPtr(outmsg_ptr);
    if (outmsg_addr == NULL) {
        logger.msg(Arc::ERROR, "Cannot convert outmsg to address");
        if (PyErr_Occurred() != NULL) {
            PyErr_Print();
        }
        Py_DECREF(inmsg_addr);
        return Arc::MCC_Status(Arc::GENERIC_ERROR);
    }
    // Convert incomming and outcoming messages to python objects
    arg = Py_BuildValue("(I)", inmsg_addr);
    if (arg == NULL) {
        logger.msg(Arc::ERROR, "Cannot create inmsg argument");
        if (PyErr_Occurred() != NULL) {
            PyErr_Print();
        }
        Py_DECREF(inmsg_addr);
        Py_DECREF(outmsg_addr);
        return Arc::MCC_Status(Arc::GENERIC_ERROR);
    }

    py_inmsg = PyObject_CallObject(arc_msg_klass, arg);
    if (py_inmsg == NULL) {
        logger.msg(Arc::ERROR, "Cannot convert inmsg to python object");
        if (PyErr_Occurred() != NULL) {
            PyErr_Print();
        }
        Py_DECREF(inmsg_addr);
        Py_DECREF(outmsg_addr);
        Py_DECREF(arg);
        return Arc::MCC_Status(Arc::GENERIC_ERROR);
    }
    Py_DECREF(inmsg_addr);
    Py_DECREF(arg);

    arg = Py_BuildValue("(I)", outmsg_addr);
    if (arg == NULL) {
        logger.msg(Arc::ERROR, "Cannot create inmsg argument");
        if (PyErr_Occurred() != NULL) {
            PyErr_Print();
        }
        Py_DECREF(outmsg_addr);
        return Arc::MCC_Status(Arc::GENERIC_ERROR);
    }
    py_outmsg = PyObject_CallObject(arc_msg_klass, arg);
    if (py_outmsg == NULL) {
        logger.msg(Arc::ERROR, "Cannot convert outmsg to python object");
        if (PyErr_Occurred() != NULL) {
            PyErr_Print();
        }
        Py_DECREF(outmsg_addr);
        Py_DECREF(arg);
        return Arc::MCC_Status(Arc::GENERIC_ERROR);
    }
    Py_DECREF(outmsg_addr);
    Py_DECREF(arg);
    
    // Call the process method
    py_status = PyObject_CallMethod(object, "process", "(OO)", 
                                    py_inmsg, py_outmsg);
    if (py_status == NULL) {
        if (PyErr_Occurred() != NULL) {
                PyErr_Print();
        }
        return Arc::MCC_Status(Arc::GENERIC_ERROR);
    }
    return Arc::MCC_Status();
}

}; // namespace Arc
