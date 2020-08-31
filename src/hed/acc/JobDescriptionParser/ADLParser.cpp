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
#include <arc/compute/JobDescription.h>

#include "XMLNodeRecover.h"
#include "ADLParser.h"

namespace Arc {

  static int bytes_to_mb(unsigned long long int value) {
    if(value == 0) return 0;
    return (int)((value - 1) / (1024*1024) + 1);
  }

  static unsigned long long int mb_to_bytes(int value) {
    return (unsigned long long int)(1024ull * 1024ull * (unsigned int)value);
  }

  /// \mapname ADL EMI ADL
  /// The libarccompute library has almost full support for EMI Activity
  /// Description Language (ADL) v1.16, it is described in the EMI Execution
  /// Service (ES) specification
  /// (<a href="https://twiki.cern.ch/twiki/pub/EMI/EmiExecutionService/EMI-ES-Specification_v1.16.odt">EMI-ES-Specification_v1.16.odt</a>).<br/>
  /// Currently the ADL parser is not able to parse: <br/>
  /// TODO
  ADLParser::ADLParser(PluginArgument* parg)
    : JobDescriptionParserPlugin(parg) {
    supportedLanguages.push_back("emies:adl");
  }

  ADLParser::~ADLParser() {}

  Plugin* ADLParser::Instance(PluginArgument *arg) {
    return new ADLParser(arg);
  }

  // EMI state             ARC state
  // ACCEPTED              ACCEPTED
  // PREPROCESSING         PREPARING
  //                       SUBMIT
  // PROCESSING            INLRMS
  // PROCESSING-ACCEPTING
  // PROCESSING-QUEUED
  // PROCESSING-RUNNING
  //                       CANCELING
  // POSTPROCESSING        FINISHING
  // TERMINAL              FINISHED
  //                       DELETED
  static std::string ADLStateToInternal(const std::string& s, bool optional, Logger& logger) {
   if(s == "ACCEPTED") {
      return "ACCEPTED";
    } else if(s == "PREPROCESSING") {
      return "PREPARING";
    } else if(s == "PROCESSING") {
      return "INLRMS";
    } else if(s == "PROCESSING-ACCEPTING") {
    } else if(s == "PROCESSING-QUEUED") {
    } else if(s == "PROCESSING-RUNNING") {
    } else if(s == "POSTPROCESSING") {
      return "FINISHING";
    } else if(s == "TERMINAL") {
      return "FINISHED";
    };
    logger.msg(optional?WARNING:ERROR, "[ADLParser] Unsupported EMI ES state %s.",s);
    return "";
  }

  static std::string InternalStateToADL(const std::string& s, bool optional, Logger& logger) {
    if(s == "ACCEPTED") {
    } else if(s == "") {
      return "ACCEPTED";
    } else if(s == "PREPARING") {
      return "PREPROCESSING";
    } else if(s == "SUBMIT") {
    } else if(s == "INLRMS") {
      return "PROCESSING";
    } else if(s == "CANCELING") {
    } else if(s == "FINISHING") {
      return "POSTPROCESSING";
    } else if(s == "FINISHED") {
      return "TERMINAL";
    } else if(s == "DELETED") {
    }
    logger.msg(optional?WARNING:ERROR, "[ADLParser] Unsupported internal state %s.",s);
    return "";
  }

  static bool ParseOptional(XMLNode el, bool& val, Logger& logger) {
    XMLNode optional = el.Attribute("optional");
    if(!optional) return true;
    if (strtobool((std::string)optional, val)) {
      return true;
    }
    logger.msg(ERROR, "[ADLParser] Optional for %s elements are not supported yet.", el.Name());
    return false;
  }

