// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>
#include <string>
#include <vector>

#include <arc/StringConv.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>
#include <arc/Logger.h>

#include "JDLParser.h"

namespace Arc {

  JDLParser::JDLParser()
    : JobDescriptionParser() {}

  JDLParser::~JDLParser() {}

  bool JDLParser::splitJDL(const std::string& original_string,
                           std::vector<std::string>& lines) const {

    // Clear the return variable
    lines.clear();

    std::string jdl_text = original_string;

    bool quotation = false;
    std::vector<char> stack;
    std::string actual_line;

    for (int i = 0; i < jdl_text.size() - 1; i++) {
      // Looking for control character marks the line end
      if (jdl_text[i] == ';' && !quotation && stack.empty()) {
        lines.push_back(actual_line);
        actual_line.clear();
        continue;
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

  std::list<std::string> JDLParser::listJDLvalue(const std::string& attributeValue) const {
    std::list<std::string> elements;
    unsigned long first_bracket = attributeValue.find_first_of("{");
    if (first_bracket == std::string::npos) {
      elements.push_back(simpleJDLvalue(attributeValue));
      return elements;
    }
    unsigned long last_bracket = attributeValue.find_last_of("}");
    if (last_bracket == std::string::npos) {
      elements.push_back(simpleJDLvalue(attributeValue));
      return elements;
    }
    std::vector<std::string> listElements;
    tokenize(attributeValue.substr(first_bracket + 1,
                                   last_bracket - first_bracket - 1),
             listElements, ",");
    for (std::vector<std::string>::const_iterator it = listElements.begin();
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
      job.Executable = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "arguments") {
      std::string value = simpleJDLvalue(attributeValue);
      std::vector<std::string> parts;
      tokenize(value, parts);
      for (std::vector<std::string>::const_iterator it = parts.begin();
           it != parts.end(); it++)
        job.Argument.push_back((*it));
      return true;
    }
    else if (attributeName == "stdinput") {
      job.Input = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "stdoutput") {
      job.Output = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "stderror") {
      job.Error = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "inputsandbox") {
      std::list<std::string> inputfiles = listJDLvalue(attributeValue);
      for (std::list<std::string>::const_iterator it = inputfiles.begin();
           it != inputfiles.end(); it++) {
        FileType file;
        std::vector<std::string> parts;
        tokenize(*it, parts, "/");
        file.Name = parts.back();
        SourceType source;
        source.URI = *it;
        source.Threads = -1;
        file.Source.push_back(source);
        //initializing this variables
        file.KeepData = false;
        file.IsExecutable = false;
        file.DownloadToCache = false;
        job.File.push_back(file);
      }
      return true;
    }
    else if (attributeName == "inputsandboxbaseuri") {
      for (std::list<FileType>::iterator it = job.File.begin();
           it != job.File.end(); it++)
        for (std::list<SourceType>::iterator sit = it->Source.begin();
             sit != it->Source.end(); sit++)
          if (!sit->URI)
            sit->URI = simpleJDLvalue(attributeValue);
      for (std::list<DirectoryType>::iterator it = job.Directory.begin();
           it != job.Directory.end(); it++)
        for (std::list<SourceType>::iterator sit = it->Source.begin();
             sit != it->Source.end(); sit++)
          if (!sit->URI)
            sit->URI = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "outputsandbox") {
      std::list<std::string> outputfiles = listJDLvalue(attributeValue);
      for (std::list<std::string>::const_iterator it = outputfiles.begin();
           it != outputfiles.end(); it++) {
        FileType file;
        file.Name = *it;
        TargetType target;
        target.URI = *it;
        target.Threads = -1;
        target.Mandatory = false;
        target.NeededReplicas = -1;
        file.Target.push_back(target);
        //initializing this variables
        file.KeepData = false;
        file.IsExecutable = false;
        file.DownloadToCache = false;
        job.File.push_back(file);
      }
      return true;
    }
    else if (attributeName == "outputsandboxdesturi") {
      std::list<std::string> value = listJDLvalue(attributeValue);
      std::list<std::string>::iterator i = value.begin();
      for (std::list<FileType>::iterator it = job.File.begin();
           it != job.File.end(); it++)
        if (!it->Target.empty()) {
          if (i != value.end()) {
            it->Target.begin()->URI = *i;
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
      for (std::list<FileType>::iterator it = job.File.begin();
           it != job.File.end(); it++)
        for (std::list<TargetType>::iterator tit = it->Target.begin();
             tit != it->Target.end(); tit++)
          if (!tit->URI)
            tit->URI = simpleJDLvalue(attributeValue);
      for (std::list<DirectoryType>::iterator it = job.Directory.begin();
           it != job.Directory.end(); it++)
        for (std::list<TargetType>::iterator tit = it->Target.begin();
             tit != it->Target.end(); tit++)
          if (!tit->URI)
            tit->URI = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "batchsystem") {
      // Not supported yet, only store it
      job.JDL_elements["BatchSystem"] = "\"" + simpleJDLvalue(attributeValue) + "\"";
      return true;
    }
    else if (attributeName == "prologue") {
      job.Prologue.Name = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "prologuearguments") {
      std::string value = simpleJDLvalue(attributeValue);
      std::vector<std::string> parts;
      tokenize(value, parts);
      for (std::vector<std::string>::const_iterator it = parts.begin();
           it != parts.end(); it++)
        job.Prologue.Arguments.push_back(*it);
      return true;
    }
    else if (attributeName == "epilogue") {
      job.Epilogue.Name = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "epiloguearguments") {
      std::string value = simpleJDLvalue(attributeValue);
      std::vector<std::string> parts;
      tokenize(value, parts);
      for (std::vector<std::string>::const_iterator it = parts.begin();
           it != parts.end(); it++)
        job.Epilogue.Arguments.push_back(*it);
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
      Time expirytime = stringtol(simpleJDLvalue(attributeValue));
      Time now = Time();
      job.SessionLifeTime = expirytime - now;
      return true;
    }
    else if (attributeName == "environment") {
      std::list<std::string> variables = listJDLvalue(attributeValue);
      for (std::list<std::string>::const_iterator it = variables.begin();
           it != variables.end(); it++) {
        std::string::size_type equal_pos = it->find('=');
        if (equal_pos != std::string::npos) {
          EnvironmentType env;
          env.name_attribute = it->substr(0, equal_pos);
          env.value = it->substr(equal_pos + 1);
          job.Environment.push_back(env);
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
      // Not supported yet, only store it
      job.JDL_elements["VirtualOrganisation"] = "\"" + simpleJDLvalue(attributeValue) + "\"";
      return true;
    }
    else if (attributeName == "queuename") {
      job.QueueName = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "retrycount") {
      int count = stringtoi(simpleJDLvalue(attributeValue));
      if (job.LRMSReRun > count)
        count = job.LRMSReRun;
      job.LRMSReRun = count;
      return true;
    }
    else if (attributeName == "shallowretrycount") {
      int count = stringtoi(simpleJDLvalue(attributeValue));
      if (job.LRMSReRun > count)
        count = job.LRMSReRun;
      job.LRMSReRun = count;
      return true;
    }
    else if (attributeName == "lbaddress") {
      // Not supported yet, only store it
      job.JDL_elements["LBAddress"] = "\"" + simpleJDLvalue(attributeValue) + "\"";
      return true;
    }
    else if (attributeName == "myproxyserver") {
      URL url(simpleJDLvalue(attributeValue));
      job.CredentialService = url;
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
      job.Rank = simpleJDLvalue(attributeValue);
      return true;
    }
    else if (attributeName == "fuzzyrank") {
      bool fuzzyrank = false;
      if (upper(simpleJDLvalue(attributeValue)) == "TRUE")
        fuzzyrank = true;
      job.FuzzyRank = fuzzyrank;
      return true;
    }
    else if (attributeName == "usertags") {
      // They have no standard and no meaning.
      job.UserTag.push_back(simpleJDLvalue(attributeValue));
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
    logger.msg(DEBUG, "[JDL Parser]: Unknown attribute name: \'%s\', with value: %s", attributeName, attributeValue);
    return false;
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
    std::vector<std::string> lines;
    tokenize(input_text, lines, "\n");
    for (unsigned long i = 0; i < lines.size();) {
      // Remove empty lines
      std::string trimmed_line = trim(lines[i]);
      if (trimmed_line.length() == 0)
        lines.erase(lines.begin() + i);
      // Remove lines starts with '#' - Comments
      else if (trimmed_line.length() >= 1 && trimmed_line.substr(0, 1) == "#")
        lines.erase(lines.begin() + i);
      // Remove lines starts with '//' - Comments
      else if (trimmed_line.length() >= 2 && trimmed_line.substr(0, 2) == "//")
        lines.erase(lines.begin() + i);
      else
        wcpy += lines[i++] + "\n";
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

    for (unsigned long i = 0; i < lines.size(); i++) {
      unsigned long equal_pos = lines[i].find_first_of("=");
      if (equal_pos == std::string::npos) {
        if (i == lines.size() - 1)
          continue;
        else {
          logger.msg(DEBUG, "[JDLParser] JDL syntax error. There is at least one equal sign missing where it would be expected.");
          return JobDescription();
        }
      }
      if (!handleJDLattribute(trim(lines[i].substr(0, equal_pos)), trim(lines[i].substr(equal_pos + 1, std::string::npos)), job))
        return JobDescription();
    }
    return job;
  }

  std::string JDLParser::UnParse(const JobDescription& job) const {
    std::string product;
    product = "[\n  Type = \"job\";\n";
    if (!job.Executable.empty()) {
      product += "  Executable = \"";
      product += job.Executable;
      product += "\";\n";
    }
    if (!job.Argument.empty()) {
      product += "  Arguments = \"";
      bool first = true;
      for (std::list<std::string>::const_iterator it = job.Argument.begin();
           it != job.Argument.end(); it++) {
        if (!first)
          product += " ";
        else
          first = false;
        product += *it;
      }
      product += "\";\n";
    }
    if (!job.Input.empty()) {
      product += "  StdInput = \"";
      product += job.Input;
      product += "\";\n";
    }
    if (!job.Output.empty()) {
      product += "  StdOutput = \"";
      product += job.Output;
      product += "\";\n";
    }
    if (!job.Error.empty()) {
      product += "  StdError = \"";
      product += job.Error;
      product += "\";\n";
    }
    if (!job.Environment.empty()) {
      product += "  Environment = {\n";
      for (std::list<EnvironmentType>::const_iterator it = job.Environment.begin();
           it != job.Environment.end(); it++) {
        product += "    \"" + (*it).name_attribute + "=" + (*it).value + "\"";
        if (it++ != job.Environment.end()) {
          product += ",";
          it--;
        }
        product += "\n";
      }
      product += "    };\n";
    }
    if (!job.Prologue.Name.empty()) {
      product += "  Prologue = \"";
      product += job.Prologue.Name.empty();
      product += "\";\n";
    }
    if (!job.Prologue.Arguments.empty()) {
      product += "  PrologueArguments = \"";
      bool first = true;
      for (std::list<std::string>::const_iterator iter = job.Prologue.Arguments.begin();
           iter != job.Prologue.Arguments.end(); iter++) {
        if (!first)
          product += " ";
        else
          first = false;
        product += *iter;
      }
      product += "\";\n";
    }
    if (!job.Epilogue.Name.empty()) {
      product += "  Epilogue = \"";
      product += job.Epilogue.Name;
      product += "\";\n";
    }
    if (!job.Epilogue.Arguments.empty()) {
      product += "  EpilogueArguments = \"";
      bool first = true;
      for (std::list<std::string>::const_iterator iter = job.Epilogue.Arguments.begin();
           iter != job.Epilogue.Arguments.end(); iter++) {
        if (!first)
          product += " ";
        else
          first = false;
        product += *iter;
      }
      product += "\";\n";
    }
    if (!job.File.empty() ||
        !job.Executable.empty() ||
        !job.Input.empty()) {

      bool executable = false;
      bool input = false;
      bool empty = true;
      product += "  InputSandbox = {\n";
      std::list<FileType>::const_iterator iter;
      for (iter = job.File.begin(); iter != job.File.end(); iter++) {
        if ((*iter).Name == job.Executable)
          executable = true;
        if ((*iter).Name == job.Input)
          input = true;
        std::list<SourceType>::const_iterator it_source;
        for (it_source = (*iter).Source.begin(); it_source != (*iter).Source.end(); it_source++) {
          if (it_source == (*iter).Source.begin() && empty)
            product += "    \"" + (*it_source).URI.fullstr() + "\"";
          else
            product += ",\n    \"" + (*it_source).URI.fullstr() + "\"";
          empty = false;
        }
      }
      if (!job.Executable.empty() && !executable) {
        if (!empty)
          product += ",\n";
        empty = false;
        product += "    \"" + job.Executable + "\"";
      }
      if (!job.Input.empty() && !input) {
        if (!empty)
          product += ",\n";
        empty = false;
        product += "    \"" + job.Input + "\"";
      }
      product += "\n    };\n";
    }
    if (!job.File.empty() ||
        !job.Error.empty() ||
        !job.Output.empty()) {

      std::list<FileType>::const_iterator iter;
      bool first = true;
      bool error = false;
      bool output = false;
      for (iter = job.File.begin(); iter != job.File.end(); iter++) {
        if ((*iter).Name == job.Error)
          error = true;
        if ((*iter).Name == job.Output)
          output = true;
        if (!(*iter).Target.empty()) {
          if (!first)
            product += ",";
          else {
            product += "  OutputSandbox = {";
            first = false;
          }
          product += " \"" + (*iter).Name + "\"";
        }
      }
      if (!job.Error.empty() && !error) {
        if (first) {
          product += "  OutputSandbox = {";
          first = false;
        }
        else
          product += ", ";
        product += " \"" + job.Error + "\"";
      }
      if (!job.Output.empty() && !output) {
        if (first) {
          product += "  OutputSandbox = {";
          first = false;
        }
        else
          product += ", ";
        product += " \"" + job.Output + "\"";
      }
      if (!first)
        product += " };\n";
    }
    if (!job.File.empty() ||
        !job.Error.empty() ||
        !job.Output.empty()) {

      std::list<FileType>::const_iterator iter;
      bool first = true;
      bool error = false;
      bool output = false;
      for (iter = job.File.begin(); iter != job.File.end(); iter++) {
        if ((*iter).Name == job.Error)
          error = true;
        if ((*iter).Name == job.Output)
          output = true;
        std::list<TargetType>::const_iterator it_target;
        for (it_target = (*iter).Target.begin(); it_target != (*iter).Target.end(); it_target++) {
          if (first)
            product += "  OutputSandboxDestURI = {\n";
          // first = false;
          if (it_target == (*iter).Target.begin() && first) {
            first = false;
            product += "    \"" + (*it_target).URI.fullstr() + "\"";
          }
          else
            product += ",\n    \"" + (*it_target).URI.fullstr() + "\"";
          //                    product += "    \"" +  (*it_target).URI.fullstr() + "\"";
          //                  if (it_target++ != (*iter).Target.end()) {
          //                   product += ",";
          //                 it_target--;
          //            }
          //          product += "\n";
        }
      }
      if (!job.Error.empty() && !error) {
        if (first) {
          product += "  OutputSandboxDestURI = {";
          first = false;
        }
        else
          product += ", \n";
        URL errorURL("gsiftp://localhost/" + job.Error);
        product += "    \"" + errorURL.fullstr() + "\"";
      }
      if (!job.Output.empty() && !output) {
        if (first) {
          product += "  OutputSandboxDestURI = {";
          first = false;
        }
        else
          product += ", \n";
        URL outputURL("gsiftp://localhost/" + job.Output);
        product += "    \"" + outputURL.fullstr() + "\"";
      }
      if (!first)
        product += "\n    };\n";
    }
    if (!job.QueueName.empty()) {
      product += "  QueueName = \"";
      product += job.QueueName;
      product += "\";\n";
    }
    if (job.LRMSReRun > -1) {
      product += "  RetryCount = \"";
      std::ostringstream oss;
      oss << job.LRMSReRun;
      product += oss.str();
      product += "\";\n";
    }
    if (job.SessionLifeTime != -1) {
      std::string outputValue;
      int attValue = atoi(((std::string)job.SessionLifeTime).c_str());
      int nowSeconds = time(NULL);
      std::stringstream ss;
      ss << attValue + nowSeconds;
      ss >> outputValue;
      product += "  ExpiryTime = \"";
      product += outputValue;
      product += "\";\n";
    }
    if (bool(job.CredentialService)) {
      product += "  MyProxyServer = \"";
      product += job.CredentialService.fullstr();
      product += "\";\n";
    }
    if (!job.Rank.empty()) {
      product += "  Rank = \"";
      product += job.Rank;
      product += "\";\n";
    }
    if (job.FuzzyRank)
      product += "  FuzzyRank = true;\n";
    if (!job.UserTag.empty()) {
      product += "  UserTag = ";
      bool first = true;
      std::list<std::string>::const_iterator iter;
      for (iter = job.UserTag.begin(); iter != job.UserTag.end(); iter++) {
        if (!first)
          product += ",";
        else
          first = false;
        product += *iter;
      }
      product += ";\n";
    }
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
