// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBDESCRIPTIONPARSERPLUGIN_H__
#define __ARC_JOBDESCRIPTIONPARSERPLUGIN_H__

/** \file
 * \brief Plugin, loader and argument classes for job description parser specialisation.
 */

#include <map>
#include <string>
#include <utility>
#include <algorithm>

#include <arc/loader/Loader.h>
#include <arc/loader/Plugin.h>


namespace Arc {

  class JobDescription;
  class Logger;

  /**
   * \ingroup accplugins
   * \headerfile JobDescriptionParserPlugin.h arc/compute/JobDescriptionParserPlugin.h
   */


  /**
   * \since Added in 5.1.0
   **/
  class JobDescriptionParsingError {
  public:
    JobDescriptionParsingError() {}
    JobDescriptionParsingError(const std::string& message, const std::pair<int, int>& line_pos = std::make_pair(0, 0), const std::string& failing_code = "")
      : message(message), failing_code(failing_code), line_pos(line_pos) {}
    ~JobDescriptionParsingError() {}
    std::string message;
    std::string failing_code;
    std::pair<int, int> line_pos;
  };

  class JobDescriptionParserPluginResult {
  public:
    typedef enum {
      Success,
      Failure, /**< Parsing failed **/
      WrongLanguage
    } Result;
    JobDescriptionParserPluginResult(void):v_(Success) { };
    JobDescriptionParserPluginResult(bool v):v_(v?Success:Failure) { };
    JobDescriptionParserPluginResult(Result v):v_(v) { };
    operator bool(void) { return (v_ == Success); };
    bool operator!(void) { return (v_ != Success); };
    bool operator==(bool v) { return ((v_ == Success) == v); };
    bool operator==(Result v) { return (v_ == v); };
    /**
     * \since Added in 5.1.0
     **/
    bool HasErrors() const { return !errors_.empty(); };
    /**
     * \since Added in 5.1.0
     **/
    const std::list<JobDescriptionParsingError>& GetErrors() const { return errors_; };
    /**
     * \since Added in 5.1.0
     **/
    void AddError(const JobDescriptionParsingError& error) { errors_.push_back(error); };
    /**
     * \since Added in 5.1.0
     **/
    void AddError(const IString& msg,
                  const std::pair<int, int>& location = std::make_pair(0, 0),
                  const std::string& failing_code = "") {
      errors_.push_back(JobDescriptionParsingError(msg.str(), location, failing_code));
    }
    /**
     * \since Added in 5.1.0
     **/
    void SetSuccess() { v_ = Success; };
    /**
     * \since Added in 5.1.0
     **/
    void SetFailure() { v_ = Failure; };
    /**
     * \since Added in 5.1.0
     **/
    void SetWrongLanguage() {v_ = WrongLanguage; };
  private:
    Result v_;
    std::list<JobDescriptionParsingError> errors_;
  };

