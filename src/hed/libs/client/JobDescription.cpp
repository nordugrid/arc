// -*- indent-tabs-mode: nil -*-

//
// TODO: Indentation fix
//
#include <cstring>
#include <algorithm>
#include <sys/stat.h>
#include <fcntl.h>
#include <exception>

#include <arc/StringConv.h>
#include <arc/job/runtimeenvironment.h>
#include <arc/data/CheckSum.h>

#include "JobDescription.h"

#include "XRSLParser.h"
#include "JSDLParser.h"
#include "PosixJSDLParser.h"
#include "JDLParser.h"

namespace Arc {

  Arc::Logger JobDescription::logger(Arc::Logger::getRootLogger(), "jobdescription");

  // JobDescription default constructor
  JobDescription::JobDescription() {
    sourceFormat = "";
    sourceString = "";
    innerRepresentation = NULL;
  }

  JobDescription::JobDescription(const JobDescription& desc) {
    sourceString = desc.sourceString;
    sourceFormat = desc.sourceFormat;
    if (desc.innerRepresentation != NULL)
      innerRepresentation = new Arc::JobInnerRepresentation(*desc.innerRepresentation);
    else
      innerRepresentation = NULL;
  }

  JobDescription::~JobDescription() {
    delete innerRepresentation;
  }

  // Set the sourceString variable of the JobDescription instance and parse it with the right parser
  bool JobDescription::setSource(const XMLNode& xmlSource) {
    std::string strSource;
    xmlSource.GetXML(strSource);
    return setSource(strSource);
  }

  bool JobDescription::setSource(const std::string source) {
    sourceFormat = "";
    sourceString = source;

    // Initialize the necessary variables
    resetJobTree();

    // Parsing the source string depending on the Orderer's decision
    //
    // Get the candidate list of formats in the proper order
    if (sourceString.empty() || sourceString.length() == 0) {
      JobDescription::logger.msg(DEBUG, "There is nothing in the source. Cannot generate any product.");
      return false;
    }

    JobDescriptionOrderer jdOrderer;
    jdOrderer.setSource(sourceString);
    std::vector<Candidate> candidates = jdOrderer.getCandidateList();

    // Try to parse the input string in the right order until it once success
    // (If not then just reset the jobTree variable and see the next one)
    for (std::vector<Candidate>::const_iterator it = candidates.begin(); it < candidates.end(); it++) {
      if (lower((*it).typeName) == "xrsl") {
        JobDescription::logger.msg(DEBUG, "[JobDescription] Try to parse as XRSL");
        XRSLParser parser;
        std::string parse_sourceString = trim(sourceString);
        if (parser.parse(*innerRepresentation, parse_sourceString)) {
          sourceFormat = "xrsl";
          break;
        }
        //else
        resetJobTree();
      }
      else if (lower((*it).typeName) == "posixjsdl") {
        JobDescription::logger.msg(DEBUG, "[JobDescription] Try to parse as POSIX JSDL");
        PosixJSDLParser parser;
        if (parser.parse(*innerRepresentation, sourceString)) {
          sourceFormat = "posixjsdl";
          break;
        }
        //else
        resetJobTree();
      }
      else if (lower((*it).typeName) == "jsdl") {
        JobDescription::logger.msg(DEBUG, "[JobDescription] Try to parse as JSDL");
        JSDLParser parser;
        if (parser.parse(*innerRepresentation, sourceString)) {
          sourceFormat = "jsdl";
          break;
        }
        //else
        resetJobTree();
      }
      else if (lower((*it).typeName) == "jdl") {
        JobDescription::logger.msg(DEBUG, "[JobDescription] Try to parse as JDL");
        JDLParser parser;
        if (parser.parse(*innerRepresentation, sourceString)) {
          sourceFormat = "jdl";
          break;
        }
        //else
        resetJobTree();
      }
    }
    if (sourceFormat.length() == 0) {
      JobDescription::logger.msg(DEBUG, "The parsing of the source string was unsuccessful.");
      return false;
    }
    return true;
  }

  // Create a new Arc::XMLNode with a single root element and out it into the JobDescription's jobTree variable
  void JobDescription::resetJobTree() {
    if (innerRepresentation != NULL)
      delete innerRepresentation;
    innerRepresentation = new JobInnerRepresentation();
  }

