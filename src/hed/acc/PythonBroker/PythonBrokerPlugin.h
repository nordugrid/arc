// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_PYTHONBROKERPLUGIN_H__
#define __ARC_PYTHONBROKERPLUGIN_H__

#include <Python.h>
#include <arc/UserConfig.h>
#include <arc/compute/Broker.h>

namespace Arc {

  class PythonBrokerPlugin : public BrokerPlugin {
  public:
    PythonBrokerPlugin(BrokerPluginArgument* parg);
    virtual ~PythonBrokerPlugin();
    static Plugin* Instance(PluginArgument *arg);
    
    bool operator()(const ExecutionTarget&, const ExecutionTarget&) const;
    bool match(const ExecutionTarget&) const;
    void set(const JobDescription&);

  private:
    PyObject *arc_module;
    PyObject *arc_userconfig_klass;
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

#endif // __ARC_PYTHONBROKERPLUGIN_H__