  /// Abstract plugin class for different parsers
  /**
   * The JobDescriptionParserPlugin class is abstract which provide an interface
   * for job description parsers. A job description parser should inherit this
   * class and overwrite the JobDescriptionParserPlugin::Parse and
   * JobDescriptionParserPlugin::UnParse methods. The inheriating class should
   * add the job description languages that it supports to the
   * 'supportedLanguages' member, formatted according to the GLUE2
   * JobDescription_t type (GFD-R-P.147). The created job description parser
   * will then be available to the JobDescription::Parse,
   * JobDescription::ParseFromFile and JobDescription::Assemble methods, adding
   * the ability to parse and assemble job descriptions in the specified
   * languages.
   *
   * Using the methods in JobDescription class for parsing job descriptions is
   * recommended, however it is also possible to use parser plugins directly,
   * which can be done by loading them using the
   * JobDescriptionParserPluginLoader class.
   *
   * Since this class inheriates from the Plugin class, inheriating classes
   * should be compiled as a loadable module. See xxx for information on
   * creating loadable modules for ARC.
   *
   * \ingroup accplugins
   * \headerfile JobDescriptionParserPlugin.h arc/compute/JobDescriptionParserPlugin.h
   */
  class JobDescriptionParserPlugin
    : public Plugin {
  public:
    virtual ~JobDescriptionParserPlugin();

    /// Parse string into JobDescription objects
    /**
     * Parses the string argument \p source into JobDescription objects. If the
     * \p language argument is specified the method will only parse the string
     * if it supports that language - a JobDescriptionParserPluginResult object
     * with status \ref JobDescriptionParserPluginResult::WrongLanguage "WrongLanguage"
     * is returned if language is not supported. Similar for the \p dialect
     * argument, if specified, string is only parsed if that dialect is known
     * by parser.
     * If the \p language argument is not specified an attempt at parsing
     * \p source using any of the supported languages is tried.
     * If parsing is successful the generated JobDescription objects is appended
     * to the \p jobdescs argument. If parsing is unsuccessful the \p jobdescs
     * argument is untouched, and details of the failure is returned.
     *
     * Inheriating classes must extend this method. The extended method should
     * parse the \p source argument string into a JobDescription object, possibly
     * into multiple objects. Some languages can contain multiple alternative
     * views, in such cases alternatives should be added using the
     * JobDescription::AddAlternative method. Only if parsing is successful
     * should the generated JobDescription objects be added to the \p jobdescs
     * argument. Note: The only allowed modification of the \p jobdescs list is
     * adding elements. If the \p language argument is specified parsing should
     * only be initiated if the specified language is among the supported
     * ones, if that is not the case \ref JobDescriptionParserPluginResult::WrongLanguage "WrongLanguage"
     * should be returned.
     * If the \p language argument
     * For some languages different dialects exist (e.g. user- and GM- side xRSL),
     * and if the \p dialect argument is specified the
     * parsing must strictly conform to that dialect. If the dialect is unknown
     * \ref JobDescriptionParserPluginResult::WrongLanguage "WrongLanguage"
     * should be returned.
     *
     * \param source should contain a representation of job description as a
     *  string.
     * \param jobdescs a reference to a list of JobDescription object which
     *  parsed job descriptions should be appended to.
     * \param language if specified parse in specified language (if not supported
     *  \ref JobDescriptionParserPluginResult::WrongLanguage "WrongLanguage" is
     *  returned).
     * \param dialect if specified parsing conforms strictly to specified
     *  dialect.
     * \return A JobDescriptionParserPluginResult is returned indicating outcome
     *  of parsing.
     * \see JobDescriptionParserPlugin::Assemble
     * \see JobDescriptionParserPluginResult
     **/
    virtual JobDescriptionParserPluginResult Parse(const std::string& source, std::list<JobDescription>& jobdescs, const std::string& language = "", const std::string& dialect = "") const = 0;

    /// Assemble job description into string
    /**
     * \since Added in 5.1.0
     **/
    virtual JobDescriptionParserPluginResult Assemble(const JobDescription& job, std::string& output, const std::string& language, const std::string& dialect = "") const = 0;

    /// [DEPRECATED] Assemble job description into string
    /**
     * \deprecated Deprecated as of 5.1.0, use the
     *  JobDescriptionParserPlugin::Assemble method instead - expected to be
     *  removed in 6.0.0.
     **/
    virtual JobDescriptionParserPluginResult UnParse(const JobDescription& job, std::string& output, const std::string& language, const std::string& dialect = "") const { return Assemble(job, output, language, dialect); };

    /// Get supported job description languages
    /**
     * \return A list of job description languages supported by this parser is
     * returned.
     **/
    const std::list<std::string>& GetSupportedLanguages() const { return supportedLanguages; }

    /// Check if language is supported
    /**
     * \param language a string formatted according to the GLUE2
     *  JobDescription_t type (GFD-R-P.147), e.g. nordugrid:xrsl.
     * \return \c true is returned if specified language is supported.
     **/
    bool IsLanguageSupported(const std::string& language) const { return std::find(supportedLanguages.begin(), supportedLanguages.end(), language) != supportedLanguages.end(); }

    /// [DEPRECATED] Get parsing error
    /**
     * \deprecated Deprecated as of 5.1.0 - expected to be removed in 6.0.0.
     **/
    const std::string& GetError(void) { return error; };

  protected:
    JobDescriptionParserPlugin(PluginArgument* parg);

    /// [DEPRECATED] Get reference to sourceLanguage member
    /**
     * \deprecated Deprecated as of 5.1.0 - expected to be removed in 6.0.0.
     **/
    std::string& SourceLanguage(JobDescription& j) const;

    /// List of supported job description languages
    /**
     * Inheriating classes should add languages supported to this list in
     * the constructor.
     **/
    std::list<std::string> supportedLanguages;

    /// [DEPRECATED] Parsing error
    /**
     * \deprecated Deprecated as of 5.1.0 - expected to be removed in 6.0.0.
     **/
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
