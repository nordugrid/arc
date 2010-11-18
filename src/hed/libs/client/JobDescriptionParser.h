// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBDESCRIPTIONPARSER_H__
#define __ARC_JOBDESCRIPTIONPARSER_H__

#include <map>
#include <string>

#include <arc/loader/Loader.h>
#include <arc/loader/Plugin.h>

/** JobDescriptionParser
 * The JobDescriptionParser class is abstract which provide a interface for job
 * description parsers. A job description parser should inherit this class and
 * overwrite the JobDescriptionParser::Parse and
 * JobDescriptionParser::UnParse methods.
 */

namespace Arc {

  class JobDescription;
  class Logger;

  // Abstract class for the different parsers
  class JobDescriptionParser
    : public Plugin {
  public:
    virtual ~JobDescriptionParser();
    virtual JobDescription Parse(const std::string& source) const = 0;
    virtual std::string UnParse(const JobDescription& job) const = 0;
    void AddHint(const std::string& key,const std::string& value);
    void SetHints(const std::map<std::string,std::string>& hints);
    const std::string& GetSourceFormat() const { return format; }

  protected:
    JobDescriptionParser(const std::string& format);

    std::string format;
    static Logger logger;
    std::map<std::string,std::string> hints;
    std::string GetHint(const std::string& key) const;
  };

  //! Class responsible for loading JobDescriptionParser plugins
  /// The JobDescriptionParser objects returned by a JobDescriptionParserLoader
  /// must not be used after the JobDescriptionParserLoader goes out of scope.
  class JobDescriptionParserLoader
    : public Loader {

  public:
    //! Constructor
    /// Creates a new JobDescriptionParserLoader.
    JobDescriptionParserLoader();

    //! Destructor
    /// Calling the destructor destroys all JobDescriptionParser object loaded
    /// by the JobDescriptionParserLoader instance.
    ~JobDescriptionParserLoader();

    //! Load a new JobDescriptionParser
    /// \param name    The name of the JobDescriptionParser to load.
    /// \return        A pointer to the new JobDescriptionParser (NULL on error).
    JobDescriptionParser* load(const std::string& name);

    //! Retrieve the list of loaded JobDescriptionParser objects.
    /// \return A reference to the list of JobDescriptionParser objects.
    const std::list<JobDescriptionParser*>& GetJobDescriptionParsers() const {
      return jdps;
    }

    class iterator {
    private:
      iterator(JobDescriptionParserLoader& jdpl);
    public:
      ~iterator() {}
      iterator& operator=(const iterator& it) { current = it.current; jdpl = it.jdpl; return *this; }
      JobDescriptionParser& operator*() { return **current; }
      const JobDescriptionParser& operator*() const { return **current; }
      JobDescriptionParser* operator->() { return *current; }
      const JobDescriptionParser* operator->() const { return *current; }
      iterator& operator++();
      operator bool() { return !jdpl.jdpDescs.empty() || current != jdpl.jdps.end() || *current != NULL; }

      friend class JobDescriptionParserLoader;
    private:
      void LoadNext();

      std::list<JobDescriptionParser*>::iterator current;
      JobDescriptionParserLoader& jdpl;
    };

    iterator GetIterator() { return iterator(*this); }

  private:
    std::list<JobDescriptionParser*> jdps;
    std::list<ModuleDesc> jdpDescs;

    void scan();
    bool scaningDone;
  };
} // namespace Arc

#endif // __ARC_JOBDESCRIPTIONPARSER_H__
