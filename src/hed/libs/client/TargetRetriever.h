// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_TARGETRETRIEVER_H__
#define __ARC_TARGETRETRIEVER_H__

#include <list>
#include <string>

#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/loader/Loader.h>
#include <arc/loader/Plugin.h>

namespace Arc {

  class Logger;
  class TargetGenerator;

  /// %TargetRetriever base class
  /**
   * The TargetRetriever class is a pure virtual base class to be used
   * for grid flavour specializations. It is designed to work in
   * conjunction with the TargetGenerator.
   **/
  class TargetRetriever
    : public Plugin {
    friend class TargetGenerator;
  protected:

    /// TargetRetriever constructor
    /**
     * Default constructor to create a TargeGenerator. The constructor
     * reads the computing and index service URL objects from the
     *
     * @param usercfg
     * @param url
     * @param st
     * @param flavour
     **/
    TargetRetriever(const UserConfig& usercfg,
                    const URL& url, ServiceType st,
                    const std::string& flavour);
  public:
    virtual ~TargetRetriever();

    /// DEPRECATED: Method for collecting targets
    /**
     * This method is DEPRECATED, the GetExecutionTargets and GetJobs methods
     * replaces it.
     *
     * Pure virtual method for collecting targets. Implementation
     * depends on the Grid middleware in question and is thus left to
     * the specialized class.
     *
     * @param mom is the reference to the TargetGenerator which has loaded the TargetRetriever
     * @param targetType is the identificaion of targets to find (0 = ExecutionTargets, 1 = Grid Jobs)
     * @param detailLevel is the required level of details (1 = All details, 2 = Limited details)
     **/
    virtual void GetTargets(TargetGenerator& mom, int targetType,
                            int detailLevel) = 0;

  protected:
    /// Method for collecting targets
    /**
     * Pure virtual method for collecting targets. Implementation
     * depends on the Grid middleware in question and is thus left to
     * the specialized class.
     *
     * @param mom is the reference to the TargetGenerator which has loaded the TargetRetriever
     * @param detailLevel is the required level of details (1 = All details, 2 = Limited details)
     **/
    virtual void GetExecutionTargets(TargetGenerator& mom) = 0;

    /// Method for collecting targets
    /**
     * Pure virtual method for collecting targets. Implementation
     * depends on the Grid middleware in question and is thus left to
     * the specialized class.
     *
     * @param mom is the reference to the TargetGenerator which has loaded the TargetRetriever
     * @param detailLevel is the required level of details (1 = All details, 2 = Limited details)
     **/
    virtual void GetJobs(TargetGenerator& mom) = 0;

    const std::string flavour;
    const UserConfig& usercfg;
    const URL url;
    const ServiceType serviceType;

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
    /// \param usercfg The UserConfig object for the new TargetRetriever.
    /// \param service The URL used to contact the target.
    /// \param st      specifies service type of the target.
    /// \returns       A pointer to the new TargetRetriever (NULL on error).
    TargetRetriever* load(const std::string& name,
                          const UserConfig& usercfg,
                          const std::string& service,
                          const ServiceType& st);

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
    TargetRetrieverPluginArgument(const UserConfig& usercfg,
				  const std::string& server,
				  const ServiceType& st)
      : usercfg(usercfg), server(server), st(st) {}
    ~TargetRetrieverPluginArgument() {}
    operator const UserConfig&() {
      return usercfg;
    }
    operator const std::string&() {
      return server;
    }
    operator const ServiceType&() {
      return st;
    }
  private:
    const UserConfig& usercfg;
    const std::string& server;
    const ServiceType& st;
  };

} // namespace Arc

#endif // __ARC_TARGETRETRIEVER_H__