  static bool ParseFlag(XMLNode el, bool& val, Logger& logger) {
    if(!el) return true;
    if (strtobool((std::string)el, val)) {
      return true;
    }
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

  JobDescriptionParserPluginResult ADLParser::Parse(const std::string& source, std::list<JobDescription>& jobdescs, const std::string& language, const std::string& dialect) const {
    JobDescriptionParserPluginResult result(JobDescriptionParserPluginResult::WrongLanguage);
    if (language != "" && !IsLanguageSupported(language)) {
      return result;
    }

    XMLNodeRecover node(source);
    if (node.HasErrors()) {
      if (node.Name() != "ActivityDescription") {
        return result;
      }
      else {
        result.SetFailure();
        for (std::list<xmlErrorPtr>::const_iterator itErr = node.GetErrors().begin();
             itErr != node.GetErrors().end(); ++itErr) {
          JobDescriptionParsingError err;
          err.message = (**itErr).message;
          if (err.message[err.message.length()-1] == '\n') {
            err.message = err.message.substr(0, err.message.length()-1);
          }
          if ((**itErr).line > 0 && (**itErr).int2 > 0) {
            err.line_pos = std::pair<int, int>((**itErr).line, (**itErr).int2);
          }
          result.AddError(err);
        }
        return result;
      }
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
          IndividualCPUTime minOccurs=0
          TotalCPUTime minOccurs=0
          WallTime minOccurs=0
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
      return false;
    }

    JobDescription parsed_jobdescription;

    XMLNode identification = node["adl:ActivityIdentification"];
    XMLNode application = node["adl:Application"];
    XMLNode resources = node["adl:Resources"];
    XMLNode staging = node["adl:DataStaging"];

    if((bool)identification) {
      /// \mapattr ActivityIdentification.Name -> JobName
      parsed_jobdescription.Identification.JobName = (std::string)identification["adl:Name"];
      /// \mapattr ActivityIdentification.Description -> Description
      parsed_jobdescription.Identification.Description = (std::string)identification["adl:Description"];
      /// \mapattr ActivityIdentification.Type -> JobIdentificationType::Type
      parsed_jobdescription.Identification.Type = (std::string)identification["adl:Type"];
      /// \mapattr ActivityIdentification.Annotation -> Annotation
      for(XMLNode annotation = identification["adl:Annotation"];
                                      (bool)annotation;++annotation) {
        parsed_jobdescription.Identification.Annotation.push_back((std::string)annotation);
      }
      // ARC extension: ActivityOldID
      // TODO: Add note about this being a ARC extension.
      /// \mapattr ActivityIdentification.ActivityOldID -> ActivityOldID
      for(XMLNode activityoldid = identification["nordugrid-adl:ActivityOldID"];
                                      (bool)activityoldid;++activityoldid) {
        parsed_jobdescription.Identification.ActivityOldID.push_back((std::string)activityoldid);
      }
    }
    if((bool)application) {
      /// \mapattr Application.Executable.Path -> ExecutableType::Path
      /// \mapattr Application.Executable.Argument -> ExecutableType::Argument
      XMLNode executable = application["adl:Executable"];
      if(executable && !ParseExecutable(executable, parsed_jobdescription.Application.Executable, dialect, logger)) {
        return false;
      }
      // ARC extensions: Rerun, Priority
      // TODO: Add note about this being a ARC extension.
      /// \mapattr Application.Rerun -> Rerun
      if((bool)application["nordugrid-adl:Rerun"])
        parsed_jobdescription.Application.Rerun = stringtoi((std::string)application["nordugrid-adl:Rerun"]);
      /// \mapattr Application.Priority -> Priority
      if((bool)application["nordugrid-adl:Priority"]) {
        parsed_jobdescription.Application.Priority = stringtoi((std::string)application["nordugrid-adl:Priority"]);
        if (parsed_jobdescription.Application.Priority > 100) {
          logger.msg(VERBOSE, "[ADLParser] priority is too large - using max value 100");
          parsed_jobdescription.Application.Priority = 100;
        }
      }

      /// \mapattr Application.Input -> Input
      parsed_jobdescription.Application.Input = (std::string)application["adl:Input"];
      /// \mapattr Application.Output -> Output
      parsed_jobdescription.Application.Output = (std::string)application["adl:Output"];
      /// \mapattr Application.Error -> Error
      parsed_jobdescription.Application.Error = (std::string)application["adl:Error"];
      /// \mapattr Application.Environment -> Environment
      for(XMLNode environment = application["adl:Environment"];
                         (bool)environment;++environment) {
        parsed_jobdescription.Application.Environment.push_back(
                std::pair<std::string,std::string>(
                        (std::string)environment["adl:Name"],
                        (std::string)environment["adl:Value"]));
      }
      /// \mapattr Application.PreExecutable -> PreExecutable
      for(XMLNode preexecutable = application["adl:PreExecutable"];
          (bool)preexecutable;++preexecutable) {
        ExecutableType exec;
        if(!ParseExecutable(preexecutable, exec, dialect, logger)) {
          return false;
        }
        parsed_jobdescription.Application.PreExecutable.push_back(exec);
      }
      /// \mapattr Application.PostExecutable -> PostExecutable
      for(XMLNode postexecutable = application["adl:PostExecutable"];
          (bool)postexecutable;++postexecutable) {
        ExecutableType exec;
        if(!ParseExecutable(postexecutable, exec, dialect, logger)) {
          return false;
        }
        parsed_jobdescription.Application.PostExecutable.push_back(exec);
      }
      /// \mapattr Application.LoggingDirectory -> LogDir
      parsed_jobdescription.Application.LogDir = (std::string)application["LoggingDirectory"];
      for(XMLNode logging = application["adl:RemoteLogging"];
                                (bool)logging;++logging) {
        URL surl((std::string)logging["adl:URL"]);
        if(!surl) {
          logger.msg(ERROR, "[ADLParser] Unsupported URL %s for RemoteLogging.",(std::string)logging["adl:URL"]);
          return false;
        }
        parsed_jobdescription.Application.RemoteLogging.push_back(RemoteLoggingType());
        parsed_jobdescription.Application.RemoteLogging.back().ServiceType = (std::string)logging["adl:ServiceType"];
        parsed_jobdescription.Application.RemoteLogging.back().Location    = (std::string)logging["adl:URL"];
        if (!ParseOptional(logging, parsed_jobdescription.Application.RemoteLogging.back().optional, logger)) {
          return false;
        }
      }
      /// \mapattr Application.ExpirationTime -> ExpirationTime
      XMLNode expire =  application["adl:ExpirationTime"];
      if((bool)expire) {
        bool b;
        if(!ParseOptional(expire,b,logger)) {
          return false;
        }
        parsed_jobdescription.Application.ExpirationTime = (std::string)expire;
        if(parsed_jobdescription.Application.ExpirationTime.GetTime() == (time_t)(-1)) {
          logger.msg(b?WARNING:ERROR, "[ADLParser] Wrong time %s in ExpirationTime.",(std::string)expire);
          if(!b) {
            return false;
          }
        }
      }
      /// \mapattr Application.WipeTime -> SessionLifeTime
      XMLNode wipe =  application["adl:WipeTime"];
      if((bool)wipe) {
        parsed_jobdescription.Resources.SessionLifeTime = (std::string)wipe;
        // TODO: check validity. Do it after type is clarified.
        bool b;
        if(!ParseOptional(wipe,b,logger)) {
          return false;
        }
      }


      /// \mapattr Application.StartTime -> ProcessingStartTime
      XMLNode start = application["nordugrid-adl:StartTime"];
      if((bool)start) {
        parsed_jobdescription.Application.ProcessingStartTime = (std::string)start;
      }

      /// \mapattr Application.AccessControl -> AccessControl
      XMLNode acl = application["nordugrid-adl:AccessControl"];
      if((bool)acl) {
        std::string acl_str = (std::string)acl;
        XMLNode node(acl_str);
        if (!node) {
          logger.msg(ERROR, "[ADLParser] AccessControl isn't valid XML.");
          return false;
        }
        node.New(parsed_jobdescription.Application.AccessControl);
      }

      /// \mapattr Application.CredentialService -> CredentialService
      XMLNode service = application["nordugrid-adl:CredentialService"];
      for(;(bool)service;++service) {
        URL url((std::string)service);
        if (!url) {
          logger.msg(ERROR, "[ADLParser] CredentialService must contain valid URL.");
          return false;
        }
        parsed_jobdescription.Application.CredentialService.push_back(url);
      }

      /// \mapattr Application.DryRun -> DryRun
      XMLNode dry =  application["nordugrid-adl:DryRun"];
      if((bool)dry) {
        bool b;
        if(!ParseFlag(dry,b,logger)) {
          return false;
        }
        parsed_jobdescription.Application.DryRun = b;
      }

      // Notification
      //   *optional
      //   Protocol 1-1 [email]
      //   Recipient 1-
      //   OnState 0-
      for(XMLNode notify = application["adl:Notification"];
                               (bool)notify;++notify) {
        bool b;
        if(!ParseOptional(expire,b,logger)) {
          return false;
        }
        if((std::string)notify["adl:Protocol"] != "email") {
          if(!b) {
            logger.msg(ERROR, "[ADLParser] Only email Prorocol for Notification is supported yet.");
            return false;
          }
          logger.msg(WARNING, "[ADLParser] Only email Prorocol for Notification is supported yet.");
          continue;
        }
        NotificationType n;
        for(XMLNode onstate = notify["adl:OnState"];(bool)onstate;++onstate) {
          std::string s = ADLStateToInternal((std::string)onstate,b,logger);
          if(s.empty()) {
            if(!b) {
              return false;
            }
          }
          n.States.push_back(s);
        }
        for(XMLNode rcpt = notify["adl:Recipient"];(bool)rcpt;++rcpt) {
          n.Email = (std::string)rcpt;
          parsed_jobdescription.Application.Notification.push_back(n);
        }
      }
    }
    if((bool)resources) {
      /// \mapattr Resources.OperatingSystem -> OperatingSystem
      XMLNode os = resources["adl:OperatingSystem"];
      if((bool)os) {
        // TODO: convert from EMI ES types. So far they look similar.
        Software os_((std::string)os["adl:Family"],(std::string)os["adl:Name"],(std::string)os["adl:Version"]);
        parsed_jobdescription.Resources.OperatingSystem.add(os_, Software::EQUAL);
      }
      /// \mapattr Resources.Platform -> Platform
      XMLNode platform = resources["adl:Platform"];
      if((bool)platform) {
        // TODO: convert from EMI ES types. So far they look similar.
        parsed_jobdescription.Resources.Platform = (std::string)platform;
      }
      /// \mapattr Resources.RuntimeEnvironment -> RunTimeEnvironment
      for(XMLNode rte = resources["adl:RuntimeEnvironment"];(bool)rte;++rte) {
        Software rte_("",(std::string)rte["adl:Name"],(std::string)rte["adl:Version"]);
        for(XMLNode o = rte["adl:Option"];(bool)o;++o) {
          rte_.addOption((std::string)o);
        }
        bool b;
        if(!ParseOptional(rte,b,logger)) {
          return false;
        }
        parsed_jobdescription.Resources.RunTimeEnvironment.add(rte_, Software::EQUAL);
      }
      if((bool)resources["adl:ParallelEnvironment"]) {
        ParallelEnvironmentType& pe = parsed_jobdescription.Resources.ParallelEnvironment;
        XMLNode xpe = resources["adl:ParallelEnvironment"];
        /// \mapattr Resources.ParallelEnvironment.Type ->  ParallelEnvironmentType::Type
        if ((bool)xpe["adl:Type"]) {
          pe.Type = (std::string)xpe["adl:Type"];
        }
        /// \mapattr Resources.ParallelEnvironment.Version ->  ParallelEnvironmentType::Version
        if ((bool)xpe["adl:Version"]) {
          pe.Version = (std::string)xpe["adl:Version"];
        }
        /// \mapattr Resources.ParallelEnvironment.ProcessesPerSlot ->  ParallelEnvironmentType::ProcessesPerSlot
        if (((bool)xpe["adl:ProcessesPerSlot"]) && !stringto(xpe["adl:ProcessesPerSlot"], pe.ProcessesPerSlot)) {
          logger.msg(ERROR, "[ADLParser] Missing or wrong value in ProcessesPerSlot.");
          return false;
        }
        /// \mapattr Resources.ParallelEnvironment.ThreadsPerProcess ->  ParallelEnvironmentType::ThreadsPerProcess
        if (((bool)xpe["adl:ThreadsPerProcess"]) && !stringto(xpe["adl:ThreadsPerProcess"], pe.ThreadsPerProcess)) {
          logger.msg(ERROR, "[ADLParser] Missing or wrong value in ThreadsPerProcess.");
          return false;
        }
        /// \mapattr Resources.ParallelEnvironment.Option ->  ParallelEnvironmentType::Options
        for (XMLNode xOption = xpe["adl:Option"]; xOption; ++xOption) {
          if ((!(bool)xOption["adl:Name"]) || ((std::string)xOption["adl:Name"]).empty()) {
            logger.msg(ERROR, "[ADLParser] Missing Name element or value in ParallelEnvironment/Option element.");
            return false;
          }
          pe.Options.insert(std::make_pair(xOption["adl:Name"], xOption["adl:Value"]));
        }
      }
      /// \mapattr Resources.Coprocessor -> Coprocessor
      XMLNode coprocessor = resources["adl:Coprocessor"];
      if((bool)coprocessor && !((std::string)coprocessor).empty()) {
        parsed_jobdescription.Resources.Coprocessor = coprocessor;
        if (!ParseOptional(coprocessor, parsed_jobdescription.Resources.Coprocessor.optIn, logger)) {
          return false;
        }
      }
      /// \mapattr Resources.NetworkInfo -> NetworkInfo
      XMLNode netinfo = resources["adl:NetworkInfo"];
      if(((bool)netinfo)) {
        logger.msg(ERROR, "[ADLParser] NetworkInfo is not supported yet.");
        return false;
      }
      /// \mapattr Resources.NodeAccess -> NodeAccess
      XMLNode nodeaccess = resources["adl:NodeAccess"];
      if(nodeaccess) {
        std::string na = nodeaccess;
        if(na == "inbound") {
          parsed_jobdescription.Resources.NodeAccess = NAT_INBOUND;
        } else if(na == "outbound") {
          parsed_jobdescription.Resources.NodeAccess = NAT_OUTBOUND;
        } else if(na == "inoutbound") {
          parsed_jobdescription.Resources.NodeAccess = NAT_INOUTBOUND;
        } else {
          logger.msg(ERROR, "[ADLParser] NodeAccess value %s is not supported yet.",na);
          return false;
        }
      }
      XMLNode slot = resources["adl:SlotRequirement"];
      if((bool)slot) {
        /// \mapattr Resources.SlotRequirement.NumberOfSlots -> NumberOfSlots
        if(!stringto(slot["adl:NumberOfSlots"],parsed_jobdescription.Resources.SlotRequirement.NumberOfSlots)) {
          logger.msg(ERROR, "[ADLParser] Missing or wrong value in NumberOfSlots.");
          return false;
        }
        if((bool)slot["adl:SlotsPerHost"]) {
          XMLNode use = slot["adl:SlotsPerHost"].Attribute("useNumberOfSlots");
          if((bool)use && (((std::string)use == "true") || ((std::string)use == "1"))) {
            if (!(bool)slot["adl:NumberOfSlots"]) {
              logger.msg(ERROR, "[ADLParser] The NumberOfSlots element should be specified, when the value of useNumberOfSlots attribute of SlotsPerHost element is \"true\".");
              return false;
            }
            parsed_jobdescription.Resources.SlotRequirement.SlotsPerHost = parsed_jobdescription.Resources.SlotRequirement.NumberOfSlots;
          }
          /// \mapattr Resources.SlotRequirement.SlotsPerHost -> SlotsPerHost
          else if(!stringto(slot["adl:SlotsPerHost"],parsed_jobdescription.Resources.SlotRequirement.SlotsPerHost)) {
            logger.msg(ERROR, "[ADLParser] Missing or wrong value in SlotsPerHost.");
            return false;
          }
          /// \mapattr Resources.SlotRequirement.ExclusiveExecution -> ExclusiveExecution
          if((bool)slot["adl:ExclusiveExecution"]) {
            const std::string ee = slot["adl:ExclusiveExecution"];
            if ((ee == "true") || (ee == "1")) {
              parsed_jobdescription.Resources.SlotRequirement.ExclusiveExecution = SlotRequirementType::EE_TRUE;
            }
            else if ((ee == "false") || (ee == "0")) {
              parsed_jobdescription.Resources.SlotRequirement.ExclusiveExecution = SlotRequirementType::EE_FALSE;
            }
            else {
              parsed_jobdescription.Resources.SlotRequirement.ExclusiveExecution = SlotRequirementType::EE_DEFAULT;
            }
          }
        }
      }
      /// \mapattr Resources.QueueName -> QueueName
      XMLNode queue = resources["adl:QueueName"];
      if((bool)queue) {
        parsed_jobdescription.Resources.QueueName = (std::string)queue;
      }
      /// \mapattr Resources.IndividualPhysicalMemory -> IndividualPhysicalMemory
      XMLNode memory;
      memory = resources["adl:IndividualPhysicalMemory"];
      if((bool)memory) {
        unsigned long long int value = -1;
        if(!stringto((std::string)memory,value)) {
          logger.msg(ERROR, "[ADLParser] Missing or wrong value in IndividualPhysicalMemory.");
          return false;
        }
        parsed_jobdescription.Resources.IndividualPhysicalMemory.max = bytes_to_mb(value);
      }
      /// \mapattr Resources.IndividualVirtualMemory -> IndividualVirtualMemory
      memory = resources["adl:IndividualVirtualMemory"];
      if((bool)memory) {
        unsigned long long int value = -1;
        if(!stringto((std::string)memory,value)) {
          logger.msg(ERROR, "[ADLParser] Missing or wrong value in IndividualVirtualMemory.");
          return false;
        }
        parsed_jobdescription.Resources.IndividualVirtualMemory.max = bytes_to_mb(value);
      }
      /// \mapattr Resources.DiskSpaceRequirement -> DiskSpace
      memory = resources["adl:DiskSpaceRequirement"];
      if((bool)memory) {
        unsigned long long int v = 0;
        if((!stringto((std::string)memory,v)) || (v == 0)) {
          logger.msg(ERROR, "[ADLParser] Missing or wrong value in DiskSpaceRequirement.");
          return false;
        }
        parsed_jobdescription.Resources.DiskSpaceRequirement.DiskSpace.max = bytes_to_mb(v);
      }
      /// \mapattr Resources.RemoteSessionAccess -> SessionDirectoryAccess
      if((bool)resources["adl:RemoteSessionAccess"]) {
        bool v = false;
        if(!ParseFlag(resources["adl:RemoteSessionAccess"],v,logger)) {
          return false;
        }
        parsed_jobdescription.Resources.SessionDirectoryAccess = v?SDAM_RW:SDAM_NONE;
      }
      if((bool)resources["adl:Benchmark"]) {
        logger.msg(ERROR, "[ADLParser] Benchmark is not supported yet.");
        return false;
      }
      XMLNode time;
      time = resources["adl:IndividualCPUTime"];
      if((bool)time) {
        /// \mapattr Resources.IndividualCPUTime -> IndividualCPUTime
        if(!stringto((std::string)time,parsed_jobdescription.Resources.IndividualCPUTime.range.max)) {
          logger.msg(ERROR, "[ADLParser] Missing or wrong value in IndividualCPUTime.");
          return false;
        }
      }
      time = resources["adl:TotalCPUTime"];
      if((bool)time) {
        /// \mapattr Resources.TotalCPUTime -> TotalCPUTime
        if(!stringto((std::string)time,parsed_jobdescription.Resources.TotalCPUTime.range.max)) {
          logger.msg(ERROR, "[ADLParser] Missing or wrong value in TotalCPUTime.");
          return false;
        }
      }
      time = resources["adl:WallTime"];
      if((bool)time) {
        /// \mapattr Resources.WallTime -> IndividualWallTime
        /// TODO: Change to IndividualWallTime
        if(!stringto((std::string)time,parsed_jobdescription.Resources.TotalWallTime.range.max)) {
          logger.msg(ERROR, "[ADLParser] Missing or wrong value in WallTime.");
          return false;
        }
        parsed_jobdescription.Resources.IndividualWallTime = parsed_jobdescription.Resources.TotalWallTime;
      }
    }
    if((bool)staging) {
      bool clientpush = false;
      if(!ParseFlag(staging["adl:ClientDataPush"],clientpush,logger)) {
        return false;
      }
      if (clientpush) {
        parsed_jobdescription.OtherAttributes["emi-adl:ClientDataPush"] = "true";
        InputFileType file; // Also using unnamed file for that
        parsed_jobdescription.DataStaging.InputFiles.push_back(file);
      }
      for(XMLNode input = staging["adl:InputFile"];(bool)input;++input) {
        InputFileType file;
        file.Name = (std::string)input["adl:Name"];
        if(file.Name.empty()) {
          logger.msg(ERROR, "[ADLParser] Missing or empty Name in InputFile.");
          return false;
        }
        std::string ex = input["adl:IsExecutable"];
        file.IsExecutable = !(ex.empty() || (ex == "false") || (ex == "0"));
        if (!input["adl:Source"]) {
          file.Sources.push_back(URL(file.Name));
        } else {
          for(XMLNode source = input["adl:Source"];(bool)source;++source) {
            SourceType surl((std::string)source["adl:URI"]);
            if(!surl) {
              logger.msg(ERROR, "[ADLParser] Wrong URI specified in Source - %s.",(std::string)source["adl:URI"]);
              return false;
            }
            if((bool)source["adl:DelegationID"]) {
              surl.DelegationID = (std::string)source["adl:DelegationID"];
            }
            if((bool)source["adl:Option"]) {
              XMLNode option = source["adl:Option"];
              for(;(bool)option;++option) {
                surl.AddOption(option["adl:Name"],option["adl:Value"],true);
              };
            }
            file.Sources.push_back(surl);
          }
        }
        // TODO: FileSize and Checksum. Probably not useful for HTTP-like interfaces anyway.
        parsed_jobdescription.DataStaging.InputFiles.push_back(file);
      }
      for(XMLNode output = staging["adl:OutputFile"];(bool)output;++output) {
        OutputFileType file;
        file.Name = (std::string)output["adl:Name"];
        if(file.Name.empty()) {
          logger.msg(ERROR, "[ADLParser] Missing or empty Name in OutputFile.");
          return false;
        }
        for(XMLNode target = output["adl:Target"];(bool)target;++target) {
          TargetType turl((std::string)target["adl:URI"]);
          if(!turl) {
            logger.msg(ERROR, "[ADLParser] Wrong URI specified in Target - %s.",(std::string)target["adl:URI"]);
            return false;
          }
          if((bool)target["adl:DelegationID"]) {
            turl.DelegationID = (std::string)target["adl:DelegationID"];
          }
          if((bool)target["adl:Option"]) {
            XMLNode option = target["adl:Option"];
            for(;(bool)option;++option) {
              std::string opt_name((std::string)option["adl:Name"]);
              if(opt_name == "location") {
                URLLocation location_url(option["adl:Value"]);
                if(!location_url || location_url.Protocol() == "file") {
                  logger.msg(ERROR, "Location URI for file %s is invalid", file.Name);
                  return false;
                }
                turl.AddLocation(location_url);
              }
              else {
                turl.AddOption(opt_name,option["adl:Value"],true);
              }
            }
          }
          bool mandatory = false;
          if(!ParseFlag(target["adl:Mandatory"],mandatory,logger)) {
            return false;
          }
          if((!ParseFlag(target["adl:UseIfFailure"],turl.UseIfFailure,logger)) ||
             (!ParseFlag(target["adl:UseIfCancel"],turl.UseIfCancel,logger)) ||
             (!ParseFlag(target["adl:UseIfSuccess"],turl.UseIfSuccess,logger))) {
            return false;
          }
          if((bool)target["adl:CreationFlag"]) {
            std::string v = target["adl:CreationFlag"];
            if(v == "overwrite") { turl.CreationFlag = TargetType::CFE_OVERWRITE; }
            else if(v == "append") { turl.CreationFlag = TargetType::CFE_APPEND; }
            else if(v == "dontOverwrite") { turl.CreationFlag = TargetType::CFE_DONTOVERWRITE; }
            else {
              logger.msg(ERROR, "[ADLParser] CreationFlag value %s is not supported.",v);
              return false;
            };
          }
          turl.AddOption("mandatory",mandatory?"true":"false",true);
          file.Targets.push_back(turl);
        }
        parsed_jobdescription.DataStaging.OutputFiles.push_back(file);
      }
      if((bool)staging["nordugrid-adl:DelegationID"]) {
        parsed_jobdescription.DataStaging.DelegationID = (std::string)staging["nordugrid-adl:DelegationID"];
      }
    }

    jobdescs.push_back(parsed_jobdescription);
    return true;
  }

  static void generateExecutableTypeElement(XMLNode element, const ExecutableType& exec) {
    if(exec.Path.empty()) return;
    element.NewChild("Path") = exec.Path;
    for(std::list<std::string>::const_iterator it = exec.Argument.begin();
        it != exec.Argument.end(); ++it) {
      element.NewChild("Argument") = *it;
    }
    if (exec.SuccessExitCode.first) {
      element.NewChild("FailIfExitCodeNotEqualTo") = tostring<int>(exec.SuccessExitCode.second);
    }
  }

  JobDescriptionParserPluginResult ADLParser::Assemble(const JobDescription& job, std::string& product, const std::string& language, const std::string& dialect) const {
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
    /// \mapattr ActivityIdentification.JobName <- JobName
    if(!job.Identification.JobName.empty()) identification.NewChild("Name") = job.Identification.JobName;
    /// \mapattr ActivityIdentification.Description <- Description
    if(!job.Identification.Description.empty()) identification.NewChild("Description") = job.Identification.Description;
    /// \mapattr ActivityIdentification.Type <- JobIdentificationType::Type
    if(!job.Identification.Type.empty()) identification.NewChild("Type") = job.Identification.Type;
    /// \mapattr ActivityIdentification.Annotation <- Annotation
    for (std::list<std::string>::const_iterator it = job.Identification.Annotation.begin();
         it != job.Identification.Annotation.end(); it++) {
      identification.NewChild("Annotation") = *it;
    }
    // ARC extension: ActivityOldID
    /// \mapattr ActivityIdentification.ActivityOldID <- ActivityOldID
    /// TODO: Add mapping note that this element is an extention.
    for (std::list<std::string>::const_iterator it = job.Identification.ActivityOldID.begin();
         it != job.Identification.ActivityOldID.end(); ++it) {
      identification.NewChild("nordugrid-adl:ActivityOldID") = *it;
    }


    // Application
    /// \mapattr Application.Executable.Path <- ExecutableType::Path
    /// \mapattr Application.Executable.Argument <- ExecutableType::Argument
    generateExecutableTypeElement(application.NewChild("Executable"), job.Application.Executable);
    /// \mapattr Application.Input <- Input
    if(!job.Application.Input.empty()) application.NewChild("Input") = job.Application.Input;
    /// \mapattr Application.Output <- Output
    if(!job.Application.Output.empty()) application.NewChild("Output") = job.Application.Output;
    /// \mapattr Application.Error <- Error
    if(!job.Application.Error.empty()) application.NewChild("Error") = job.Application.Error;
    // ARC extensions: Rerun, Priority
    /// TODO: Add mapping note that these elements are an extention.
    /// \mapattr Application.Rerun -> Rerun
    if(job.Application.Rerun > -1) {
      XMLNode rerun = application.NewChild("nordugrid-adl:Rerun");
      rerun = tostring(job.Application.Rerun);
    }
    /// \mapattr Application.Priority -> Priority
    if(job.Application.Priority > -1) {
      XMLNode priority = application.NewChild("nordugrid-adl:Priority");
      priority = tostring(job.Application.Priority);
    }

    /// \mapattr Application.Environment <- Environment
    for(std::list< std::pair<std::string, std::string> >::const_iterator it =
        job.Application.Environment.begin(); it != job.Application.Environment.end(); it++) {
      XMLNode environment = application.NewChild("Environment");
      environment.NewChild("Name") = it->first;
      environment.NewChild("Value") = it->second;
    }
    /// \mapattr Application.PreExecutable <- PreExecutable
    for(std::list<ExecutableType>::const_iterator it = job.Application.PreExecutable.begin();
        it != job.Application.PreExecutable.end(); ++it) {
      generateExecutableTypeElement(application.NewChild("PreExecutable"), *it);
    }
    /// \mapattr Application.PostExecutable <- PostExecutable
    for(std::list<ExecutableType>::const_iterator it = job.Application.PostExecutable.begin();
        it != job.Application.PostExecutable.end(); ++it) {
      generateExecutableTypeElement(application.NewChild("PostExecutable"), *it);
    }
    /// \mapattr Application.LoggingDirectory <- LogDir
    if(!job.Application.LogDir.empty()) application.NewChild("nordugrid-adl:LoggingDirectory") = job.Application.LogDir;
    for (std::list<RemoteLoggingType>::const_iterator it = job.Application.RemoteLogging.begin();
         it != job.Application.RemoteLogging.end(); it++) {
      XMLNode logging = application.NewChild("RemoteLogging");
      logging.NewChild("ServiceType") = it->ServiceType;
      logging.NewChild("URL") = it->Location.fullstr();
      if(it->optional) logging.NewAttribute("optional") = "true";
    }
    /// \mapattr Application.ExpirationTime <- ExpirationTime
    if(job.Application.ExpirationTime > -1) {
      XMLNode expire = application.NewChild("ExpirationTime");
      expire = job.Application.ExpirationTime.str();
      //if() expire.NewAttribute("optional") = "true";
    }
    /// \mapattr Application.WipeTime <- SessionLifeTime
    if(job.Resources.SessionLifeTime > -1) {
      XMLNode wipe = application.NewChild("WipeTime");
      // TODO: ask for type change from dateTime to period.
      wipe = (std::string)job.Resources.SessionLifeTime;
      //if() wipe.NewAttribute("optional") = "true";
    }

    for (std::list<NotificationType>::const_iterator it = job.Application.Notification.begin();
         it != job.Application.Notification.end(); it++) {
      XMLNode notification = application.NewChild("Notification");
      notification.NewChild("Protocol") = "email";
      notification.NewChild("Recipient") = it->Email;
      for (std::list<std::string>::const_iterator s = it->States.begin();
                  s != it->States.end(); s++) {
        std::string st = InternalStateToADL(*s,false,logger);
        if(st.empty()) continue; // return false; TODO later
        notification.NewChild("OnState") = st;
      }
    }

    /// \mapattr Application.StartTime <- ProcessingStartTime 
    if(job.Application.ProcessingStartTime != -1) {
      XMLNode start = application.NewChild("nordugrid-adl:StartTime");
      start = job.Application.ProcessingStartTime.str(MDSTime);
    }

    /// \mapattr Application.AccessControl <- AccessControl
    if((bool)job.Application.AccessControl) {
      XMLNode acl = application.NewChild("nordugrid-adl:AccessControl");
      std::string acl_str;
      job.Application.AccessControl.GetXML(acl_str, false);
      acl = acl_str;
    }

    /// \mapattr Application.CredentialService <- CredentialService
    for(std::list<URL>::const_iterator url = job.Application.CredentialService.begin(); url != job.Application.CredentialService.end();++url) {
      XMLNode service = application.NewChild("nordugrid-adl:CredentialService");
      service = url->fullstr();;
    }

    /// \mapattr Application.DryRun <- DryRun
    if(job.Application.DryRun) {
      XMLNode dry = application.NewChild("nordugrid-adl:DryRun");
      dry = booltostr(job.Application.DryRun);
    }

    // Resources
    /// \mapattr Resources.OperatingSystem <- OperatingSystem
    for(std::list<Software>::const_iterator o = job.Resources.OperatingSystem.getSoftwareList().begin();
        o != job.Resources.OperatingSystem.getSoftwareList().end(); ++o) {
      XMLNode os = resources.NewChild("OperatingSystem");
      os.NewChild("Name") = o->getName();
      std::string fam = o->getFamily();
      if(!fam.empty()) os.NewChild("Family") = fam;
      os.NewChild("Version") = o->getVersion();
    }
    /// \mapattr Resources.Platform <- Platform
    if(!job.Resources.Platform.empty()) {
      // TODO: convert to EMI ES types. So far they look same.
      resources.NewChild("Platform") = job.Resources.Platform;
    }
    /// \mapattr Resources.RuntimeEnvironment <- RunTimeEnvironment
    for(std::list<Software>::const_iterator s =
                    job.Resources.RunTimeEnvironment.getSoftwareList().begin();
                    s != job.Resources.RunTimeEnvironment.getSoftwareList().end();++s) {
      XMLNode rte = resources.NewChild("RuntimeEnvironment");
      rte.NewChild("Name") = s->getName();
      rte.NewChild("Version") = s->getVersion();
      for(std::list<std::string>::const_iterator opt = s->getOptions().begin();
                      opt != s->getOptions().end(); ++opt) {
        rte.NewChild("Option") = *opt;
      }
      //if() rte.NewAttribute("optional") = "true";
    }
    {
      XMLNode xpe("<ParallelEnvironment/>");
      const ParallelEnvironmentType& pe = job.Resources.ParallelEnvironment;
      /// \mapattr Resources.ParallelEnvironment.Type <- ParallelEnvironmentType::Type
      if (!pe.Type.empty()) {
        xpe.NewChild("Type") = pe.Type;
      }
      /// \mapattr Resources.ParallelEnvironment.Version <- ParallelEnvironmentType::Version
      if (!pe.Version.empty()) {
        xpe.NewChild("Version") = pe.Version;
      }
      /// \mapattr Resources.ParallelEnvironment.ProcessesPerSlot <- ParallelEnvironmentType::ProcessesPerSlot
      if (pe.ProcessesPerSlot > -1) {
        xpe.NewChild("ProcessesPerSlot") = tostring(pe.ProcessesPerSlot);
      }
      /// \mapattr Resources.ParallelEnvironment.ThreadsPerProcess <- ParallelEnvironmentType::ThreadsPerProcess
      if (pe.ThreadsPerProcess > -1) {
        xpe.NewChild("ThreadsPerProcess") = tostring(pe.ThreadsPerProcess);
      }
      /// \mapattr Resources.ParallelEnvironment.Option <- ParallelEnvironmentType::Options
      for (std::multimap<std::string, std::string>::const_iterator it = pe.Options.begin();
           it != pe.Options.end(); ++it) {
        XMLNode xo = xpe.NewChild("Option");
        xo.NewChild("Name") = it->first;
        xo.NewChild("Value") = it->second;
      }
      if (xpe.Size() > 0) {
        resources.NewChild(xpe);
      }
    }
    /// \mapattr Resources.Coprocessor <- Coprocessor
    if(!((std::string)job.Resources.Coprocessor).empty()) {
      XMLNode coprocessor = resources.NewChild("Coprocessor");
      coprocessor = (std::string)job.Resources.Coprocessor;
      if(job.Resources.Coprocessor.optIn) coprocessor.NewAttribute("optional") = "true";
    }
    //TODO: check values. So far they look close.
    /// \mapattr Resources.NetworkInfo <- NetworkInfo
    if(!job.Resources.NetworkInfo.empty()) {
      resources.NewChild("NetworkInfo") = job.Resources.NetworkInfo;
    }
    /// \mapattr Resources.NodeAccess <- NodeAccess
    switch(job.Resources.NodeAccess) {
      case NAT_INBOUND: resources.NewChild("NodeAccess") = "inbound"; break;
      case NAT_OUTBOUND: resources.NewChild("NodeAccess") = "outbound"; break;
      case NAT_INOUTBOUND: resources.NewChild("NodeAccess") = "inoutbound"; break;
      default: break;
    }
    /// \mapattr Resources.IndividualPhysicalMemory <- IndividualPhysicalMemory
    if(job.Resources.IndividualPhysicalMemory.max != -1) {
      resources.NewChild("IndividualPhysicalMemory") = tostring(mb_to_bytes(job.Resources.IndividualPhysicalMemory.max));
    }
    /// \mapattr Resources.IndividualVirtualMemory <- IndividualVirtualMemory
    if(job.Resources.IndividualVirtualMemory.max != -1) {
      resources.NewChild("IndividualVirtualMemory") = tostring(mb_to_bytes(job.Resources.IndividualVirtualMemory.max));
    }
    /// \mapattr Resources.DiskSpaceRequirement <- DiskSpace
    if(job.Resources.DiskSpaceRequirement.DiskSpace.max > -1) {
      resources.NewChild("DiskSpaceRequirement") = tostring(mb_to_bytes(job.Resources.DiskSpaceRequirement.DiskSpace.max));
    }
    /// \mapattr Resources.RemoteSessionAccess <- SessionDirectoryAccess
    switch(job.Resources.SessionDirectoryAccess) {
      case SDAM_RW: resources.NewChild("RemoteSessionAccess") = "true"; break;
      case SDAM_RO: resources.NewChild("RemoteSessionAccess") = "true"; break; // approximately; TODO: Document in mapping.
      default: break;
    }
    //Benchmark
    //  BenchmarkType
    //  BenchmarkValue
    XMLNode slot = resources.NewChild("SlotRequirement");
    /// \mapattr Resources.SlotRequirement.NumberOfSlots <- NumberOfSlots
    if(job.Resources.SlotRequirement.NumberOfSlots > -1) {
      slot.NewChild("NumberOfSlots") = tostring(job.Resources.SlotRequirement.NumberOfSlots);
    }
    /// \mapattr Resources.SlotRequirement.SlotsPerHost <- SlotsPerHost
    if (job.Resources.SlotRequirement.SlotsPerHost > -1) {
      slot.NewChild("SlotsPerHost") = tostring(job.Resources.SlotRequirement.SlotsPerHost);
    }
    /// \mapattr Resources.SlotRequirement.ExclusiveExecution <- ExclusiveExecution
    switch(job.Resources.SlotRequirement.ExclusiveExecution) {
      case SlotRequirementType::EE_TRUE: slot.NewChild("ExclusiveExecution") = "true"; break;
      case SlotRequirementType::EE_FALSE: slot.NewChild("ExclusiveExecution") = "false"; break;
      default: break;
    }
    if(slot.Size() <= 0) slot.Destroy();
    /// \mapattr Resources.QueueName <- QueueName
    if(!job.Resources.QueueName.empty()) {;
      resources.NewChild("QueueName") = job.Resources.QueueName;
    }
    /// \mapattr Resources.IndividualCPUTime <- IndividualCPUTime
    if(job.Resources.IndividualCPUTime.range.max != -1) {
      resources.NewChild("IndividualCPUTime") = tostring(job.Resources.IndividualCPUTime.range.max);
    }
    /// \mapattr Resources.TotalCPUTime <- TotalCPUTime
    if(job.Resources.TotalCPUTime.range.max != -1) {
      resources.NewChild("TotalCPUTime") = tostring(job.Resources.TotalCPUTime.range.max);
    }
    /// \mapattr Resources.WallTime <- IndividualWallTime
    if(job.Resources.TotalWallTime.range.max != -1) {
      resources.NewChild("WallTime") = tostring(job.Resources.TotalWallTime.range.max);
    } else if(job.Resources.IndividualWallTime.range.max != -1) {
      resources.NewChild("WallTime") = tostring(job.Resources.IndividualWallTime.range.max);
    }

    // job.Resources.NetworkInfo
    // job.Resources.DiskSpaceRequirement.CacheDiskSpace
    // job.Resources.DiskSpaceRequirement.SessionDiskSpace
    // job.Resources.CEType

    // DataStaging

    // ClientDataPush
    {
      std::map<std::string, std::string>::const_iterator it = job.OtherAttributes.find("emi-adl:ClientDataPush");
      if (it != job.OtherAttributes.end() && it->second == "true") {
        staging.NewChild("ClientDataPush") = "true";
      } else {
        // Other way to do that is to ask for undefined InputFile
        for (std::list<InputFileType>::const_iterator it = job.DataStaging.InputFiles.begin();
             it != job.DataStaging.InputFiles.end(); ++it) {
          if(it->Name.empty()) {
            staging.NewChild("ClientDataPush") = "true";
            break;
          }
        }
      }
    }

    // InputFile
    for (std::list<InputFileType>::const_iterator it = job.DataStaging.InputFiles.begin();
         it != job.DataStaging.InputFiles.end(); ++it) {
      if(it->Name.empty()) { // Used to mark free stage in
        continue;
        //return false;
      }
      XMLNode file = staging.NewChild("InputFile");
      file.NewChild("Name") = it->Name;
      for(std::list<SourceType>::const_iterator u = it->Sources.begin();
          u != it->Sources.end(); ++u) {
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
        if (!u->DelegationID.empty()) {
          source.NewChild("DelegationID") = u->DelegationID;
        }
      }
      if(it->IsExecutable || it->Name == job.Application.Executable.Path) {
        file.NewChild("IsExecutable") = "true";
      }
      // it->FileSize
    }
    // OutputFile
    for (std::list<OutputFileType>::const_iterator it = job.DataStaging.OutputFiles.begin();
         it != job.DataStaging.OutputFiles.end(); ++it) {
      if(it->Name.empty()) { // mandatory
        return false;
      }
      XMLNode file = staging.NewChild("OutputFile");
      file.NewChild("Name") = it->Name;
      for(std::list<TargetType>::const_iterator u = it->Targets.begin();
                     u != it->Targets.end(); ++u) {
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
        const std::list<URLLocation>& locations = u->Locations();
        for(std::list<URLLocation>::const_iterator l = locations.begin();
            l != locations.end();++l) {
          XMLNode option = target.NewChild("Option");
          option.NewChild("Name") = "location";
          option.NewChild("Value") = l->fullstr();
        }
        if (!u->DelegationID.empty()) {
          target.NewChild("DelegationID") = u->DelegationID;
        }
        target.NewChild("Mandatory") = u->Option("mandatory","false");
        // target.NewChild("CreationFlag") = ;
        target.NewChild("UseIfFailure") = booltostr(u->UseIfFailure);
        target.NewChild("UseIfCancel") = booltostr(u->UseIfCancel);
        target.NewChild("UseIfSuccess") = booltostr(u->UseIfSuccess);
      }
    }
    if(!job.DataStaging.DelegationID.empty()) {
      staging.NewChild("nordugrid-adl:DelegationID") = job.DataStaging.DelegationID;
    }

    description.GetDoc(product, true);

    return true;
  }

} // namespace Arc
