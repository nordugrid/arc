// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SUBMITTER_H__
#define __ARC_SUBMITTER_H__

#include <list>
#include <string>

#include <arc/URL.h>
#include <arc/loader/Loader.h>
#include <arc/loader/Plugin.h>

namespace Arc {

  class Config;
  class JobDescription;
  class Logger;
  class UserConfig;

  //! Base class for the Submitters
  /// Must be specialiced for each supported middleware flavour.
  class Submitter
    : public Plugin {
  protected:
    Submitter(const Config& cfg, const UserConfig& usercfg,
              const std::string& flavour);
  public:
    virtual ~Submitter();
    virtual URL Submit(const JobDescription& jobdesc,
                       const std::string& joblistfile) const = 0;
    virtual URL Migrate(const URL& jobid, const JobDescription& jobdesc,
                        bool forcemigration,
                        const std::string& joblistfile) const = 0;
    std::string GetCksum(const std::string& file) const;
  protected:
    bool PutFiles(const JobDescription& jobdesc, const URL& url) const;
    void AddJob(const JobDescription& job, const URL& jobid,
                const URL& infoendpoint,
                const std::string& joblistfile) const;

    const std::string flavour;
    const UserConfig& usercfg;
    const URL submissionEndpoint;
    const URL cluster;
    const std::string queue;
    const std::string lrmsType;
    static Logger logger;
  };

  //! Class responsible for loading Submitter plugins
  /// The Submitter objects returned by a SubmitterLoader
  /// must not be used after the SubmitterLoader goes out of scope.
  class SubmitterLoader
    : public Loader {

  public:
    //! Constructor
    /// Creates a new SubmitterLoader.
    SubmitterLoader();

    //! Destructor
    /// Calling the destructor destroys all Submitters loaded
    /// by the SubmitterLoader instance.
    ~SubmitterLoader();

    //! Load a new Submitter
    /// \param name    The name of the Submitter to load.
    /// \param cfg     The Config object for the new Submitter.
    /// \param usercfg The UserConfig object for the new Submitter.
    /// \returns       A pointer to the new Submitter (NULL on error).
    Submitter* load(const std::string& name,
                    const Config& cfg, const UserConfig& usercfg);

    //! Retrieve the list of loaded Submitters.
    /// \returns A reference to the list of Submitters.
    const std::list<Submitter*>& GetSubmitters() const {
      return submitters;
    }

  private:
    std::list<Submitter*> submitters;
  };

  class SubmitterPluginArgument
    : public PluginArgument {
  public:
    SubmitterPluginArgument(const Config& cfg, const UserConfig& usercfg)
      : cfg(cfg),
        usercfg(usercfg) {}
    ~SubmitterPluginArgument() {}
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

#endif // __ARC_SUBMITTER_H__
