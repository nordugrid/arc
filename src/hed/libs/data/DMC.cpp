#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "DMC.h"
#include <arc/Logger.h>
#include <arc/ArcConfig.h>
#include <arc/misc/ClientInterface.h>
#include <arc/loader/Loader.h>

namespace Arc {

  Logger DMC::logger(Logger::rootLogger, "DMC");

  std::list<DMC *> DMC::dmcs;

  Glib::Mutex DMC::mutex;

  DataPoint *DMC::GetDataPoint(const URL& url) {
    DataPoint *dp = NULL;
    Glib::Mutex::Lock lock(mutex);
    for (std::list<DMC *>::iterator i = dmcs.begin();
	 !dp && (i != dmcs.end()); i++)
      dp = (*i)->iGetDataPoint(url);
    return dp;
  }

  void DMC::Register(DMC *dmc) {
    Glib::Mutex::Lock lock(mutex);
    dmcs.push_back(dmc);
  }

  void DMC::Unregister(DMC *dmc) {
    Glib::Mutex::Lock lock(mutex);
    dmcs.remove(dmc);
  }


  static class DMCLoader {

  public:
    DMCLoader() {
      DMCConfig dmcconf;
      NS ns;
      Config cfg(ns);
      dmcconf.MakeConfig(cfg);
      loader = new Loader(&cfg);
    }

    ~DMCLoader() {}

  private:
    Loader *loader;

  } dmcloader;

} // namespace Arc
