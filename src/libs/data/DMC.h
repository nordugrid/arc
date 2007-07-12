#ifndef __ARC_DMC_H__
#define __ARC_DMC_H__

#include <list>
#include <glibmm.h>

namespace Arc {

  class ChainContext;
  class Config;
  class DataPoint;
  class Logger;
  class URL;

  class DMC {
   protected:
    DMC(Config *cfg) {};
   public:
    virtual ~DMC() {};
    virtual DataPoint* iGetDataPoint(const URL& url) = 0;
    static DataPoint* GetDataPoint(const URL& url);
   protected:
    static Logger logger;
    static void Register(DMC* dmc);
    static void Unregister(DMC* dmc);
   private:
    static std::list<DMC*> dmcs;
    static Glib::Mutex mutex;
  };

} // namespace Arc

#endif // __ARC_DMC_H__