  // Returns with "true" and the jobTree XMLNode, or "false" if the innerRepresentation is emtpy.
  bool JobDescription::getXML(Arc::XMLNode& jobTree) const {
    if (innerRepresentation == NULL)
      return false;
    if (!(*innerRepresentation).getXML(jobTree)) {
      JobDescription::logger.msg(DEBUG, "Error during the XML generation!");
      return false;
    }
    return true;
  }

  // Generate the output in the requested format
  bool JobDescription::getProduct(std::string& product, std::string format) const {
    // Initialize the necessary variables
    product = "";

    // Generate the output text with the right parser class
    if (innerRepresentation == NULL || !this->isValid()) {
      JobDescription::logger.msg(DEBUG, "There is no successfully parsed source");
      return false;
    }

    if (lower(format) == "jdl") {
      JobDescription::logger.msg(DEBUG, "[JobDescription] Generate JDL output");
      JDLParser parser;
      if (!parser.getProduct(*innerRepresentation, product)) {
        JobDescription::logger.msg(DEBUG, "Generating %s output was unsuccessful", format);
        return false;
      }
      return true;
    }
    else if (lower(format) == "xrsl") {
      JobDescription::logger.msg(DEBUG, "[JobDescription] Generate XRSL output");
      XRSLParser parser;
      if (!parser.getProduct(*innerRepresentation, product)) {
        JobDescription::logger.msg(DEBUG, "Generating %s output was unsuccessful", format);
        return false;
      }
      return true;
    }
    else if (lower(format) == "posixjsdl") {
      JobDescription::logger.msg(DEBUG, "[JobDescription] Generate POSIX JSDL output");
      PosixJSDLParser parser;
      if (!parser.getProduct(*innerRepresentation, product)) {
        JobDescription::logger.msg(DEBUG, "Generating %s output was unsuccessful", format);
        return false;
      }
      return true;
    }
    else if (lower(format) == "jsdl") {
      JobDescription::logger.msg(DEBUG, "[JobDescription] Generate JSDL output");
      JSDLParser parser;
      if (!parser.getProduct(*innerRepresentation, product)) {
        JobDescription::logger.msg(DEBUG, "Generating %s output was unsuccessful", format);
        return false;
      }
      return true;
    }
    else {
      JobDescription::logger.msg(DEBUG, "Unknown output format: %s", format);
      return false;
    }
  }

  bool JobDescription::getSourceFormat(std::string& _sourceFormat) const {
    if (!isValid()) {
      JobDescription::logger.msg(DEBUG, "There is no input defined yet or it's format can be determinized.");
      return false;
    }
    else {
      _sourceFormat = sourceFormat;
      return true;
    }
  }

  bool JobDescription::getUploadableFiles(std::vector<std::pair<std::string, std::string> >& sourceFiles) const {
    if (!isValid()) {
      JobDescription::logger.msg(DEBUG, "There is no input defined yet or it's format can be determinized.");
      return false;
    }
    // Get the URI's from the DataStaging/File/Source files
    Arc::XMLNode xml_repr;
    if (!getXML(xml_repr))
      return false;
    Arc::XMLNode sourceNode = xml_repr["JobDescription"]["DataStaging"]["File"];
    while ((bool)sourceNode) {
      if ((bool)sourceNode["Source"]) {
        std::string uri((std::string)sourceNode["Source"]["URI"]);
        std::string filename((std::string)sourceNode["Name"]);
        Arc::URL sourceUri(uri);
        if (sourceUri.Protocol() == "file") {
          std::pair<std::string, std::string> item(filename, uri);
          sourceFiles.push_back(item);
        }
      }
      ++sourceNode;
    }
    return true;
  }

  bool JobDescription::getInnerRepresentation(Arc::JobInnerRepresentation& job) const {
    if (innerRepresentation == NULL)
      return false;
    job = *innerRepresentation;
    return true;
  }

  bool JobDescription::addOldJobID(const URL& oldjobid) {
    if (innerRepresentation == NULL)
      return false;
    innerRepresentation->OldJobIDs.push_back(oldjobid);
    return true;
  }

  bool JobDescription::getSourceString(std::string& string) const {
    string = sourceString;
  }
} // namespace Arc
