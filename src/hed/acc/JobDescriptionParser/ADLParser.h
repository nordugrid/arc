// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_ADLPARSER_H__
#define __ARC_ADLPARSER_H__

#include <string>

#include <arc/client/JobDescriptionParser.h>

/** ARDLParser
 * The ARCJSDLParser class, derived from the JobDescriptionParser class, is
 * a job description parser for the EMI ES job description language (ADL)
 * described in <http://>.
 */

namespace Arc {

  template<typename T> class Range;
  class Software;
  class SoftwareRequirement;

  class ADLParser
    : public JobDescriptionParser {
  public:
    ADLParser();
    ~ADLParser();
    bool Parse(const std::string& source, std::list<JobDescription>& jobdescs, const std::string& language = "", const std::string& dialect = "") const;
    bool UnParse(const JobDescription& job, std::string& product, const std::string& language, const std::string& dialect = "") const;

    static Plugin* Instance(PluginArgument *arg);

  private:
    /*
    bool parseSoftware(XMLNode xmlSoftware, SoftwareRequirement& sr) const;
    void outputSoftware(const SoftwareRequirement& sr, XMLNode& xmlSoftware) const;

    template<typename T>
    void parseRange(XMLNode xmlRange, Range<T>& range, const T& undefValue) const;
    template<typename T>
    Range<T> parseRange(XMLNode xmlRange, const T& undefValue) const;
    template<typename T>
    void outputARCJSDLRange(const Range<T>& range, XMLNode& arcJSDL, const T& undefValue) const;
    template<typename T>
    void outputJSDLRange(const Range<T>& range, XMLNode& jsdl, const T& undefValue) const;


    void parseBenchmark(XMLNode xmlBenchmark, std::pair<std::string, double>& benchmark) const;
    void outputBenchmark(const std::pair<std::string, double>& benchmark, XMLNode& xmlBenchmark) const;
    */
  };

} // namespace Arc

#endif // __ARC_ADLPARSER_H__
