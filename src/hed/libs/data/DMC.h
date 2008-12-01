#ifndef __ARC_DMC_H__
#define __ARC_DMC_H__

#include <list>

#include <glibmm/thread.h>

#include <arc/ArcConfig.h>
#include <arc/loader/Plugin.h>

namespace Arc {

  class DMCLoader;
  class DataPoint;
  class Logger;
  class URL;

  class DMC: public Plugin {
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
    static DMCLoader *loader;
  };

  class DMCConfig
    : public BaseConfig {
  public:
    DMCConfig()
      : BaseConfig() {}
    virtual ~DMCConfig() {}
    virtual XMLNode MakeConfig(XMLNode cfg) const;
  };

  #define DMCPluginKind ("HED:DMC")

  class DMCPluginArgument: public PluginArgument {
   private:
    Config* config_;
   public:
    DMCPluginArgument(Config* config):config_(config) { };
    virtual ~DMCPluginArgument(void) { };
    operator Config* (void) { return config_; };
  };

} // namespace Arc

#endif // __ARC_DMC_H__
