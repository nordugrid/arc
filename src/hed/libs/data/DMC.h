#ifndef __ARC_DMC_H__
#define __ARC_DMC_H__

#include <list>

#include <glibmm/thread.h>

#include <arc/ArcConfig.h>

namespace Arc {

  class ChainContext;
  class Config;
  class DataPoint;
  class Loader;
  class Logger;
  class URL;

  class DMC {
  protected:
    DMC(Config *) {}
  public:
    virtual ~DMC() {}
    virtual DataPoint *iGetDataPoint(const URL& url) = 0;
    static DataPoint *GetDataPoint(const URL& url);
  protected:
    static Logger logger;
    static void Register(DMC *dmc);
    static void Unregister(DMC *dmc);
  private:
    static void Load();
    static std::list<DMC *> dmcs;
    static Glib::StaticMutex mutex;
    static Loader *loader;
  };

  class DMCConfig
    : public BaseConfig {
  public:
    DMCConfig()
      : BaseConfig() {}
    virtual ~DMCConfig() {}
    virtual XMLNode MakeConfig(XMLNode cfg) const;
  };

} // namespace Arc

#endif // __ARC_DMC_H__
