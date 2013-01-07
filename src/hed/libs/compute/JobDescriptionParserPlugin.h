// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBDESCRIPTIONPARSERPLUGIN_H__
#define __ARC_JOBDESCRIPTIONPARSERPLUGIN_H__

/** \file
 * \brief Plugin, loader and argument classes for job description parser specialisation.
 */

#include <map>
#include <string>

#include <arc/loader/Loader.h>
#include <arc/loader/Plugin.h>


namespace Arc {

  class JobDescription;
  class Logger;

  /**
   * \ingroup accplugins
   * \headerfile JobDescriptionParserPlugin.h arc/compute/JobDescriptionParserPlugin.h
   */
  class JobDescriptionParserPluginResult {
  public:
    typedef enum {
      Success,
      Failure,
      WrongLanguage
    } Result;
    JobDescriptionParserPluginResult(void):v_(Success) { };
    JobDescriptionParserPluginResult(bool v):v_(v?Success:Failure) { };
    JobDescriptionParserPluginResult(Result v):v_(v) { };
    operator bool(void) { return (v_ == Success); };
    bool operator!(void) { return (v_ != Success); };
    bool operator==(bool v) { return ((v_ == Success) == v); };
    bool operator==(Result v) { return (v_ == v); };
  private:
    Result v_;
  };

  /// Abstract class for the different parsers
  /**
   * The JobDescriptionParserPlugin class is abstract which provide a interface
   * for job description parsers. A job description parser should inherit this
   * class and overwrite the JobDescriptionParserPlugin::Parse and
   * JobDescriptionParserPlugin::UnParse methods.
   * 
   * \ingroup accplugins
   * \headerfile JobDescriptionParserPlugin.h arc/compute/JobDescriptionParserPlugin.h
   */
  class JobDescriptionParserPlugin
    : public Plugin {
  public:
    virtual ~JobDescriptionParserPlugin();

    virtual JobDescriptionParserPluginResult Parse(const std::string& source, std::list<JobDescription>& jobdescs, const std::string& language = "", const std::string& dialect = "") const = 0;
    virtual JobDescriptionParserPluginResult UnParse(const JobDescription& job, std::string& output, const std::string& language, const std::string& dialect = "") const = 0;
    const std::list<std::string>& GetSupportedLanguages() const { return supportedLanguages; }
    bool IsLanguageSupported(const std::string& language) const { return std::find(supportedLanguages.begin(), supportedLanguages.end(), language) != supportedLanguages.end(); }
    const std::string& GetError(void) { return error; };

  protected:
    JobDescriptionParserPlugin(PluginArgument* parg);

    std::string& SourceLanguage(JobDescription& j) const;

    std::list<std::string> supportedLanguages;

    mutable std::string error;

    static Logger logger;
  };

  /** Class responsible for loading JobDescriptionParserPlugin plugins
   * The JobDescriptionParserPlugin objects returned by a
   * JobDescriptionParserPluginLoader must not be used after the
   * JobDescriptionParserPluginLoader goes out of scope.
   * 
   * \ingroup accplugins
   * \headerfile JobDescriptionParserPlugin.h arc/compute/JobDescriptionParserPlugin.h
   */
  class JobDescriptionParserPluginLoader
    : public Loader {

  public:
    /** Constructor
     * Creates a new JobDescriptionParserPluginLoader.
     */
    JobDescriptionParserPluginLoader();

    /** Destructor
     * Calling the destructor destroys all JobDescriptionParserPlugin object
     * loaded by the JobDescriptionParserPluginLoader instance.
     */
    ~JobDescriptionParserPluginLoader();

    /** Load a new JobDescriptionParserPlugin
     * \param name The name of the JobDescriptionParserPlugin to load.
     * \return A pointer to the new JobDescriptionParserPlugin (NULL on error).
     */
    JobDescriptionParserPlugin* load(const std::string& name);

    /** Retrieve the list of loaded JobDescriptionParserPlugin objects.
     * \return A reference to the list of JobDescriptionParserPlugin objects.
     */
    const std::list<JobDescriptionParserPlugin*>& GetJobDescriptionParserPlugins() const { return jdps; }

    class iterator {
    private:
      iterator(JobDescriptionParserPluginLoader& jdpl);
      iterator& operator=(const iterator& it) { return *this; }
    public:
      ~iterator() {}
      //iterator& operator=(const iterator& it) { current = it.current; jdpl = it.jdpl; return *this; }
      JobDescriptionParserPlugin& operator*() { return **current; }
      const JobDescriptionParserPlugin& operator*() const { return **current; }
      JobDescriptionParserPlugin* operator->() { return *current; }
      const JobDescriptionParserPlugin* operator->() const { return *current; }
      iterator& operator++();
      operator bool() { return !jdpl->jdpDescs.empty() || current != jdpl->jdps.end(); }

      friend class JobDescriptionParserPluginLoader;
    private:
      void LoadNext();

      std::list<JobDescriptionParserPlugin*>::iterator current;
      JobDescriptionParserPluginLoader* jdpl;
    };

    iterator GetIterator() { return iterator(*this); }

  private:
    std::list<JobDescriptionParserPlugin*> jdps;
    std::list<ModuleDesc> jdpDescs;

    void scan();
    bool scaningDone;
  };
} // namespace Arc

#endif // __ARC_JOBDESCRIPTIONPARSERPLUGIN_H__
