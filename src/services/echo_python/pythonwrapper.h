#ifndef __ARC_SERVICE_PYTHON_WRAPPER_H__
#define __ARC_SERVICE_PYTHON_WRAPPER_H__

#include <Python.h>
#include <arc/message/Service.h>
#include <arc/Logger.h>

namespace Arc {
class Service_PythonWrapper: public Arc::Service {
    protected:
        Arc::MCC_Status make_fault(Arc::Message& outmsg);
        static Arc::Logger logger;
        PyObject *arc_module;
        PyObject *arc_msg_klass;
        PyObject *module;
        PyObject *klass;
        PyObject *object;

    public:
        Service_PythonWrapper(Arc::Config *cfg);
        virtual ~Service_PythonWrapper(void);
        /** Service request processing routine */
        virtual Arc::MCC_Status process(Arc::Message&, Arc::Message&);
};

}; // namespace Arc

#endif // __ARC_SERVICE_PYTHON_WRAPPER_H__
