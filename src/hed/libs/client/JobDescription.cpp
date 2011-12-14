// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include <arc/client/JobDescriptionParser.h>

#include "JobDescription.h"


#define INTPRINT(OUT, X, Y) if ((X) > -1) \
  OUT << IString(#Y ": %d", X) << std::endl;
#define STRPRINT(OUT, X, Y) if (!(X).empty()) \
  OUT << IString(#Y ": %s", X) << std::endl;


namespace Arc {

  Logger JobDescription::logger(Logger::getRootLogger(), "JobDescription");

  // Maybe this mutex could go to JobDescriptionParserLoader. That would make
  // it transparent. On another hand JobDescriptionParserLoader must not know
  // how it is used.
  Glib::Mutex JobDescription::jdpl_lock;
  // TODO: JobDescriptionParserLoader need to be freed when not used any more.
  JobDescriptionParserLoader *JobDescription::jdpl = NULL;

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

  void JobDescription::Set(const JobDescription& j) {
    Identification = j.Identification;
    Application = j.Application;
    Resources = j.Resources;
    DataStaging = j.DataStaging;

    OtherAttributes = j.OtherAttributes;

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
      INTPRINT(out, Resources.TotalWallTime.range.max, TotalWallTime)
      INTPRINT(out, Resources.IndividualWallTime.range.max, IndividualWallTime)

      STRPRINT(out, Resources.NetworkInfo, NetworkInfo)

      if (!Resources.OperatingSystem.empty()) {
        out << IString(" Operating system requirements:") << std::endl;
        std::list<Software>::const_iterator itOS = Resources.OperatingSystem.getSoftwareList().begin();
        std::list<Software::ComparisonOperator>::const_iterator itCO = Resources.OperatingSystem.getComparisonOperatorList().begin();
        for (; itOS != Resources.OperatingSystem.getSoftwareList().end(); itOS++, itCO++) {
          if (*itCO != &Software::operator==) out << Software::toString(*itCO) << " ";
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
        std::list<Software::ComparisonOperator>::const_iterator itCO = Resources.CEType.getComparisonOperatorList().begin();
        for (; itCE != Resources.CEType.getSoftwareList().end(); itCE++, itCO++) {
          if (*itCO != &Software::operator==) out << Software::toString(*itCO) << " ";
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
        out << IString(" Job dosn't require exclusive execution") << std::endl;
        break;
      }

      if (!Resources.RunTimeEnvironment.empty()) {
        out << IString(" Run time environment requirements:") << std::endl;
        std::list<Software>::const_iterator itSW = Resources.RunTimeEnvironment.getSoftwareList().begin();
        std::list<Software::ComparisonOperator>::const_iterator itCO = Resources.RunTimeEnvironment.getComparisonOperatorList().begin();
        for (; itSW != Resources.RunTimeEnvironment.getSoftwareList().end(); itSW++, itCO++) {
          if (*itCO != &Software::operator==) out << Software::toString(*itCO) << " ";
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

  JobDescriptionResult JobDescription::Parse(const std::string& source, std::list<JobDescription>& jobdescs, const std::string& language, const std::string& dialect) {
    if (source.empty()) {
      logger.msg(ERROR, "Empty job description source string");
      return false;
    }

    jdpl_lock.lock();
    if (!jdpl) {
      jdpl = new JobDescriptionParserLoader();
    }

    std::string parse_error;
    for (JobDescriptionParserLoader::iterator it = jdpl->GetIterator(); it; ++it) {
      // Releasing lock because we can't know how long parsing will take
      // But for current implementations of parsers it is not specified
      // if their Parse/Unparse methods can be called concurently.
      if (language.empty() || it->IsLanguageSupported(language)) {
        if (it->Parse(source, jobdescs, language, dialect)) {
          jdpl_lock.unlock();
          return true;
        }
        if (!it->GetError().empty()) {
          parse_error += it->GetError();
          parse_error += "\n";
        }
      }
    }
    jdpl_lock.unlock();

    return JobDescriptionResult(false,parse_error);
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
      jdpl = new JobDescriptionParserLoader();
    }

    for (JobDescriptionParserLoader::iterator it = jdpl->GetIterator(); it; ++it) {
      if (it->IsLanguageSupported(language)) {
        logger.msg(VERBOSE, "Generating %s job description output", language);
        bool r = it->UnParse(*this, product, language, dialect);
        std::string unparse_error = it->GetError();
        JobDescriptionResult res(r,unparse_error);
        jdpl_lock.unlock();
        return res;
      }
    }
    jdpl_lock.unlock();

    logger.msg(ERROR, "Language (%s) not recognized by any job description parsers.", language);
    return JobDescriptionResult(false,"Language not recognized");
  }
} // namespace Arc
