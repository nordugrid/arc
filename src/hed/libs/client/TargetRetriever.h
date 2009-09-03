// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_TARGETRETRIEVER_H__
#define __ARC_TARGETRETRIEVER_H__

#include <list>
#include <string>

#include <arc/URL.h>
#include <arc/loader/Loader.h>
#include <arc/loader/Plugin.h>

namespace Arc {

  class Config;
  class Logger;
  class TargetGenerator;
  class UserConfig;

  //! Base class for the TargetRetrievers
  /// Must be specialiced for each supported middleware flavour.
  class TargetRetriever
    : public Plugin {
  protected:
    TargetRetriever(const Config& cfg, const UserConfig& usercfg,
                    const std::string& flavour);
  public:
    virtual ~TargetRetriever();
    virtual void GetTargets(TargetGenerator& mom, int targetType,
                            int detailLevel) = 0;
  protected:
    const std::string flavour;
    const UserConfig& usercfg;
    const URL url;
    const std::string serviceType;
    static Logger logger;
  };

  //! Class responsible for loading TargetRetriever plugins
  /// The TargetRetriever objects returned by a TargetRetrieverLoader
  /// must not be used after the TargetRetrieverLoader goes out of scope.
  class TargetRetrieverLoader
    : public Loader {

  public:
    //! Constructor
    /// Creates a new TargetRetrieverLoader.
    TargetRetrieverLoader();

    //! Destructor
    /// Calling the destructor destroys all TargetRetrievers loaded
    /// by the TargetRetrieverLoader instance.
    ~TargetRetrieverLoader();

    //! Load a new TargetRetriever
    /// \param name    The name of the TargetRetriever to load.
    /// \param cfg     The Config object for the new TargetRetriever.
    /// \param usercfg The UserConfig object for the new TargetRetriever.
    /// \returns       A pointer to the new TargetRetriever (NULL on error).
    TargetRetriever* load(const std::string& name,
                          const Config& cfg, const UserConfig& usercfg);

    //! Retrieve the list of loaded TargetRetrievers.
    /// \returns A reference to the list of TargetRetrievers.
    const std::list<TargetRetriever*>& GetTargetRetrievers() const {
      return targetretrievers;
    }

  private:
    std::list<TargetRetriever*> targetretrievers;
  };

  class TargetRetrieverPluginArgument
    : public PluginArgument {
  public:
    TargetRetrieverPluginArgument(const Config& cfg, const UserConfig& usercfg)
      : cfg(cfg),
        usercfg(usercfg) {}
    ~TargetRetrieverPluginArgument() {}
    operator const Config&() {
      return cfg;
    }
    operator const UserConfig&() {
      return usercfg;
    }
  private:
    const Config& cfg;
    const UserConfig& usercfg;
  };

} // namespace Arc

#endif // __ARC_TARGETRETRIEVER_H__
