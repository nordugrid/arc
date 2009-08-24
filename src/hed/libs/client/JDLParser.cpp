// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm.h>

#include <arc/StringConv.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>
#include <arc/Logger.h>

#include "JDLParser.h"

#define ADDJDLSTRING(X, Y) (!(X).empty() ? "  " Y " = \"" + X + "\";\n" : "")
#define ADDJDLNUMBER(X, Y) ((X) > -1 ? "  " Y " = \"" + tostring(X) + "\";\n" : "")

namespace Arc {

  JDLParser::JDLParser()
    : JobDescriptionParser() {}

  JDLParser::~JDLParser() {}

  bool JDLParser::splitJDL(const std::string& original_string,
                           std::list<std::string>& lines) const {
    // Clear the return variable
    lines.clear();

    std::string jdl_text = original_string;

    bool quotation = false;
    std::list<char> stack;
    std::string actual_line;

    for (int i = 0; i < jdl_text.size() - 1; i++) {
      // Looking for control character marks the line end
      if (jdl_text[i] == ';' && !quotation && stack.empty()) {
        lines.push_back(actual_line);
        actual_line.clear();
        continue;
      }
      else if (jdl_text[i] == ';' && !quotation && stack.back() == '{') {
        logger.msg(ERROR, "[JDLParser] Semicolon (;) is not allowed inside brackets, at '%s;'.", actual_line);
        return false;
      }
      // Determinize the quotations
      if (jdl_text[i] == '"') {
        if (!quotation)
          quotation = true;
        else if (jdl_text[i - 1] != '\\')
          quotation = false;
      }
      if (!quotation) {
        if (jdl_text[i] == '{' || jdl_text[i] == '[')
          stack.push_back(jdl_text[i]);
        if (jdl_text[i] == '}') {
          if (stack.back() == '{')
            stack.pop_back();
          else
            return false;
        }
        if (jdl_text[i] == ']') {
          if (stack.back() == '[')
            stack.pop_back();
          else
            return false;
        }
      }
      actual_line += jdl_text[i];
    }
    return true;
  }

  std::string JDLParser::simpleJDLvalue(const std::string& attributeValue) const {
    std::string whitespaces(" \t\f\v\n\r");
    unsigned long last_pos = attributeValue.find_last_of("\"");
    // If the text is not between quotation marks, then return with the original form
    if (attributeValue.substr(attributeValue.find_first_not_of(whitespaces), 1) != "\"" || last_pos == std::string::npos)
      return trim(attributeValue);
    // Else remove the marks and return with the quotation's content
    else
      return attributeValue.substr(attributeValue.find_first_of("\"") + 1, last_pos - attributeValue.find_first_of("\"") - 1);
  }

  std::list<std::string> JDLParser::listJDLvalue(const std::string& attributeValue, std::pair<char, char> brackets, char lineEnd) const {
    std::list<std::string> elements;
    unsigned long first_bracket = attributeValue.find_first_of(brackets.first);
    if (first_bracket == std::string::npos) {
      elements.push_back(simpleJDLvalue(attributeValue));
      return elements;
    }
    unsigned long last_bracket = attributeValue.find_last_of(brackets.second);
    if (last_bracket == std::string::npos) {
      elements.push_back(simpleJDLvalue(attributeValue));
      return elements;
    }
    std::list<std::string> listElements;
    tokenize(attributeValue.substr(first_bracket + 1,
                                   last_bracket - first_bracket - 1),
             listElements, &lineEnd);
    for (std::list<std::string>::const_iterator it = listElements.begin();
         it != listElements.end(); it++)
      elements.push_back(simpleJDLvalue(*it));
    return elements;
  }

