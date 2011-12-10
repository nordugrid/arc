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

  static bool IsOptional(XMLNode el) {
    XMLNode optional = el.Attribute("optional");
    if(!optional) return false;
    std::string v = optional;
    if(v == "false") return false;
    if(v == "0") return false;
    return true;
  }

  static bool ParseOptional(XMLNode el, Logger& logger) {
    XMLNode optional = el.Attribute("optional");
    if(!optional) return true;
    std::string v = optional;
    if(v == "false") return true;
    if(v == "0") return true;
    logger.msg(ERROR, "[ADLParser] Optional %s elements are not supported yet.", el.Name());
    return false;
  }

  static bool ParseFlagTrue(XMLNode el, Logger& logger) {
    if(!el) return true;
    std::string v = el;
    if((v != "false") && (v != "0")) return true;
    logger.msg(ERROR, "[ADLParser] %s element with false value is not supported yet.", el.Name());
    return false;
  }

  static bool ParseFlagFalse(XMLNode el, Logger& logger) {
    if(!el) return true;
    std::string v = el;
    if((v == "false") || (v == "0")) return true;
    logger.msg(ERROR, "[ADLParser] %s element with true value is not supported yet.", el.Name());
    return false;
  }

  static bool ParseFlag(XMLNode el, Logger& logger, bool& val) {
    if(!el) return true;
    std::string v = el;
    if((v == "false") || (v == "0")) { val = false; return true; }
    if((v == "true") || (v == "1")) { val = true; return true; }
    logger.msg(ERROR, "[ADLParser] %s element must be boolean.", el.Name());
    return false;
  }

  static bool ParseFailureCode(XMLNode executable, std::pair<bool, int>& sec, const std::string& dialect, Logger& logger) {
    XMLNode failcode = executable["adl:FailIfExitCodeNotEqualTo"];
    sec.first = (bool)failcode;
    if (!sec.first) {
      return true;
    }
    
    if(!stringto((std::string)failcode, sec.second)) {
      logger.msg(ERROR, "[ADLParser] Code in FailIfExitCodeNotEqualTo in %s is not valid number.",executable.Name());
      return false;
    }
    
    if(dialect == "GRIDMANAGER" && sec.second != 0) {
      logger.msg(ERROR, "[ADLParser] FailIfExitCodeNotEqualTo in %s contain non-zero code. This feature is not supported yet.", executable.Name());
      return false;
    }
    return true;
  }

  static bool ParseExecutable(XMLNode executable, ExecutableType& exec, const std::string& dialect, Logger& logger) {
    exec.Path = (std::string)executable["adl:Path"];
    for(XMLNode argument = executable["adl:Argument"];
                                    (bool)argument;++argument) {
      exec.Argument.push_back((std::string)argument);
    }
    if(!ParseFailureCode(executable, exec.SuccessExitCode, dialect, logger)) {
      return false;
    }
    return true;
  }

  bool ADLParser::Parse(const std::string& source, std::list<JobDescription>& jobdescs, const std::string& language, const std::string& dialect) const {
    if (language != "" && !IsLanguageSupported(language)) {
      return false;
    }
    logger.msg(VERBOSE, "Parsing string using ADLParser");

    jobdescs.clear();

    jobdescs.push_back(JobDescription());
    JobDescription& job = jobdescs.back();

    XMLNode node(source);
    if (!node) {
        logger.msg(VERBOSE, "[ADLParser] Parsing error: %s\n", (xmlGetLastError())->message);
    }

    if (node.Size() == 0) {
      logger.msg(VERBOSE, "[ADLParser] Wrong XML structure! ");
      jobdescs.clear();
      return false;
    }

    /*
      ActivityDescription
        ActivityIdentification minOccurs=0
          Name minOccurs=0
          Description minOccurs=0
          Type minOccurs=0
            "collectionelement"
            "parallelelement"
            "single"
            "workflownode"
          Annotation minOccurs=0 maxOccurs=unbounded
        Application minOccurs=0
          Executable minOccurs=0
            Path
            Argument minOccurs=0 maxOccurs=unbounded
            FailIfExitCodeNotEqualTo minOccurs=0
          Input minOccurs=0
          Output minOccurs=0
          Error minOccurs=0
          Environment minOccurs=0 maxOccurs=unbounded
            Name
            Value
          PreExecutable minOccurs=0 maxOccurs=unbounded
            Path
            Argument minOccurs=0 maxOccurs=unbounded
            FailIfExitCodeNotEqualTo minOccurs=0
          PostExecutable minOccurs=0 maxOccurs=unbounded
            Path
            Argument minOccurs=0 maxOccurs=unbounded
            FailIfExitCodeNotEqualTo minOccurs=0
          RemoteLogging minOccurs=0 maxOccurs=unbounded
            ServiceType
            URL minOccurs=0
            attr:optional=false
          ExpirationTime(dateTime) minOccurs=0
            attr:optional=false
          WipeTime(dateTime) minOccurs=0
            attr:optional=false
          Notification minOccurs=0 maxOccurs=unbounded
            Protocol
             "email"
            Recipient maxOccurs=unbounded
            OnState(esmain:PrimaryActivityStatus) minOccurs=0 maxOccurs=unbounded
            attr:optional
        Resources minOccurs=0
          OperatingSystem minOccurs=0
            Name
              aix
              centos
              debian
              fedoracore
              gentoo
              leopard
              linux-rocks
              mandrake
              redhatenterpriseas
              scientificlinux
              scientificlinuxcern
              slackware
              suse
              ubuntu
              windowsvista
              windowsxp
            Family minOccurs=0
              "linux"
              "macosx"
              "solaris"
              "windows"
            Version minOccurs=0
          Platform minOccurs=0
            "amd64"
            "i386"
            "itanium"
            "powerpc"
            "sparc"
          RuntimeEnvironment minOccurs=0 maxOccurs=unbounded
            Name !!!
              aix
              centos
              debian
              fedoracore
              gentoo
              leopard
              linux-rocks
              mandrake
              redhatenterpriseas
              scientificlinux
              scientificlinuxcern
              slackware
              suse
              ubuntu
              windowsvista
              windowsxp
            Version minOccurs=0
            Option minOccurs=0 maxOccurs=unbounded
            attr:optional=false
          ParallelEnvironment minOccurs=0
            Type minOccurs=0
              "MPI"
              "GridMPI"
              "IntelMPI"
              "LAM-MPI"
              "MPICH1"
              "MPICH2"
              "MPICH-GM"
              "MPICH-MX"
              "MVAPICH"
              "MVAPICH2"
              "OpenMPI"
              "POE"
              "PVM"
            Version minOccurs=0
            ProcessesPerSlot minOccurs=0
            ThreadsPerProcess minOccurs=0
            Option minOccurs=0 maxOccurs=unbounded
              Name
              Value
          Coprocessor minOccurs=0
            "CUDA"
            "FPGA"
            attr:optional
          NetworkInfo minOccurs=0
            "100megabitethernet"
            "gigabitethernet"
            "10gigabitethernet"
            "infiniband"
            "myrinet"
            attr:optional
          NodeAccess minOccurs=0
            "inbound"
            "outbound"
            "inoutbound"
          IndividualPhysicalMemory minOccurs=0
          IndividualVirtualMemory minOccurs=0
          DiskSpaceRequirement minOccurs=0
          RemoteSessionAccess minOccurs=0
          Benchmark minOccurs=0
            BenchmarkType
              bogomips
              cfp2006
              cint2006
              linpack
              specfp2000
              specint2000
            BenchmarkValue
            attr:optional=false
          SlotRequirement minOccurs=0
            NumberOfSlots
            SlotsPerHost minOccurs=0
              attr:useNumberOfSlots=false
            ExclusiveExecution minOccurs=0
          QueueName minOccurs=0
        DataStaging minOccurs=0
          ClientDataPush
          InputFile minOccurs=0 maxOccurs=unbounded
            Name
            Source minOccurs=0 maxOccurs=unbounded
              URI
              DelegationID minOccurs=0
              Option minOccurs=0 maxOccurs=unbounded
                Name
                Value
            IsExecutable minOccurs=0
          OutputFile minOccurs=0 maxOccurs=unbounded
            Name
            Target minOccurs=0 maxOccurs=unbounded
              URI
              DelegationID minOccurs=0
              Option minOccurs=0 maxOccurs=unbounded
                Name
                Value
              Mandatory minOccurs=0
              CreationFlag minOccurs=0
                "overwrite"
                "append"
                "dontOverwrite"
              UseIfFailure minOccurs=0
              UseIfCancel minOccurs=0
              UseIfSuccess minOccurs=0
    */
    NS ns;
    ns["adl"] = "http://www.eu-emi.eu/es/2010/12/adl";
    ns["nordugrid-adl"] = "http://www.nordugrid.org/es/2011/12/nordugrid-adl";
    node.Namespaces(ns);

    // ActivityDescription
    if(!MatchXMLName(node,"adl:ActivityDescription")) {
      logger.msg(VERBOSE, "[ADLParser] Root element is not ActivityDescription ");
      jobdescs.clear();
      return false;
    }

    XMLNode identification = node["adl:ActivityIdentification"];
    XMLNode application = node["adl:Application"];
    XMLNode resources = node["adl:Resouces"];
    XMLNode staging = node["adl:DataStaging"];

    if((bool)identification) {
      job.Identification.JobName = (std::string)identification["adl:Name"];
      job.Identification.Description = (std::string)identification["adl:Description"];
      job.Identification.Type = (std::string)identification["adl:Type"];
      for(XMLNode annotation = identification["adl:Annotation"];
                                      (bool)annotation;++annotation) {
        job.Identification.Annotation.push_back((std::string)annotation);
      }
      // ARC extension: ActivityOldID
      for(XMLNode activityoldid = identification["nordugrid-adl:ActivityOldID"];
                                      (bool)activityoldid;++activityoldid) {
        job.Identification.ActivityOldID.push_back((std::string)activityoldid);
      }
    }
    if((bool)application) {
      XMLNode executable = application["adl:Executable"];
      if(executable && !ParseExecutable(executable, job.Application.Executable, dialect, logger)) {
        jobdescs.clear();
        return false;
      }
      job.Application.Input = (std::string)application["adl:Input"];
      job.Application.Output = (std::string)application["adl:Output"];
      job.Application.Error = (std::string)application["adl:Error"];
      for(XMLNode environment = application["adl:Environment"];
                         (bool)environment;++environment) {
        job.Application.Environment.push_back(
                std::pair<std::string,std::string>(
                        (std::string)environment["adl:Name"],
                        (std::string)environment["adl:Value"]));
      }
      for(XMLNode preexecutable = application["adl:PreExecutable"];
          (bool)preexecutable;++preexecutable) {
        ExecutableType exec;
        if(!ParseExecutable(preexecutable, exec, dialect, logger)) {
          jobdescs.clear();
          return false;
        }
        job.Application.PreExecutable.push_back(exec);
      }
      for(XMLNode postexecutable = application["adl:PostExecutable"];
          (bool)postexecutable;++postexecutable) {
        ExecutableType exec;
        if(!ParseExecutable(postexecutable, exec, dialect, logger)) {
          jobdescs.clear();
          return false;
        }
        job.Application.PostExecutable.push_back(exec);
      }
      job.Application.LogDir = (std::string)application["LoggingDirectory"];
      for(XMLNode logging = application["adl:RemoteLogging"];
                                (bool)logging;++logging) {
        if((std::string)logging["adl:ServiceType"] != "SGAS") {
          logger.msg(ERROR, "[ADLParser] Only SGAS ServiceType for RemoteLogging is supported yet.");
          jobdescs.clear();
          return false;
        }
        URL surl((std::string)logging["adl:URL"]);
        if(!surl) {
          logger.msg(ERROR, "[ADLParser] Unsupported URL %s for RemoteLogging.",(std::string)logging["adl:URL"]);
          jobdescs.clear();
          return false;
        }
        if(!ParseOptional(logging,logger)) {
          jobdescs.clear();
          return false;
        }
      }
      XMLNode expire =  application["adl:ExpirationTime"];
      if((bool)expire) {
        job.Application.ExpiryTime = (std::string)expire;
        // TODO: check validity
        if(!ParseOptional(expire,logger)) {
          jobdescs.clear();
          return false;
        }
      }
      XMLNode wipe =  application["adl:WipeTime"];
      if((bool)wipe) {
        job.Resources.SessionLifeTime = (std::string)wipe;
        // TODO: check validity
        if(!ParseOptional(wipe,logger)) {
          jobdescs.clear();
          return false;
        }
      }
      for(XMLNode notify = application["adl:Notification"];
                               (bool)notify;++notify) {
        // TODO: check and pack
        if((std::string)notify["adl:Protocol"] != "email") {
          logger.msg(ERROR, "[ADLParser] Only email Prorocol for Notification is supported yet.");
          jobdescs.clear();
          return false;
        }
        std::string state = notify["adl:OnState"]; // TODO: convert from EMI ES states
        for(XMLNode rcpt = notify["adl:Recipient"];(bool)rcpt;++rcpt) {
          NotificationType n;
          n.States.push_back(state);
          n.Email = (std::string)rcpt;
          job.Application.Notification.push_back(n);
        }
      }
    }
    if((bool)resources) {
      XMLNode os = resources["adl:OperatingSystem"];
      if((bool)os) {
        // TODO: convert from EMI ES types
        Software os_((std::string)os["adl:Family"],(std::string)os["adl:Name"],(std::string)os["adl:Version"]);
        job.Resources.OperatingSystem.add(os_);
      }
      XMLNode platform = resources["adl:Platform"];
      if((bool)platform) {
        // TODO: convert from EMI ES types
        job.Resources.Platform = (std::string)platform;
      }
      for(XMLNode rte = resources["adl:RuntimeEnvironment"];(bool)rte;++rte) {
        Software rte_("",(std::string)rte["adl:Name"],(std::string)rte["adl:Version"]);
        if((bool)rte["adl:Option"]) {
          logger.msg(ERROR, "[ADLParser] Option element inside RuntimeEnvironment is not supported yet.");
          jobdescs.clear();
          return false;
        }
        if(!ParseOptional(rte,logger)) {
          jobdescs.clear();
          return false;
        }
        job.Resources.RunTimeEnvironment.add(rte_);
      }
      if((bool)resources["adl:ParallelEnvironment"]) {
        logger.msg(ERROR, "[ADLParser] ParallelEnvironment is not supported yet.");
        jobdescs.clear();
        return false;
      }
      XMLNode coprocessor = resources["adl:Coprocessor"];
      if(((bool)coprocessor) && (!IsOptional(coprocessor))) {
        logger.msg(ERROR, "[ADLParser] Coprocessor is not supported yet.");
        jobdescs.clear();
        return false;
      }
      XMLNode netinfo = resources["adl:NetworkInfo"];
      if(((bool)netinfo) && (!IsOptional(netinfo))) {
        logger.msg(ERROR, "[ADLParser] NetworkInfo is not supported yet.");
        jobdescs.clear();
        return false;
      }
      XMLNode nodeaccess = resources["adl:NodeAccess"];
      if(nodeaccess) {
        std::string na = nodeaccess;
        if(na == "inbound") {
          job.Resources.NodeAccess = NAT_INBOUND;
        } else if(na == "outbound") {
          job.Resources.NodeAccess = NAT_OUTBOUND;
        } else if(na == "inoutbound") {
          job.Resources.NodeAccess = NAT_INOUTBOUND;
        } else {
          logger.msg(ERROR, "[ADLParser] NodeAccess value %s is not supported yet.",na);
          jobdescs.clear();
          return false;
        }
      }
      XMLNode slot = resources["adl:SlotRequirement"];
      if((bool)slot) {
        if(!stringto(slot["adl:NumberOfSlots"],job.Resources.SlotRequirement.NumberOfSlots.max)) {
          logger.msg(ERROR, "[ADLParser] Missing or wrong value in NumberOfSlots.");
          jobdescs.clear();
          return false;
        }
        if((bool)slot["adl:SlotsPerHost"]) {
          if(!stringto(slot["adl:SlotsPerHost"],job.Resources.SlotRequirement.ThreadsPerProcesses.max)) {
            logger.msg(ERROR, "[ADLParser] Missing or wrong value in NumberOfSlots.");
            jobdescs.clear();
            return false;
          }
          XMLNode use = slot["adl:SlotsPerHost"].Attribute("useNumberOfSlots");
          if((bool)use) {
            if(((std::string)use != "false") && ((std::string)use != "0")) {
              logger.msg(ERROR, "[ADLParser] For useNumberOfSlots of SlotsPerHost only false value is supported yet.");
              jobdescs.clear();
              return false;
            }
          }
          if((bool)slot["adl:ExclusiveExecution"]) {
            logger.msg(ERROR, "[ADLParser] ExclusiveExecution is not supported yet.");
            jobdescs.clear();
            return false;
          }
        }
      }
      XMLNode queue = resources["adl:QueueName"];
      if((bool)queue) {
        job.Resources.QueueName = (std::string)queue;
      }
      XMLNode memory;
      memory = resources["adl:IndividualPhysicalMemory"];
      if((bool)memory) {
        if(!stringto((std::string)memory,job.Resources.IndividualPhysicalMemory.max)) {
          logger.msg(ERROR, "[ADLParser] Missing or wrong value in IndividualPhysicalMemory.");
          jobdescs.clear();
          return false;
        }
      }
      memory = resources["adl:IndividualVirtualMemory"];
      if((bool)memory) {
        if(!stringto((std::string)memory,job.Resources.IndividualVirtualMemory.max)) {
          logger.msg(ERROR, "[ADLParser] Missing or wrong value in IndividualVirtualMemory.");
          jobdescs.clear();
          return false;
        }
      }
      memory = resources["adl:DiskSpaceRequirement"];
      if((bool)memory) {
        if(!stringto((std::string)memory,job.Resources.DiskSpaceRequirement.DiskSpace.min)) {
          logger.msg(ERROR, "[ADLParser] Missing or wrong value in DiskSpaceRequirement.");
          jobdescs.clear();
          return false;
        }
        job.Resources.DiskSpaceRequirement.DiskSpace.min /= 1024*1024; // TODO: round
      }
      if((bool)resources["adl:RemoteSessionAccess"]) {
        logger.msg(ERROR, "[ADLParser] RemoteSessionAccess is not supported yet.");
        jobdescs.clear();
        return false;
      }
      if((bool)resources["adl:Benchmark"]) {
        logger.msg(ERROR, "[ADLParser] Benchmark is not supported yet.");
        jobdescs.clear();
        return false;
      }
    }
    if((bool)staging) {
      bool clientpush = false;
      if(!ParseFlag(staging["adl:ClientDataPush"],logger,clientpush)) {
        jobdescs.clear();
        return false;
      }
      // TODO: store clientpush in JobDescription
      for(XMLNode input = staging["adl:InputFile"];(bool)input;++input) {
        FileType file;
        file.Name = (std::string)input["adl:Name"];
        if(file.Name.empty()) {
          logger.msg(ERROR, "[ADLParser] Missing or empty Name in InputFile.");
          jobdescs.clear();
          return false;
        }
        file.KeepData = false;
        std::string ex = input["adl:IsExecutable"];
        file.IsExecutable = !(ex.empty() || (ex == "false") || (ex == "0"));
        for(XMLNode source = input["adl:Source"];(bool)source;++source) {
          URL surl((std::string)source["adl:URI"]);
          if(!surl) {
            logger.msg(ERROR, "[ADLParser] Wrong URI specified in Source - %s.",(std::string)source["adl:URI"]);
            jobdescs.clear();
            return false;
          }
          if((bool)source["adl:DelegationID"]) {
            // TODO: support per source
            file.DelegationID = (std::string)source["adl:DelegationID"];
            //logger.msg(ERROR, "[ADLParser] DelegationID in Source is not supported yet.");
            //jobdescs.clear();
            //return false;
          }
          if((bool)source["adl:Option"]) {
            XMLNode option = source["adl:Option"];
            for(;(bool)option;++option) {
              surl.AddOption(option["adl:Name"],option["adl:Value"],true);
            };
          }
          file.Source.push_back(surl);
        }
        // TODO: FileSize
        job.Files.push_back(file);
      }
      for(XMLNode output = staging["adl:OutputFile"];(bool)output;++output) {
        FileType file;
        file.Name = (std::string)output["adl:Name"];
        if(file.Name.empty()) {
          logger.msg(ERROR, "[ADLParser] Missing or empty Name in OutputFile.");
          jobdescs.clear();
          return false;
        }
        file.KeepData = false;
        file.IsExecutable = false;
        for(XMLNode target = output["adl:Target"];(bool)target;++target) {
          URL turl((std::string)target["adl:URI"]);
          if(!turl) {
            logger.msg(ERROR, "[ADLParser] Wrong URI specified in Target - %s.",(std::string)target["adl:URI"]);
            jobdescs.clear();
            return false;
          }
          if((bool)target["adl:DelegationID"]) {
            // TODO: support per target
            file.DelegationID = (std::string)target["adl:DelegationID"];
            //logger.msg(ERROR, "[ADLParser] DelegationID in Target is not supported yet.");
            //jobdescs.clear();
            //return false;
          }
          if((bool)target["adl:Option"]) {
            XMLNode option = target["adl:Option"];
            for(;(bool)option;++option) {
              turl.AddOption(option["adl:Name"],option["adl:Value"],true);
            };
          }
          bool mandatory = false;
          if(!ParseFlag(target["adl:Mandatory"],logger,mandatory)) {
            jobdescs.clear();
            return false;
          }
          bool use_if_failure = false;
          bool use_if_cancel = false;
          bool use_if_success = true;
          if((!ParseFlag(target["adl:UseIfFailure"],logger,use_if_failure)) ||
             (!ParseFlag(target["adl:UseIfCancel"],logger,use_if_cancel)) ||
             (!ParseFlag(target["adl:UseIfSuccess"],logger,use_if_success))) {
            jobdescs.clear();
            return false;
          }
          if((bool)target["adl:CreationFlag"]) {
            logger.msg(ERROR, "[ADLParser] CreationFlag in Target is not supported yet.");
            jobdescs.clear();
            return false;
          }
          turl.AddOption("useiffailure",use_if_failure?"true":"false",true);
          turl.AddOption("useifcancel",use_if_cancel?"true":"false",true);
          turl.AddOption("useifsuccess",use_if_success?"true":"false",true);
          turl.AddOption("mandatory",mandatory?"true":"false",true);
          file.Target.push_back(turl);
        }
        job.Files.push_back(file);
      }
    }
    return true;
  }

  static void generateExecutableTypeElement(XMLNode element, const ExecutableType& exec) {
    element.NewChild("Path") = exec.Path;
    for(std::list<std::string>::const_iterator it = exec.Argument.begin();
        it != exec.Argument.end(); ++it) {
      element.NewChild("Argument") = *it;
    }
    if (exec.SuccessExitCode.first) {
      element.NewChild("FailIfExitCodeNotEqualTo") = tostring<int>(exec.SuccessExitCode.second);
    }
  }

  bool ADLParser::UnParse(const JobDescription& job, std::string& product, const std::string& language, const std::string& dialect) const {
    if (!IsLanguageSupported(language)) {
      return false;
    }

    NS ns;
    ns[""] = "http://www.eu-emi.eu/es/2010/12/adl";
    ns["emiestypes"] = "http://www.eu-emi.eu/es/2010/12/types";
    ns["nordugrid-adl"] = "http://www.nordugrid.org/es/2011/12/nordugrid-adl";

    // ActivityDescription
    XMLNode description(ns, "ActivityDescription");
    XMLNode identification = description.NewChild("ActivityIdentification");
    XMLNode application = description.NewChild("Application");
    XMLNode resources = description.NewChild("Resources");
    XMLNode staging = description.NewChild("DataStaging");

    // ActivityIdentification
    if(!job.Identification.JobName.empty()) identification.NewChild("Name") = job.Identification.JobName;
    if(!job.Identification.Description.empty()) identification.NewChild("Description") = job.Identification.Description;
    if(!job.Identification.Type.empty()) identification.NewChild("Type") = job.Identification.Type;
    for (std::list<std::string>::const_iterator it = job.Identification.Annotation.begin();
         it != job.Identification.Annotation.end(); it++) {
      identification.NewChild("Annotation") = *it;
    }
    // ARC extension: ActivityOldID
    for (std::list<std::string>::const_iterator it = job.Identification.ActivityOldID.begin();
         it != job.Identification.ActivityOldID.end(); ++it) {
      identification.NewChild("nordugrid-adl:ActivityOldID") = *it;
    }

    // Application
    if (!job.Application.Executable.Path.empty()) {
      generateExecutableTypeElement(application.NewChild("Executable"), job.Application.Executable);
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
    for(std::list<ExecutableType>::const_iterator it = job.Application.PreExecutable.begin();
        it != job.Application.PreExecutable.end(); ++it) {
      if (!it->Path.empty()) {
        generateExecutableTypeElement(application.NewChild("PreExecutable"), *it);
      }
    }
    for(std::list<ExecutableType>::const_iterator it = job.Application.PostExecutable.begin();
        it != job.Application.PostExecutable.end(); ++it) {
      if (!it->Path.empty()) {
        generateExecutableTypeElement(application.NewChild("PostExecutable"), *it);
      }
    }
    if(!job.Application.LogDir.empty()) application.NewChild("LoggingDirectory") = job.Application.LogDir;
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
      default: break;
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
          // It is not correct to do job description transformation 
          // in parser. Parser should be dumb. Other solution is needed.
          if(u->Protocol() == "file") continue;
          XMLNode source = file.NewChild("Source");
          source.NewChild("URI") = u->str();
          const std::map<std::string,std::string>& options = u->Options();
          for(std::map<std::string,std::string>::const_iterator o = options.begin();
                              o != options.end();++o) {
            XMLNode option = source.NewChild("Option");
            option.NewChild("Name") = o->first;
            option.NewChild("Value") = o->second;
          }
          //source.NewChild("DelegationID") = ; TODO?
        }
        if(it->IsExecutable || it->Name == job.Application.Executable.Path) {
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
          const std::map<std::string,std::string>& options = u->Options();
          for(std::map<std::string,std::string>::const_iterator o = options.begin();
                              o != options.end();++o) {
            if(o->first == "mandatory") continue;
            if(o->first == "useiffailure") continue;
            if(o->first == "useifcancel") continue;
            if(o->first == "useifsuccess") continue;
            XMLNode option = target.NewChild("Option");
            option.NewChild("Name") = o->first;
            option.NewChild("Value") = o->second;
          }
          // target.NewChild("DelegationID") = ;
          target.NewChild("Mandatory") = u->Option("mandatory","false");
          // target.NewChild("CreationFlag") = ;
          target.NewChild("UseIfFailure") = u->Option("useiffailure","false");
          target.NewChild("UseIfCancel") = u->Option("useifcancel","false");
          target.NewChild("UseIfSuccess") = u->Option("useifsuccess","true");
        }
      }
    }

    description.GetDoc(product, true);

    return true;
  }

} // namespace Arc
