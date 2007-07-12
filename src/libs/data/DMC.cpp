#include "DMC.h"
#include "common/Logger.h"

namespace Arc {

  Logger DMC::logger(Logger::rootLogger, "DMC");

  std::list<DMC*> DMC::dmcs;

  Glib::Mutex DMC::mutex;

  DataPoint* DMC::GetDataPoint(const URL& url) {
    DataPoint* dp = NULL;
    Glib::Mutex::Lock lock(mutex);
    for (std::list<DMC*>::iterator i = dmcs.begin();
	 !dp && (i != dmcs.end()); i++) {
      dp = (*i)->iGetDataPoint(url);
    }
    return dp;
  }

  void DMC::Register(DMC* dmc) {
    Glib::Mutex::Lock lock(mutex);
    dmcs.push_back(dmc);
  }

  void DMC::Unregister(DMC* dmc) {
    Glib::Mutex::Lock lock(mutex);
    dmcs.remove(dmc);
  }

} // namespace Arc