  bool JDLParser::handleJDLattribute(const std::string& attributeName_,
                                     const std::string& attributeValue,
                                     JobDescription& job) const {
    // To do the attributes name case-insensitive do them lowercase and remove the quotiation marks
    std::string attributeName = lower(attributeName_);
    if (attributeName == "type") {
      std::string value = lower(simpleJDLvalue(attributeValue));
      if (value == "job")
        return true;
      if (value == "dag") {
        logger.msg(DEBUG, "[JDLParser] This kind of JDL decriptor is not supported yet: %s", value);
        return false;  // This kind of JDL decriptor is not supported yet
      }
      if (value == "collection") {
        logger.msg(DEBUG, "[JDLParser] This kind of JDL decriptor is not supported yet: %s", value);
        return false;  // This kind of JDL decriptor is not supported yet
      }
      logger.msg(DEBUG, "[JDLParser] Attribute name: %s, has unknown value: %s", attributeName, value);
      return false;    // Unknown attribute value - error
    }
    else if (attributeName == "jobtype")
      return true;     // Skip this attribute
    else if (attributeName == "executable") {
      job.Application.Executable.Name = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "arguments") {
      tokenize(simpleJDLvalue(attributeValue), job.Application.Executable.Argument);
      return true;
    }
    else if (attributeName == "stdinput") {
      job.Application.Input = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "stdoutput") {
      job.Application.Output = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "stderror") {
      job.Application.Error = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "inputsandbox") {
      std::list<std::string> inputfiles = listJDLvalue(attributeValue);
      for (std::list<std::string>::const_iterator it = inputfiles.begin();
           it != inputfiles.end(); it++) {
        FileType file;
        const std::size_t pos = it->find_last_of('/');
        file.Name = (pos == std::string::npos ? *it : it->substr(pos+1));
        DataSourceType source;
        source.URI = *it;
        source.Threads = -1;
        file.Source.push_back(source);
        // Initializing these variables
        file.KeepData = false;
        file.IsExecutable = false;
        file.DownloadToCache = false;
        job.DataStaging.File.push_back(file);
      }
      return true;
    }
    else if (attributeName == "inputsandboxbaseuri") {
      for (std::list<FileType>::iterator it = job.DataStaging.File.begin();
           it != job.DataStaging.File.end(); it++)
        /* Since JDL does not have support for multiple locations the size of
         * the Source member is exactly 1.
         */
        if (!it->Source.empty() && !it->Source.front().URI)
          it->Source.front().URI = simpleJDLvalue(attributeValue);
      for (std::list<DirectoryType>::iterator it = job.DataStaging.Directory.begin();
           it != job.DataStaging.Directory.end(); it++)
        if (!it->Source.empty() && !it->Source.front().URI)
          it->Source.front().URI = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "outputsandbox") {
      std::list<std::string> outputfiles = listJDLvalue(attributeValue);
      for (std::list<std::string>::const_iterator it = outputfiles.begin();
           it != outputfiles.end(); it++) {
        FileType file;
        file.Name = *it;
        DataTargetType target;
        target.URI = *it;
        target.Threads = -1;
        target.Mandatory = false;
        target.NeededReplica = -1;
        file.Target.push_back(target);
        // Initializing these variables
        file.KeepData = false;
        file.IsExecutable = false;
        file.DownloadToCache = false;
        job.DataStaging.File.push_back(file);
      }
      return true;
    }
    else if (attributeName == "outputsandboxdesturi") {
      std::list<std::string> value = listJDLvalue(attributeValue);
      std::list<std::string>::iterator i = value.begin();
      for (std::list<FileType>::iterator it = job.DataStaging.File.begin();
           it != job.DataStaging.File.end(); it++) {
        if (it->Target.empty())
          continue;
        if (i != value.end()) {
          it->Target.front().URI = *i;
          it->KeepData = (it->Target.front().URI.Protocol() == "file");
          i++;
        }
        else {
          logger.msg(DEBUG, "Not enough outputsandboxdesturi element!");
          return false;
        }
      }
      return true;
    }
    else if (attributeName == "outputsandboxbaseuri") {
      for (std::list<FileType>::iterator it = job.DataStaging.File.begin();
           it != job.DataStaging.File.end(); it++)
        if (!it->Target.empty() && !it->Target.front().URI)
          it->Target.front().URI = simpleJDLvalue(attributeValue);
      for (std::list<DirectoryType>::iterator it = job.DataStaging.Directory.begin();
           it != job.DataStaging.Directory.end(); it++)
        if (!it->Target.empty() && !it->Target.front().URI)
          it->Target.front().URI = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "prologue") {
      job.Application.Prologue.Name = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "prologuearguments") {
      tokenize(simpleJDLvalue(attributeValue), job.Application.Prologue.Argument);
      return true;
    }
    else if (attributeName == "epilogue") {
      job.Application.Epilogue.Name = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "epiloguearguments") {
      tokenize(simpleJDLvalue(attributeValue), job.Application.Epilogue.Argument);
      return true;
    }
    else if (attributeName == "allowzippedisb") {
      // Not supported yet, only store it
      job.JDL_elements["AllowZippedISB"] = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "zippedisb") {
      // Not supported yet, only store it
      job.JDL_elements["ZippedISB"] = "\"" + simpleJDLvalue(attributeValue) + "\"";
      return true;
    }
    else if (attributeName == "expirytime") {
      job.Application.ExpiryTime = Time(stringtol(simpleJDLvalue(attributeValue)));
      return true;
    }
    else if (attributeName == "environment") {
      std::list<std::string> variables = listJDLvalue(attributeValue);
      for (std::list<std::string>::const_iterator it = variables.begin();
           it != variables.end(); it++) {
        std::string::size_type equal_pos = it->find('=');
        if (equal_pos != std::string::npos) {
          job.Application.Environment.push_back(
            std::pair<std::string, std::string>(
              trim(it->substr(0, equal_pos)),
              trim(it->substr(equal_pos + 1))));
        }
        else {
          logger.msg(DEBUG, "[JDLParser] Environment variable has been defined without any equal sign.");
          return false;
        }
      }
      return true;
    }
    else if (attributeName == "perusalfileenable") {
      // Not supported yet, only store it
      job.JDL_elements["PerusalFileEnable"] = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "perusaltimeinterval") {
      // Not supported yet, only store it
      job.JDL_elements["PerusalTimeInterval"] = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "perusalfilesdesturi") {
      // Not supported yet, only store it
      job.JDL_elements["PerusalFilesDestURI"] = "\"" + simpleJDLvalue(attributeValue) + "\"";
      return true;
    }
    else if (attributeName == "inputdata")
      // Not supported yet
      // will be soon deprecated
      return true;
    else if (attributeName == "outputdata")
      // Not supported yet
      // will be soon deprecated
      return true;
    else if (attributeName == "storageindex")
      // Not supported yet
      // will be soon deprecated
      return true;
    else if (attributeName == "datacatalog")
      // Not supported yet
      // will be soon deprecated
      return true;
    else if (attributeName == "datarequirements") {
      // Not supported yet, only store it
      job.JDL_elements["DataRequirements"] = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "dataaccessprotocol") {
      // Not supported yet, only store it
      job.JDL_elements["DataAccessProtocol"] = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "virtualorganisation") {
      job.Identification.JobVOName = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "queuename") {
        if (job.Resources.CandidateTarget.empty()) {
          ResourceTargetType candidateTarget;
          candidateTarget.EndPointURL = URL();
          candidateTarget.QueueName = simpleJDLvalue(attributeValue);
          job.Resources.CandidateTarget.push_back(candidateTarget);
        }
        else
          job.Resources.CandidateTarget.front().EndPointURL = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "batchsystem") {
      job.JDL_elements["batchsystem"] = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "retrycount") {
      const int count = stringtoi(simpleJDLvalue(attributeValue));
      if (job.Application.Rerun < count)
        job.Application.Rerun = count;
      return true;
    }
    else if (attributeName == "shallowretrycount") {
      const int count = stringtoi(simpleJDLvalue(attributeValue));
      if (job.Application.Rerun < count)
        job.Application.Rerun = count;
      return true;
    }
    else if (attributeName == "lbaddress") {
      // Not supported yet, only store it
      job.JDL_elements["LBAddress"] = "\"" + simpleJDLvalue(attributeValue) + "\"";
      return true;
    }
    else if (attributeName == "myproxyserver") {
      job.Application.CredentialService.push_back(URL(simpleJDLvalue(attributeValue)));
      return true;
    }
    else if (attributeName == "hlrlocation") {
      // Not supported yet, only store it
      job.JDL_elements["HLRLocation"] = "\"" + simpleJDLvalue(attributeValue) + "\"";
      return true;
    }
    else if (attributeName == "jobprovenance") {
      // Not supported yet, only store it
      job.JDL_elements["JobProvenance"] = "\"" + simpleJDLvalue(attributeValue) + "\"";
      return true;
    }
    else if (attributeName == "nodenumber") {
      // Not supported yet, only store it
      job.JDL_elements["NodeNumber"] = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "jobsteps")
      // Not supported yet
      // will be soon deprecated
      return true;
    else if (attributeName == "currentstep")
      // Not supported yet
      // will be soon deprecated
      return true;
    else if (attributeName == "jobstate")
      // Not supported yet
      // will be soon deprecated
      return true;
    else if (attributeName == "listenerport") {
      // Not supported yet, only store it
      job.JDL_elements["ListenerPort"] = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "listenerhost") {
      // Not supported yet, only store it
      job.JDL_elements["ListenerHost"] = "\"" + simpleJDLvalue(attributeValue) + "\"";
      return true;
    }
    else if (attributeName == "listenerpipename") {
      // Not supported yet, only store it
      job.JDL_elements["ListenerPipeName"] = "\"" + simpleJDLvalue(attributeValue) + "\"";
      return true;
    }
    else if (attributeName == "requirements") {
      // It's too complicated to determinize the right conditions, because the definition language is
      // LRMS specific.
      // Only store it.
      job.JDL_elements["Requirements"] = "\"" + simpleJDLvalue(attributeValue) + "\"";
      return true;
    }
    else if (attributeName == "rank") {
      job.JobMeta.Rank = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "fuzzyrank") {
      job.JobMeta.FuzzyRank = (upper(simpleJDLvalue(attributeValue)) == "TRUE");
      return true;
    }
    else if (attributeName == "usertags") {
      job.Identification.UserTag = listJDLvalue(attributeValue, std::make_pair('[', ']'), ';');
      return true;
    }
    else if (attributeName == "outputse") {
      // Not supported yet, only store it
      job.JDL_elements["OutputSE"] = "\"" + simpleJDLvalue(attributeValue) + "\"";
      return true;
    }
    else if (attributeName == "shortdeadlinejob") {
      // Not supported yet, only store it
      job.JDL_elements["ShortDeadlineJob"] = simpleJDLvalue(attributeValue);
      return true;
    }
    logger.msg(WARNING, "[JDL Parser]: Unknown attribute name: \'%s\', with value: %s", attributeName, attributeValue);
    return true;
  }

