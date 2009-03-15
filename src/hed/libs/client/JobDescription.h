// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBDESCRIPTION_H__
#define __ARC_JOBDESCRIPTION_H__

#include <string>
#include <vector>
#include <arc/XMLNode.h>
#include <arc/URL.h>
#include <arc/client/JobInnerRepresentation.h>
#include <arc/Logger.h>

namespace Arc {

  class JobDescription {
  private:
    Arc::XMLNode jobTree;
    std::string sourceString;
    std::string sourceFormat;
    Arc::JobInnerRepresentation *innerRepresentation;
    void resetJobTree();
  public:
    static Logger logger;
    JobDescription();
    JobDescription(const JobDescription& desc);
    ~JobDescription();

    // Returns with true if the source has been setted up and wasn't any syntax error in it and there
    // is a mandatory (Executable) element setted correctly or false otherwise
    bool isValid() const {
      return (sourceFormat.length() != 0) &&
             !(*innerRepresentation).Executable.empty();
    }

    // Try to parse the source string and store it in an inner job description representation.
    // If there is some syntax error then it throws an exception.
    bool setSource(const std::string source);
    bool setSource(const XMLNode& source);

    // Transform the inner job description representation into a given format, if it's known as a parser (JSDL as default)
    // If there is some error during this method, then return with false.
    bool getProduct(std::string& product, std::string format = "POSIXJSDL") const;

    // Returns with the original job descriptions format as a string. Right now, this value is one of the following:
    // "jsdl", "jdl", "xrsl". If there is an other parser written for another language, then this set can be extended.
    bool getSourceFormat(std::string& _sourceFormat) const;

    // Returns with the XML representation of the job description. This inner representation is very similar
    // to the JSDL structure and exactly the same in cases, which are defined in the JSDL specification.
    bool getXML(Arc::XMLNode& node) const;

    // Returns with true and the uploadable local files list, if the source has been setted up and is valid, else return with false.
    bool getUploadableFiles(std::vector<std::pair<std::string, std::string> >& sourceFiles) const;

    // Returns with the inner representation object.
    bool getInnerRepresentation(Arc::JobInnerRepresentation& job) const;

    // Add an URL to OldJobIDs
    bool addOldJobID(const URL& oldjobid);

    // Return sourceString
    bool getSourceString(std::string& string) const;
  };

} // namespace Arc

#endif // __ARC_JOBDESCRIPTION_H__
