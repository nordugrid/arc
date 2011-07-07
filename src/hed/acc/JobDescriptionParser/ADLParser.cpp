// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <glibmm.h>

#include <arc/DateTime.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/client/JobDescription.h>

#include "ADLParser.h"

namespace Arc {

  ADLParser::ADLParser()
    : JobDescriptionParser() {
    supportedLanguages.push_back("emies:adl");
  }

  ADLParser::~ADLParser() {}

  Plugin* ADLParser::Instance(PluginArgument *arg) {
    return new ADLParser();
  }

  static void XmlErrorHandler(void* /* ctx */, const char* /* msg */) {
    return;
  }

  bool ADLParser::Parse(const std::string& source, std::list<JobDescription>& jobdescs, const std::string& language, const std::string& dialect) const {
    if (language != "" && !IsLanguageSupported(language)) {
      return false;
    }
    logger.msg(VERBOSE, "Parsing EMI ES ADL is not supported yet");
    return false;
  }

  bool ADLParser::UnParse(const JobDescription& job, std::string& product, const std::string& language, const std::string& dialect) const {
    if (!IsLanguageSupported(language)) {
      return false;
    }

    NS ns;
    ns[""] = "http://www.eu-emi.eu/es/2010/12/adl";
    ns["emiestypes"] = "http://www.eu-emi.eu/es/2010/12/types";

    // ActivityDescription
    XMLNode description(ns, "ActivityDescription");
    XMLNode identification = description.NewChild("ActivityIdentification");
    XMLNode application = description.NewChild("Application");
    XMLNode resources = description.NewChild("Resources");
    XMLNode staging = description.NewChild("DataStaging");

    // ActivityIdentification
    if(!job.Identification.JobName.empty()) identification.NewChild("Name") = job.Identification.JobName;
    if(!job.Identification.Description.empty()) identification.NewChild("Description") = job.Identification.Description;
    identification.NewChild("Type") = "single"; // optional
    for (std::list<std::string>::const_iterator it = job.Identification.UserTag.begin();
         it != job.Identification.UserTag.end(); it++) {
      identification.NewChild("Annotation") = *it;
    }
    // job.Identification.JobVOName;
    // job.Identification.ActivityOldId

    // Application
    XMLNode executable = application.NewChild("Executable");
    if(!job.Application.Executable.Name.empty()) {
      executable.NewChild("Path") = job.Application.Executable.Name;
    }
    for (std::list<std::string>::const_iterator it = job.Application.Executable.Argument.begin();
         it != job.Application.Executable.Argument.end(); it++) {
      executable.NewChild("Argument") = *it;
    }
    if(executable.Size() > 0) {
      executable.NewChild("FailIfExitCodeNotEqualTo") = "0";
    } else {
      executable.Destroy();
    }
    if(!job.Application.Input.empty()) application.NewChild("Input") = job.Application.Input;
    if(!job.Application.Output.empty()) application.NewChild("Output") = job.Application.Output;
    if(!job.Application.Error.empty()) application.NewChild("Error") = job.Application.Error;
    for(std::list< std::pair<std::string, std::string> >::const_iterator it = 
        job.Application.Environment.begin(); it != job.Application.Environment.end(); it++) {
      XMLNode environment = application.NewChild("Environment");
      environment.NewChild("Name") = it->first;
      environment.NewChild("Value") = it->second;
    }
    if(!job.Application.Prologue.Name.empty()) {
      XMLNode prologue = application.NewChild("PreExecutable");
      prologue.NewChild("Path") = job.Application.Prologue.Name;
      for(std::list<std::string>::const_iterator it = job.Application.Prologue.Argument.begin();
           it != job.Application.Prologue.Argument.end(); it++) {
        prologue.NewChild("Argument") = *it;
      }
    }
    if(!job.Application.Epilogue.Name.empty()) {
      XMLNode epilogue = application.NewChild("PostExecutable");
      epilogue.NewChild("Path") = job.Application.Epilogue.Name;
      for(std::list<std::string>::const_iterator it = job.Application.Epilogue.Argument.begin();
           it != job.Application.Epilogue.Argument.end(); it++) {
        epilogue.NewChild("Argument") = *it;
      }
    }
    for (std::list<URL>::const_iterator it = job.Application.RemoteLogging.begin();
         it != job.Application.RemoteLogging.end(); it++) {
      XMLNode logging = application.NewChild("RemoteLogging");
      logging.NewChild("ServiceType") = "SGAS"; // ???
      logging.NewChild("URL") = it->str();
    }
    if(job.Application.ExpiryTime > -1) application.NewChild("ExpirationTime") = job.Application.ExpiryTime.str();
    if(job.Resources.SessionLifeTime > -1) application.NewChild("WipeTime") = tostring(job.Resources.SessionLifeTime); // TODO: use ADL types
    for (std::list<NotificationType>::const_iterator it = job.Application.Notification.begin();
         it != job.Application.Notification.end(); it++) {
      XMLNode notification = application.NewChild("Notification");
      notification.NewChild("Protocol") = "email";
      notification.NewChild("Recipient") = it->Email;
      for (std::list<std::string>::const_iterator s = it->States.begin();
                  s != it->States.end(); s++) {
        notification.NewChild("OnState") = *s; // TODO: convert to EMI ES states
      }
    }
    // job.Application.LogDir
    // job.Application.Rerun
    // job.Application.Priority
    // job.Application.ProcessingStartTime
    // job.Application.AccessControl
    // job.Application.CredentialService
    // job.Application.DryRun

    // Resources
    
    if(!job.Resources.OperatingSystem.empty()) {
      XMLNode os = resources.NewChild("OperatingSystem");
      os.NewChild("Name") = job.Resources.OperatingSystem.getSoftwareList().front().getName();
      // TODO: convert to EMI ES types
      // OperatingSystem.Name, OperatingSystem.Family
      os.NewChild("Version") = job.Resources.OperatingSystem.getSoftwareList().front().getVersion();
    }
    if(!job.Resources.Platform.empty()) {
      // TODO: convert to EMI ES types
      resources.NewChild("Platform") = job.Resources.Platform;
    }
    for(std::list<Software>::const_iterator s =
                    job.Resources.RunTimeEnvironment.getSoftwareList().begin();
                    s != job.Resources.RunTimeEnvironment.getSoftwareList().end();++s) {
      XMLNode rte = resources.NewChild("RuntimeEnvironment");
      rte.NewChild("Name") = s->getName();
      rte.NewChild("Version") = s->getVersion();
      //  Option
    }
    //ParallelEnvironment
    //  Type
    //  Version
    //  ProcessesPerSlot
    //  ThreadsPerProcess
    //  Option
    //    Name
    //    Value
    //Coprocessor
    //NetworkInfo
    switch(job.Resources.NodeAccess) {
      case NAT_INBOUND: resources.NewChild("NodeAccess") = "inbound"; break;
      case NAT_OUTBOUND: resources.NewChild("NodeAccess") = "outbound"; break;
      case NAT_INOUTBOUND: resources.NewChild("NodeAccess") = "inoutbound"; break;
    }
    if(job.Resources.IndividualPhysicalMemory.max != -1) {
      resources.NewChild("IndividualPhysicalMemory") = tostring(job.Resources.IndividualPhysicalMemory.max);
    }
    if(job.Resources.IndividualVirtualMemory.max != -1) {
      resources.NewChild("IndividualVirtualMemory") = tostring(job.Resources.IndividualVirtualMemory.max);
    }
    if(job.Resources.DiskSpaceRequirement.DiskSpace.min > -1) {
      resources.NewChild("DiskSpaceRequirement") = tostring(job.Resources.DiskSpaceRequirement.DiskSpace.min*1024*1024); // TODO: adapt units to ADL
    }
    //RemoteSessionAccess
    //Benchmark
    //  BenchmarkType
    //  BenchmarkValue
    XMLNode slot = resources.NewChild("SlotRequirement");
    if(job.Resources.SlotRequirement.NumberOfSlots.max != -1) {
      slot.NewChild("NumberOfSlots") = tostring(job.Resources.SlotRequirement.NumberOfSlots.max);
    }
    if (job.Resources.SlotRequirement.ThreadsPerProcesses.max != -1) {
      slot.NewChild("SlotsPerHost") = tostring(job.Resources.SlotRequirement.ThreadsPerProcesses.max);
    }
    if(slot.Size() <= 0) slot.Destroy();
    if(!job.Resources.QueueName.empty()) {;
      resources.NewChild("QueueName") = job.Resources.QueueName;
    }

    // job.Resources.TotalWallTime.range.max
    // job.Resources.TotalCPUTime.range.max
    // job.Resources.NetworkInfo
    // job.Resources.DiskSpaceRequirement.CacheDiskSpace
    // job.Resources.DiskSpaceRequirement.SessionDiskSpace
    // job.Resources.IndividualCPUTime.range
    // job.Resources.IndividualCPUTime.benchmark
    // job.Resources.TotalCPUTime.range
    // job.Resources.TotalCPUTime.benchmark
    // job.Resources.IndividualWallTime.range
    // job.Resources.IndividualWallTime.benchmark
    // job.Resources.TotalWallTime.range
    // job.Resources.TotalWallTime.benchmark
    // job.Resources.CEType
    // job.Resources.SlotRequirement.ThreadsPerProcesses
    // job.Resources.SlotRequirement.SPMDVariation

    // DataStaging

    // XMLNode clientpush = staging.NewChild("ClientDataPush"); TODO

    // InputFile
    for (std::list<FileType>::const_iterator it = job.Files.begin();
         it != job.Files.end(); it++) {
      if(it->Name.empty()) continue; // mandatory
      if(!it->Source.empty()) {
        XMLNode file = staging.NewChild("InputFile");
        file.NewChild("Name") = it->Name;
        for(std::list<URL>::const_iterator u = it->Source.begin();
                       u != it->Source.end(); ++u) {
          if(!*u) continue; // mandatory
          XMLNode source = file.NewChild("Source");
          source.NewChild("URI") = u->str(); // TODO file://
          //source.NewChild("DelegationID") = ; TODO
          //source.NewChild("Option") = ; TODO
        }
        if(it->IsExecutable || it->Name == job.Application.Executable.Name) {
          file.NewChild("IsExecutable") = "true";
        }
        // it->FileSize
      }
    }
    // OutputFile
    for (std::list<FileType>::const_iterator it = job.Files.begin();
         it != job.Files.end(); it++) {
      if(it->Name.empty()) continue; // mandatory
      if(it->KeepData || !it->Target.empty()) {
        XMLNode file = staging.NewChild("OutputFile");
        file.NewChild("Name") = it->Name;
        for(std::list<URL>::const_iterator u = it->Target.begin();
                       u != it->Target.end(); ++u) {
          if(!*u) continue; // mandatory
          XMLNode target = file.NewChild("Target");
          target.NewChild("URI") = u->str();
          // target.NewChild("DelegationID") = ;
          // target.NewChild("Option") = ;
          // target.NewChild("Mandatory") = ;
          // target.NewChild("CreationFlag") = ;
          target.NewChild("UseIfFailure") = "false";
          target.NewChild("UseIfCancel") = "false";
          target.NewChild("UseIfSuccess") = "true";
        }
      }
    }

    description.GetDoc(product, true);

    return true;
  }

} // namespace Arc