  std::string JDLParser::generateOutputList(const std::string& attribute, const std::list<std::string>& list, std::pair<char, char> brackets, char lineEnd) const {
    const std::string space = "             "; // 13 spaces seems to be standard padding.
    std::ostringstream output;
    output << "  " << attribute << " = " << brackets.first << std::endl;
    for (std::list<std::string>::const_iterator it = list.begin();
         it != list.end(); it++) {
      if (it != list.begin())
        output << lineEnd << std::endl;
      output << space << "\"" << *it << "\"";
    }

    output << std::endl << space << brackets.second << ";" << std::endl;
    return output.str();
  }

  JobDescription JDLParser::Parse(const std::string& source) const {
    unsigned long first = source.find_first_of("[");
    unsigned long last = source.find_last_of("]");
    if (first == std::string::npos || last == std::string::npos) {
      logger.msg(DEBUG, "[JDLParser] There is at least one necessary ruler character missing. ('[' or ']')");
      return JobDescription();
    }
    std::string input_text = source.substr(first + 1, last - first - 1);

    //Remove multiline comments
    unsigned long comment_start = 0;
    while ((comment_start = input_text.find("/*", comment_start)) != std::string::npos)
      input_text.erase(input_text.begin() + comment_start, input_text.begin() + input_text.find("*/", comment_start) + 2);

    std::string wcpy = "";
    std::list<std::string> lines;
    tokenize(input_text, lines, "\n");
    for (std::list<std::string>::iterator it = lines.begin();
         it != lines.end();) {
      // Remove empty lines
      const std::string trimmed_line = trim(*it);
      if (trimmed_line.length() == 0)
        it = lines.erase(it);
      // Remove lines starts with '#' - Comments
      else if (trimmed_line.length() >= 1 && trimmed_line.substr(0, 1) == "#")
        it = lines.erase(it);
      // Remove lines starts with '//' - Comments
      else if (trimmed_line.length() >= 2 && trimmed_line.substr(0, 2) == "//")
        it = lines.erase(it);
      else {
        wcpy += *it + "\n";
        it++;
      }
    }

    if (!splitJDL(wcpy, lines)) {
      logger.msg(DEBUG, "[JDLParser] Syntax error found during the split function.");
      return JobDescription();
    }
    if (lines.size() <= 0) {
      logger.msg(DEBUG, "[JDLParser] Lines count is zero or other funny error has occurred.");
      return JobDescription();
    }

    JobDescription job;

    for (std::list<std::string>::iterator it = lines.begin();
         it != lines.end(); it++) {
      const unsigned long equal_pos = it->find_first_of("=");
      if (equal_pos == std::string::npos) {
        if (it == --lines.end())
          continue;
        else {
          logger.msg(DEBUG, "[JDLParser] JDL syntax error. There is at least one equal sign missing where it would be expected.");
          return JobDescription();
        }
      }
      if (!handleJDLattribute(trim(it->substr(0, equal_pos)), trim(it->substr(equal_pos + 1, std::string::npos)), job))
        return JobDescription();
    }
    return job;
  }

