// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <arc/ArcLocation.h>
#include <arc/StringConv.h>
#include <arc/FileUtils.h>
#include <arc/CheckSum.h>
#include <arc/compute/ExecutionTarget.h>
#include <arc/compute/JobDescriptionParserPlugin.h>

#include "JobDescription.h"


#define INTPRINT(OUT, X, Y) if ((X) > -1) \
  OUT << IString(#Y ": %d", X) << std::endl;
#define STRPRINT(OUT, X, Y) if (!(X).empty()) \
  OUT << IString(#Y ": %s", X) << std::endl;


namespace Arc {

  Logger JobDescription::logger(Logger::getRootLogger(), "JobDescription");

  // Maybe this mutex could go to JobDescriptionParserPluginLoader. That would make
  // it transparent. On another hand JobDescriptionParserPluginLoader must not know
  // how it is used.
  Glib::Mutex JobDescription::jdpl_lock;
  // TODO: JobDescriptionParserPluginLoader need to be freed when not used any more.
  JobDescriptionParserPluginLoader *JobDescription::jdpl = NULL;

  JobDescription::JobDescription(const long int& ptraddr) { *this = *((JobDescription*)ptraddr); }

  JobDescription::JobDescription(const JobDescription& j, bool withAlternatives) {
    if (withAlternatives) {
      *this = j;
    }
    else {
      RemoveAlternatives();
      Set(j);
    }
  }

  ApplicationType& ApplicationType::operator=(const ApplicationType& at) {
    Executable = at.Executable;
    Input = at.Input;
    Output = at.Output;
    Error = at.Error;
    Environment = at.Environment;
    PreExecutable = at.PreExecutable;
    PostExecutable = at.PostExecutable;
    LogDir = at.LogDir;
    RemoteLogging = at.RemoteLogging;
    Rerun = at.Rerun;
    ExpirationTime = at.ExpirationTime;
    ProcessingStartTime = at.ProcessingStartTime;
    Priority = at.Priority;
    Notification = at.Notification;
    CredentialService = at.CredentialService;
    at.AccessControl.New(AccessControl);
    DryRun = at.DryRun;
    return *this;
  }

  ResourcesType& ResourcesType::operator=(const ResourcesType& rt) {
    OperatingSystem = rt.OperatingSystem;
    Platform = rt.Platform;
    NetworkInfo = rt.NetworkInfo;
    IndividualPhysicalMemory = rt.IndividualPhysicalMemory;
    IndividualVirtualMemory = rt.IndividualVirtualMemory;
    DiskSpaceRequirement = rt.DiskSpaceRequirement;
    SessionLifeTime = rt.SessionLifeTime;
    SessionDirectoryAccess = rt.SessionDirectoryAccess;
    IndividualCPUTime = rt.IndividualCPUTime;
    TotalCPUTime = rt.TotalCPUTime;
    IndividualWallTime = rt.IndividualWallTime;
    NodeAccess = rt.NodeAccess;
    CEType = rt.CEType;
    SlotRequirement = rt.SlotRequirement;
    ParallelEnvironment = rt.ParallelEnvironment;
    Coprocessor = rt.Coprocessor;
    QueueName = rt.QueueName;
    RunTimeEnvironment = rt.RunTimeEnvironment;
    return *this;
  }

  void JobDescription::Set(const JobDescription& j) {
    Identification = j.Identification;
    Application = j.Application;
    Resources = j.Resources;
    DataStaging = j.DataStaging;
    OtherAttributes = j.OtherAttributes;
    X509Delegation = j.X509Delegation;
    TokenDelegation = j.TokenDelegation;
    sourceLanguage = j.sourceLanguage;
  }

  void JobDescription::RemoveAlternatives() {
    alternatives.clear();
    current = alternatives.begin();
  }

  JobDescription& JobDescription::operator=(const JobDescription& j) {
    RemoveAlternatives();
    Set(j);

    if (!j.alternatives.empty()) {
      alternatives = j.alternatives;
      current = alternatives.begin();
      for (std::list<JobDescription>::const_iterator it = j.alternatives.begin();
           it != j.current && it != j.alternatives.end(); it++) {
        current++; // Increase iterator so it points to same object as in j.
      }
    }

    return *this;
  }

  JobDescriptionResult JobDescription::SaveToStream(std::ostream& out, const std::string& format) const {

    if (format != "user" && format != "userlong") {
      std::string outjobdesc;
      if (!UnParse(outjobdesc, format)) {
        return false;
      }
      out << outjobdesc;
      return true;
    }

    STRPRINT(out, Application.Executable.Path, Executable);
    if (Application.DryRun) {
      out << istring(" --- DRY RUN --- ") << std::endl;
    }
    STRPRINT(out, Application.LogDir, Log Directory)
    STRPRINT(out, Identification.JobName, JobName)
    STRPRINT(out, Identification.Description, Description)

    if (format == "userlong") {
      if (!Identification.Annotation.empty()) {
        std::list<std::string>::const_iterator iter = Identification.Annotation.begin();
        for (; iter != Identification.Annotation.end(); iter++)
          out << IString(" Annotation: %s", *iter) << std::endl;
      }

      if (!Identification.ActivityOldID.empty()) {
        std::list<std::string>::const_iterator iter = Identification.ActivityOldID.begin();
        for (; iter != Identification.ActivityOldID.end(); iter++)
          out << IString(" Old activity ID: %s", *iter) << std::endl;
      }

      if (!Application.Executable.Argument.empty()) {
        std::list<std::string>::const_iterator iter = Application.Executable.Argument.begin();
        for (; iter != Application.Executable.Argument.end(); iter++)
          out << IString(" Argument: %s", *iter) << std::endl;
      }

      STRPRINT(out, Application.Input, Input)
      STRPRINT(out, Application.Output, Output)
      STRPRINT(out, Application.Error, Error)

      if (!Application.RemoteLogging.empty()) {
        std::list<RemoteLoggingType>::const_iterator iter = Application.RemoteLogging.begin();
        for (; iter != Application.RemoteLogging.end(); iter++) {
          if (iter->optional) {
            out << IString(" RemoteLogging (optional): %s (%s)", iter->Location.fullstr(), iter->ServiceType) << std::endl;
          }
          else {
            out << IString(" RemoteLogging: %s (%s)", iter->Location.fullstr(), iter->ServiceType) << std::endl;
          }
        }
      }

      if (!Application.Environment.empty()) {
        std::list< std::pair<std::string, std::string> >::const_iterator iter = Application.Environment.begin();
        for (; iter != Application.Environment.end(); iter++) {
          out << IString(" Environment.name: %s", iter->first) << std::endl;
          out << IString(" Environment: %s", iter->second) << std::endl;
        }
      }

      INTPRINT(out, Application.Rerun, Rerun)

      if (!Application.PreExecutable.empty()) {
        std::list<ExecutableType>::const_iterator itPreEx = Application.PreExecutable.begin();
        for (; itPreEx != Application.PreExecutable.end(); ++itPreEx) {
          STRPRINT(out, itPreEx->Path, PreExecutable)
          if (!itPreEx->Argument.empty()) {
            std::list<std::string>::const_iterator iter = itPreEx->Argument.begin();
            for (; iter != itPreEx->Argument.end(); ++iter)
              out << IString(" PreExecutable.Argument: %s", *iter) << std::endl;
          }
          if (itPreEx->SuccessExitCode.first) {
            out << IString(" Exit code for successful execution: %d", itPreEx->SuccessExitCode.second) << std::endl;
          }
          else {
            out << IString(" No exit code for successful execution specified.") << std::endl;
          }
        }
      }

      if (!Application.PostExecutable.empty()) {
        std::list<ExecutableType>::const_iterator itPostEx = Application.PostExecutable.begin();
        for (; itPostEx != Application.PostExecutable.end(); ++itPostEx) {
          STRPRINT(out, itPostEx->Path, PostExecutable)
          if (!itPostEx->Argument.empty()) {
            std::list<std::string>::const_iterator iter = itPostEx->Argument.begin();
            for (; iter != itPostEx->Argument.end(); ++iter)
              out << IString(" PostExecutable.Argument: %s", *iter) << std::endl;
          }
          if (itPostEx->SuccessExitCode.first) {
            out << IString(" Exit code for successful execution: %d", itPostEx->SuccessExitCode.second) << std::endl;
          }
          else {
            out << IString(" No exit code for successful execution specified.") << std::endl;
          }
        }
      }

      INTPRINT(out, Resources.SessionLifeTime.GetPeriod(), SessionLifeTime)

      if (bool(Application.AccessControl)) {
        std::string str;
        Application.AccessControl.GetXML(str, true);
        out << IString(" Access control: %s", str) << std::endl;
      }

      if (Application.ProcessingStartTime.GetTime() > 0)
        out << IString(" Processing start time: %s", Application.ProcessingStartTime.str()) << std::endl;

      if (Application.Notification.size() > 0) {
        out << IString(" Notify:") << std::endl;
        for (std::list<NotificationType>::const_iterator it = Application.Notification.begin();
             it != Application.Notification.end(); it++) {
          for (std::list<std::string>::const_iterator it2 = it->States.begin();
               it2 != it->States.end(); it2++) {
            out << " " << *it2;
          }
          out << ":   " << it->Email << std::endl;
        }
      }

      if (!Application.CredentialService.empty()) {
        std::list<URL>::const_iterator iter = Application.CredentialService.begin();
        for (; iter != Application.CredentialService.end(); iter++)
          out << IString(" Credential service: %s", iter->str()) << std::endl;
      }

      INTPRINT(out, Resources.TotalCPUTime.range.max, TotalCPUTime)
      INTPRINT(out, Resources.IndividualCPUTime.range.max, IndividualCPUTime)
      INTPRINT(out, Resources.IndividualWallTime.range.max, IndividualWallTime)

      STRPRINT(out, Resources.NetworkInfo, NetworkInfo)

      if (!Resources.OperatingSystem.empty()) {
        out << IString(" Operating system requirements:") << std::endl;
        std::list<Software>::const_iterator itOS = Resources.OperatingSystem.getSoftwareList().begin();
        std::list<Software::ComparisonOperatorEnum>::const_iterator itCO = Resources.OperatingSystem.getComparisonOperatorList().begin();
        for (; itOS != Resources.OperatingSystem.getSoftwareList().end(); itOS++, itCO++) {
          if (*itCO != Software::EQUAL) out << Software::toString(*itCO) << " ";
          out << *itOS << std::endl;
        }
      }

      STRPRINT(out, Resources.Platform, Platform)
      INTPRINT(out, Resources.IndividualPhysicalMemory.max, IndividualPhysicalMemory)
      INTPRINT(out, Resources.IndividualVirtualMemory.max, IndividualVirtualMemory)
      INTPRINT(out, Resources.DiskSpaceRequirement.DiskSpace.max, DiskSpace [MB])
      INTPRINT(out, Resources.DiskSpaceRequirement.CacheDiskSpace, CacheDiskSpace [MB])
      INTPRINT(out, Resources.DiskSpaceRequirement.SessionDiskSpace, SessionDiskSpace [MB])
      STRPRINT(out, Resources.QueueName, QueueName)

      if (!Resources.CEType.empty()) {
        out << IString(" Computing endpoint requirements:") << std::endl;
        std::list<Software>::const_iterator itCE = Resources.CEType.getSoftwareList().begin();
        std::list<Software::ComparisonOperatorEnum>::const_iterator itCO = Resources.CEType.getComparisonOperatorList().begin();
        for (; itCE != Resources.CEType.getSoftwareList().end(); itCE++, itCO++) {
          if (*itCO != Software::EQUAL) out << Software::toString(*itCO) << " ";
          out << *itCE << std::endl;
        }
      }

      switch (Resources.NodeAccess) {
      case NAT_NONE:
        break;
      case NAT_INBOUND:
        out << IString(" Node access: inbound") << std::endl;
        break;
      case NAT_OUTBOUND:
        out << IString(" Node access: outbound") << std::endl;
        break;
      case NAT_INOUTBOUND:
        out << IString(" Node access: inbound and outbound") << std::endl;
        break;
      }

      INTPRINT(out, Resources.SlotRequirement.NumberOfSlots, NumberOfSlots)
      INTPRINT(out, Resources.SlotRequirement.SlotsPerHost, SlotsPerHost)
      switch (Resources.SlotRequirement.ExclusiveExecution) {
      case SlotRequirementType::EE_DEFAULT:
        break;
      case SlotRequirementType::EE_TRUE:
        out << IString(" Job requires exclusive execution") << std::endl;
        break;
      case SlotRequirementType::EE_FALSE:
        out << IString(" Job does not require exclusive execution") << std::endl;
        break;
      }

      if (!Resources.RunTimeEnvironment.empty()) {
        out << IString(" Run time environment requirements:") << std::endl;
        std::list<Software>::const_iterator itSW = Resources.RunTimeEnvironment.getSoftwareList().begin();
        std::list<Software::ComparisonOperatorEnum>::const_iterator itCO = Resources.RunTimeEnvironment.getComparisonOperatorList().begin();
        for (; itSW != Resources.RunTimeEnvironment.getSoftwareList().end(); itSW++, itCO++) {
          if (*itCO != Software::EQUAL) out << Software::toString(*itCO) << " ";
          out << *itSW << std::endl;
        }
      }

      if (!DataStaging.InputFiles.empty()) {
        std::list<InputFileType>::const_iterator iter = DataStaging.InputFiles.begin();
        for (; iter != DataStaging.InputFiles.end(); ++iter) {
          out << IString(" Inputfile element:") << std::endl;
          out << IString("     Name: %s", iter->Name) << std::endl;
          if (iter->IsExecutable) {
            out << IString("     Is executable: true") << std::endl;
          }
          std::list<SourceType>::const_iterator itSource = iter->Sources.begin();
          for (; itSource != iter->Sources.end(); ++itSource) {
            out << IString("     Sources: %s", itSource->str()) << std::endl;
            if (!itSource->DelegationID.empty()) {
              out << IString("     Sources.DelegationID: %s", itSource->DelegationID) << std::endl;
            }
            for (std::multimap<std::string, std::string>::const_iterator itOptions = itSource->Options().begin();
                 itOptions != itSource->Options().end(); ++itOptions) {
              out << IString("     Sources.Options: %s = %s", itOptions->first, itOptions->second) << std::endl;
            }
          }
        }
      }

      if (!DataStaging.OutputFiles.empty()) {
        std::list<OutputFileType>::const_iterator iter = DataStaging.OutputFiles.begin();
        for (; iter != DataStaging.OutputFiles.end(); ++iter) {
          out << IString(" Outputfile element:") << std::endl;
          out << IString("     Name: %s", iter->Name) << std::endl;
          std::list<TargetType>::const_iterator itTarget = iter->Targets.begin();
          for (; itTarget != iter->Targets.end(); ++itTarget) {
            out << IString("     Targets: %s", itTarget->str()) << std::endl;
            if (!itTarget->DelegationID.empty()) {
              out << IString("     Targets.DelegationID: %s", itTarget->DelegationID) << std::endl;
            }
            for (std::multimap<std::string, std::string>::const_iterator itOptions = itTarget->Options().begin();
                 itOptions != itTarget->Options().end(); ++itOptions) {
              out << IString("     Targets.Options: %s = %s", itOptions->first, itOptions->second) << std::endl;
            }
          }
        }
      }

      if (!DataStaging.DelegationID.empty()) {
        out << IString(" DelegationID element: %s", DataStaging.DelegationID) << std::endl;
      }
    }

    if (!OtherAttributes.empty()) {
      std::map<std::string, std::string>::const_iterator it;
      for (it = OtherAttributes.begin(); it != OtherAttributes.end(); it++)
        out << IString(" Other attributes: [%s], %s", it->first, it->second) << std::endl;
    }

    out << std::endl;

    return true;
  } // end of Print

  void JobDescription::UseOriginal() {
    if (!alternatives.empty()) {
      std::list<JobDescription>::iterator it = alternatives.insert(current, *this); // Insert this before current position.
      it->RemoveAlternatives(); // No nested alternatives.
      Set(alternatives.front()); // Set this to first element.
      alternatives.pop_front(); // Remove this from list.
      current = alternatives.begin(); // Set current to first element.
    }
  }

  bool JobDescription::UseAlternative() {
    if (!alternatives.empty() && current != alternatives.end()) {
      std::list<JobDescription>::iterator it = alternatives.insert(current, *this); // Insert this before current position.
      it->RemoveAlternatives(); // No nested alternatives.
      Set(*current); // Set this to current.
      current = alternatives.erase(current); // Remove this from list.
      return true;
    }

    // There is no alternative JobDescription objects or end of list.
    return false;
  }

  void JobDescription::AddAlternative(const JobDescription& j) {
    alternatives.push_back(j);

    if (current == alternatives.end()) {
      current--; // If at end of list, set current to newly added jobdescription.
    }

    if (!j.alternatives.empty()) {
      alternatives.back().RemoveAlternatives(); // No nested alternatives.
      alternatives.insert(alternatives.end(), j.alternatives.begin(), j.alternatives.end());
    }
  }

  JobDescriptionResult JobDescription::ParseFromFile(const std::string& filename, std::list<JobDescription>& jobdescs, const std::string& language, const std::string& dialect) {
    std::ifstream descriptionfile(filename.c_str());
    if (!descriptionfile) {
      return JobDescriptionResult(false, "Can not open job description file: " + filename);
    }

    descriptionfile.seekg(0, std::ios::end);
    std::streamsize length = descriptionfile.tellg();
    descriptionfile.seekg(0, std::ios::beg);

    char *buffer = new char[length + 1];
    descriptionfile.read(buffer, length);
    descriptionfile.close();

    buffer[length] = '\0';
    JobDescriptionResult r = Parse((std::string)buffer, jobdescs);
    delete[] buffer;
    return r;
  }

  JobDescriptionResult JobDescription::Parse(const std::string& source, std::list<JobDescription>& jobdescs, const std::string& language, const std::string& dialect) {
    if (source.empty()) {
      logger.msg(ERROR, "Empty job description source string");
      return false;
    }

    jdpl_lock.lock();
    if (!jdpl) {
      jdpl = new JobDescriptionParserPluginLoader();
    }

    std::list< std::pair<std::string, JobDescriptionParserPluginResult> > results;

    bool has_parsers = false;
    bool has_languages = false;
    for (JobDescriptionParserPluginLoader::iterator it = jdpl->GetIterator(); it; ++it) {
      // Releasing lock because we can't know how long parsing will take
      // But for current implementations of parsers it is not specified
      // if their Parse/Unparse methods can be called concurently.
      has_parsers = true;
      if (language.empty() || it->IsLanguageSupported(language)) {
        has_languages = true;
        JobDescriptionParserPluginResult result = it->Parse(source, jobdescs, language, dialect);
        if (result) {
          jdpl_lock.unlock();
          return JobDescriptionResult(true);
        }

        results.push_back(std::make_pair(!it->GetSupportedLanguages().empty() ? it->GetSupportedLanguages().front() : "", result));
      }
    }
    jdpl_lock.unlock();

    std::string parse_error;
    if(!has_parsers) {
      parse_error = IString("No job description parsers available").str();
    } else if(!has_languages) {
      parse_error = IString("No job description parsers suitable for handling '%s' language are available", language).str();
    } else {
      for (std::list< std::pair<std::string, JobDescriptionParserPluginResult> >::iterator itRes = results.begin();
           itRes != results.end(); ++itRes) {
        if (itRes->second == JobDescriptionParserPluginResult::Failure) {
          if (!parse_error.empty()) {
            parse_error += "\n";
          }
          parse_error += IString("%s parsing error", itRes->first).str();
          if (itRes->second.HasErrors()) {
            parse_error += ":";
          }
          for (std::list<JobDescriptionParsingError>::const_iterator itErr = itRes->second.GetErrors().begin();
               itErr != itRes->second.GetErrors().end(); ++itErr) {
            parse_error += "\n";
            if (itErr->line_pos.first > 0 && itErr->line_pos.second > 0) {
              parse_error += inttostr(itErr->line_pos.first) + ":" + inttostr(itErr->line_pos.second) + ": ";
            }
            parse_error += itErr->message;
          }
        }
      }
    }
    if(parse_error.empty()) {
      parse_error = IString("No job description parser was able to interpret job description").str();
    }

    return JobDescriptionResult(false, parse_error);
  }

  JobDescriptionResult JobDescription::UnParse(std::string& product, std::string language, const std::string& dialect) const {
    if (language.empty()) {
      language = sourceLanguage;
      if (language.empty()) {
        logger.msg(ERROR, "Job description language is not specified, unable to output description.");
        return false;
      }
    }

    jdpl_lock.lock();
    if (!jdpl) {
      jdpl = new JobDescriptionParserPluginLoader();
    }

    for (JobDescriptionParserPluginLoader::iterator it = jdpl->GetIterator(); it; ++it) {
      if (it->IsLanguageSupported(language)) {
        logger.msg(VERBOSE, "Generating %s job description output", language);
        bool r = it->UnParse(*this, product, language, dialect);
        std::string unparse_error = it->GetError();
        JobDescriptionResult res(r,unparse_error);
        /* TOOD: This log message for some reason causes a race
         *       condition in globus on certain platforms. It has
         *       currently only been observed during job submission on
         *       Ubuntu 11.10 64bit, in which case job submission fails.
         *if (!r) logger.msg(VERBOSE, "Generating %s job description output failed: %s", language, unparse_error);
         */
        jdpl_lock.unlock();
        return res;
      }
    }
    jdpl_lock.unlock();

    logger.msg(ERROR, "Language (%s) not recognized by any job description parsers.", language);
    return JobDescriptionResult(false,"Language not recognized");
  }

  bool JobDescription::Prepare(const ExecutionTarget* et) {
    // Check for identical file names.
    // Check if executable and input is contained in the file list.
    bool executableIsAdded(false), inputIsAdded(false), outputIsAdded(false), errorIsAdded(false), logDirIsAdded(false);
    for (std::list<InputFileType>::iterator it1 = DataStaging.InputFiles.begin();
         it1 != DataStaging.InputFiles.end(); ++it1) {
      std::list<InputFileType>::const_iterator it2 = it1;
      for (++it2; it2 != DataStaging.InputFiles.end(); ++it2) {
        if (it1->Name == it2->Name && !it1->Sources.empty() && !it2->Sources.empty()) {
          logger.msg(ERROR, "Two input files have identical name '%s'.", it1->Name);
          return false;
        }
      }

      /*
      if (!it1->Sources.empty() && it1->Sources.front().Protocol() == "file" && !Glib::file_test(it1->Sources.front().Path(), Glib::FILE_TEST_EXISTS)) {
        logger.msg(ERROR, "Cannot stat local input file '%s'", it1->Sources.front().Path());
        return false;
      }
      */

      executableIsAdded  |= (it1->Name == Application.Executable.Path);
      inputIsAdded       |= (it1->Name == Application.Input);
    }

    if (!Application.Executable.Path.empty() && !executableIsAdded &&
        !Glib::path_is_absolute(Application.Executable.Path)) {
      if (!Glib::file_test(Application.Executable.Path, Glib::FILE_TEST_EXISTS)) {
        logger.msg(ERROR, "Cannot stat local input file '%s'", Application.Executable.Path);
        return false;
      }
      DataStaging.InputFiles.push_back(InputFileType());
      InputFileType& file = DataStaging.InputFiles.back();
      file.Name = Application.Executable.Path;
      file.Sources.push_back(URL(file.Name));
      file.IsExecutable = true;
    }

    if (!Application.Input.empty() && !inputIsAdded &&
        !Glib::path_is_absolute(Application.Input)) {
      if (!Glib::file_test(Application.Input, Glib::FILE_TEST_EXISTS)) {
        logger.msg(ERROR, "Cannot stat local input file '%s'", Application.Input);
        return false;
      }
      DataStaging.InputFiles.push_back(InputFileType());
      InputFileType& file = DataStaging.InputFiles.back();
      file.Name = Application.Input;
      file.Sources.push_back(URL(file.Name));
      file.IsExecutable = false;
    }

    for (std::list<InputFileType>::iterator it1 = DataStaging.InputFiles.begin();
         it1 != DataStaging.InputFiles.end(); ++it1) {
      if (it1->Name.empty()) continue; // undefined input fule
      if (it1->Sources.empty() || (it1->Sources.front().Protocol() == "file")) {
        std::string path = it1->Name;
        if (!it1->Sources.empty()) path = it1->Sources.front().Path();
        // Local file
        // Check presence
        struct stat st;
        if (!FileStat(path,&st,true) || !S_ISREG(st.st_mode)) {
          logger.msg(ERROR, "Cannot find local input file '%s' (%s)", path, it1->Name);
          return false;
        }
        // Collect information about file
        if (it1->FileSize < 0) it1->FileSize = st.st_size; // TODO: if FileSize defined compare?
        if (it1->Checksum.empty() && (st.st_size <= 65536)) {
          // Checksum is only done for reasonably small files
          // Checkum type is chosen for xRSL
          it1->Checksum = CheckSumAny::FileChecksum(path,CheckSumAny::cksum,true);
        }
      }
    }

    for (std::list<OutputFileType>::iterator it1 = DataStaging.OutputFiles.begin();
         it1 != DataStaging.OutputFiles.end(); ++it1) {
      outputIsAdded      |= (it1->Name == Application.Output);
      errorIsAdded       |= (it1->Name == Application.Error);
      logDirIsAdded      |= (it1->Name == Application.LogDir);
    }

    if (!Application.Output.empty() && !outputIsAdded) {
      DataStaging.OutputFiles.push_back(OutputFileType());
      OutputFileType& file = DataStaging.OutputFiles.back();
      file.Name = Application.Output;
    }

    if (!Application.Error.empty() && !errorIsAdded) {
      DataStaging.OutputFiles.push_back(OutputFileType());
      OutputFileType& file = DataStaging.OutputFiles.back();
      file.Name = Application.Error;
    }

    if (!Application.LogDir.empty() && !logDirIsAdded) {
      DataStaging.OutputFiles.push_back(OutputFileType());
      OutputFileType& file = DataStaging.OutputFiles.back();
      file.Name = Application.LogDir;
    }

    if (et != NULL) {
      if (!Resources.RunTimeEnvironment.empty() &&
          !Resources.RunTimeEnvironment.selectSoftware(*et->ApplicationEnvironments)) {
        // This error should never happen since RTE is checked in the Broker.
        logger.msg(VERBOSE, "Unable to select runtime environment");
        return false;
      }

      if (!Resources.CEType.empty() &&
          !Resources.CEType.selectSoftware(et->ComputingEndpoint->Implementation)) {
        // This error should never happen since Middleware is checked in the Broker.
        logger.msg(VERBOSE, "Unable to select middleware");
        return false;
      }

      if (!Resources.OperatingSystem.empty() &&
          !Resources.OperatingSystem.selectSoftware(et->ExecutionEnvironment->OperatingSystem)) {
        // This error should never happen since OS is checked in the Broker.
        logger.msg(VERBOSE, "Unable to select operating system.");
        return false;
      }

      // Set queue name to the selected ExecutionTarget
      if(et->ComputingShare->MappingQueue.empty()) {
        Resources.QueueName = et->ComputingShare->Name;
      } else {
        Resources.QueueName = et->ComputingShare->MappingQueue;
      }
    }

    return true;
  }

  bool JobDescription::GetTestJob(int testid, JobDescription& jobdescription) {
    const std::string testJobFileName(ArcLocation::Get() + G_DIR_SEPARATOR_S PKGDATASUBDIR G_DIR_SEPARATOR_S "test-jobs" G_DIR_SEPARATOR_S "test-job-" + tostring(testid));
    std::ifstream testJobFile(testJobFileName.c_str());
    if (!Glib::file_test(testJobFileName, Glib::FILE_TEST_IS_REGULAR) || !testJobFile) {
      logger.msg(ERROR, "No test-job with ID %d found.", testid);
      return false;
    }

    std::string description;
    testJobFile.seekg(0, std::ios::end);
    description.reserve(testJobFile.tellg());
    testJobFile.seekg(0, std::ios::beg);
    description.assign((std::istreambuf_iterator<char>(testJobFile)), std::istreambuf_iterator<char>());

    std::list<JobDescription> jobdescs;
    if (!JobDescription::Parse(description, jobdescs, "nordugrid:xrsl")) {
      logger.msg(ERROR, "Test was defined with ID %d, but some error occurred during parsing it.", testid);
      return false;
    }
    if (jobdescs.empty()) {
      logger.msg(ERROR, "No jobdescription resulted at %d test", testid);
      return false;
    }
    jobdescription = (*(jobdescs.begin()));
    return true;
  }
} // namespace Arc
