#ifndef __ARC_PYTHON_BROKER_H__
#define __ARC_PYTHON_BROKER_H__

#include <Python.h>
#include <arc/client/Broker.h>

namespace Arc {

  class PythonBroker
    : public Broker {

  public:
    PythonBroker(Config *cfg);
    virtual ~PythonBroker();
    static Plugin* Instance(PluginArgument *arg);

  protected:
    virtual void SortTargets();

  private:
    PyObject *arc_module;
    PyObject *arc_config_klass;
    PyObject *arc_jobrepr_klass;
    PyObject *arc_xtarget_klass;
    PyObject *module;
    PyObject *klass;
    PyObject *object;

    static Logger logger;
    static PyThreadState *tstate;
    static int refcount;
    static Glib::Mutex lock;
  };

} // namespace Arc

#endif // __ARC_PYTHON_BROKER_H__