  std::string JDLParser::UnParse(const JobDescription& job) const {
    std::string product;
    product = "[\n  Type = \"job\";\n";

    product += ADDJDLSTRING(job.Application.Executable.Name, "Executable");
    if (!job.Application.Executable.Argument.empty()) {
      product += "  Arguments = \"";
      for (std::list<std::string>::const_iterator it = job.Application.Executable.Argument.begin();
           it != job.Application.Executable.Argument.end(); it++) {
        if (it != job.Application.Executable.Argument.begin())
          product += " ";
        product += *it;
      }
      product += "\";\n";
    }

    product += ADDJDLSTRING(job.Application.Input, "StdInput");
    product += ADDJDLSTRING(job.Application.Output, "StdOutput");
    product += ADDJDLSTRING(job.Application.Error, "StdError");
    product += ADDJDLSTRING(job.Identification.JobVOName, "VirtualOrganisation");

    if (!job.Application.Environment.empty()) {
      std::list<std::string> environment;
      for (std::list< std::pair<std::string, std::string> >::const_iterator it = job.Application.Environment.begin();
           it != job.Application.Environment.end(); it++) {
        environment.push_back(it->first + " = " + it->second);
      }

      if (!environment.empty())
        product += generateOutputList("Environment", environment);
    }

    product += ADDJDLSTRING(job.Application.Prologue.Name, "Prologue");
    if (!job.Application.Prologue.Argument.empty()) {
      product += "  PrologueArguments = \"";
      for (std::list<std::string>::const_iterator iter = job.Application.Prologue.Argument.begin();
           iter != job.Application.Prologue.Argument.end(); iter++) {
        if (iter != job.Application.Prologue.Argument.begin())
          product += " ";
        product += *iter;
      }
      product += "\";\n";
    }

    product += ADDJDLSTRING(job.Application.Epilogue.Name, "Epilogue");
    if (!job.Application.Epilogue.Argument.empty()) {
      product += "  EpilogueArguments = \"";
      for (std::list<std::string>::const_iterator iter = job.Application.Epilogue.Argument.begin();
           iter != job.Application.Epilogue.Argument.end(); iter++) {
        if (iter != job.Application.Epilogue.Argument.begin())
          product += " ";
        product += *iter;
      }
      product += "\";\n";
    }

    if (!job.Application.Executable.Name.empty() ||
        !job.DataStaging.File.empty() ||
        !job.Application.Input.empty() ||
        !job.Application.Output.empty() ||
        !job.Application.Error.empty()) {

      bool addExecutable = !job.Application.Executable.Name.empty() && !Glib::path_is_absolute(job.Application.Executable.Name);
      bool addInput      = !job.Application.Input.empty();
      bool addOutput     = !job.Application.Output.empty();
      bool addError      = !job.Application.Error.empty();

      std::list<std::string> inputSandboxList;
      std::list<std::string> outputSandboxList;
      std::list<std::string> outputSandboxDestURIList;
      for (std::list<FileType>::const_iterator it = job.DataStaging.File.begin();
           it != job.DataStaging.File.end(); it++) {
        /* Since JDL does not have support for multiple locations only the first
         * location will be added.
         */
        if (!it->Source.empty())
          inputSandboxList.push_back(it->Source.front().URI ? it->Source.front().URI.fullstr() : it->Name);
        if (!it->Target.empty() && it->Target.front().URI) {
          outputSandboxList.push_back(it->Name);
          const std::string uri_tmp = (it->Target.front().URI.Host() == "localhost" ?
                                       it->Target.front().URI.Protocol() + "://" + it->Target.front().URI.Host() + it->Target.front().URI.Path() :
                                       it->Target.front().URI.fullstr());
          outputSandboxDestURIList.push_back(uri_tmp);
        }
        else if (it->KeepData) {
          outputSandboxList.push_back(it->Name);
          outputSandboxDestURIList.push_back(it->Name);
        }

        addExecutable &= (it->Name != job.Application.Executable.Name);
        addInput      &= (it->Name != job.Application.Input);
        addOutput     &= (it->Name != job.Application.Output);
        addError      &= (it->Name != job.Application.Error);
      }

      if (addExecutable)
        inputSandboxList.push_back(job.Application.Executable.Name);
      if (addInput)
        inputSandboxList.push_back(job.Application.Input);
      if (addOutput) {
        outputSandboxList.push_back(job.Application.Output);
        outputSandboxDestURIList.push_back(job.Application.Output);
      }
      if (addError) {
        outputSandboxList.push_back(job.Application.Error);
        outputSandboxDestURIList.push_back(job.Application.Error);
      }

      if (!inputSandboxList.empty())
        product += generateOutputList("InputSandbox", inputSandboxList);
      if (!outputSandboxList.empty())
        product += generateOutputList("OutputSandbox", outputSandboxList);
      if (!outputSandboxDestURIList.empty())
        product += generateOutputList("OutputSandboxDestURI", outputSandboxDestURIList);
    }

    if (!job.Resources.CandidateTarget.empty() &&
        !job.Resources.CandidateTarget.front().QueueName.empty()) {
      product += "  QueueName = \"";
      product += job.Resources.CandidateTarget.front().QueueName;
      product += "\";\n";
    }

    product += ADDJDLNUMBER(job.Application.Rerun, "RetryCount");
    product += ADDJDLNUMBER(job.Application.Rerun, "ShallowRetryCount");
    product += ADDJDLNUMBER(job.Application.ExpiryTime.GetTime(), "ExpiryTime");

    if (!job.Application.CredentialService.empty() &&
        job.Application.CredentialService.front()) {
      product += "  MyProxyServer = \"";
      product += job.Application.CredentialService.front().fullstr();
      product += "\";\n";
    }

    product += ADDJDLSTRING(job.JobMeta.Rank, "Rank");

    if (job.JobMeta.FuzzyRank)
      product += "  FuzzyRank = true;\n";

    if (!job.Identification.UserTag.empty())
      product += generateOutputList("UserTags", job.Identification.UserTag, std::pair<char, char>('[', ']'), ';');

    if (!job.JDL_elements.empty()) {
      std::map<std::string, std::string>::const_iterator it;
      for (it = job.JDL_elements.begin(); it != job.JDL_elements.end(); it++) {
        product += "  ";
        product += it->first;
        product += " = ";
        product += it->second;
        product += ";\n";
      }
    }
    product += "]";

    return product;
  }

} // namespace Arc
