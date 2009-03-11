#include <string>
#include <vector>
#include <sstream>
#include <time.h>
#include <arc/StringConv.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>
#include <arc/client/JobInnerRepresentation.h>
#include <arc/Logger.h>

#include "JSDLParser.h"


namespace Arc {

    // Optional name attribute checker.
    // When the node has "isoptional" attribute and it is "true", then add this node name to
    // the list of OptionalElement.  
    void Optional_attribute_check(Arc::XMLNode node, std::string path, Arc::JobInnerRepresentation& innerRepresentation, Arc::OptionalElementType& optional_element, std::string value=""){
         if (node.AttributesSize() > 0 && lower((std::string)node.Attribute("isoptional")) == "true"){
            optional_element.Name = node.Name();
            optional_element.Path = path;
            optional_element.Value = value;
            innerRepresentation.OptionalElement.push_back(optional_element);
         }
    }

    bool JSDLParser::parse( Arc::JobInnerRepresentation& innerRepresentation, const std::string source ) {
        //Source checking
        Arc::XMLNode node(source);
        if ( node.Size() == 0 ){
            logger.msg(DEBUG, "[JSDLParser] Wrong XML structure! ");
            return false;
        }
        if ( !Element_Search(node, false ) ) return false;

        // The source parsing start now.
        Arc::NS nsList;
        nsList.insert(std::pair<std::string, std::string>("jsdl","http://schemas.ggf.org/jsdl/2005/11/jsdl"));
        //TODO: wath is the new GIN namespace  (it is now temporary namespace)      
        //nsList.insert(std::pair<std::string, std::string>("gin","http://"));

        //Meta
        Arc::XMLNode meta;
        Arc::OptionalElementType optional;

        meta = node["Meta"]["Author"];
        if ( bool(meta) && (std::string)(meta) != "" ){
           Optional_attribute_check(meta, "//Meta/Author", innerRepresentation,optional);
           innerRepresentation.Author = (std::string)(meta);
        }
        meta.Destroy();

        meta = node["Meta"]["DocumentExpiration"];
        if ( bool(meta) && (std::string)(meta) != "" ){
           Optional_attribute_check(meta, "//Meta/DocumentExpiration", innerRepresentation,optional);
           Time time((std::string)(meta));
           innerRepresentation.DocumentExpiration = time;
        }
        meta.Destroy();

        meta = node["Meta"]["Rank"];
        if ( bool(meta) && (std::string)(meta) != "" ){
           Optional_attribute_check(meta, "//Meta/Rank", innerRepresentation,optional);
           innerRepresentation.Rank = (std::string)(meta);
        }
        meta.Destroy();

        meta = node["Meta"]["FuzzyRank"];
        if ( bool(meta) && (std::string)(meta) != "" ){
           Optional_attribute_check(meta, "//Meta/FuzzyRank", innerRepresentation,optional);
           if ( (std::string)(meta) == "true" ) {
                innerRepresentation.FuzzyRank = true;
           }
           else if ( (std::string)(meta) == "false" ) {
                innerRepresentation.FuzzyRank = false;
           }
            else {
                logger.msg(DEBUG, "Invalid \"/Meta/FuzzyRank\" value: %s", (std::string)(meta));
                return false;
            }
        }
        meta.Destroy();

        //Application
        Arc::XMLNode application;
        application = node["JobDescription"]["Application"]["Executable"];
        if ( bool(application) && (std::string)(application) != "" ){
           Optional_attribute_check(application, "//JobDescription/Application/Executable", innerRepresentation,optional);
           innerRepresentation.Executable = (std::string)(application);
        }
        application.Destroy();

        application = node["JobDescription"]["Application"]["LogDir"];
        if ( bool(application) && (std::string)(application) != "" ){
           Optional_attribute_check(application, "//JobDescription/Application/LogDir", innerRepresentation,optional);
           innerRepresentation.LogDir = (std::string)(application);
        }
        application.Destroy();

        application = node["JobDescription"]["Application"]["Argument"];
        for (int i=0; bool(application[i])&& (std::string)(application[i]) != ""; i++) {
           Optional_attribute_check(application[i], "//JobDescription/Application/Argument", innerRepresentation,optional, (std::string)(application[i]));
            innerRepresentation.Argument.push_back( (std::string)(application[i]) );
        }
        application.Destroy();

        application = node["JobDescription"]["Application"]["Input"];
        if ( bool(application) && (std::string)(application) != "" ){
           Optional_attribute_check(application, "//JobDescription/Application/Input", innerRepresentation,optional);
           innerRepresentation.Input = (std::string)(application);
        }
        application.Destroy();

        application = node["JobDescription"]["Application"]["Output"];
        if ( bool(application) && (std::string)(application) != "" ){
           Optional_attribute_check(application, "//JobDescription/Application/Output", innerRepresentation,optional);
           innerRepresentation.Output = (std::string)(application);
        }
        application.Destroy();

        application = node["JobDescription"]["Application"]["Error"];
        if ( bool(application) && (std::string)(application) != "" ){
           Optional_attribute_check(application, "//JobDescription/Application/Error", innerRepresentation,optional);
           innerRepresentation.Error = (std::string)(application);
        }
        application.Destroy();

        application = node["JobDescription"]["Application"]["RemoteLogging"];
        if ( bool(application) && (std::string)(application) != "" ){
           Optional_attribute_check(application, "//JobDescription/Application/RemoteLogging", innerRepresentation,optional);
           URL url((std::string)(application));
           innerRepresentation.RemoteLogging = url;
        }
        application.Destroy();

        application = node["JobDescription"]["Application"]["Environment"];
        for (int i=0; bool(application[i])&& (std::string)(application[i]) != ""; i++) {
           Optional_attribute_check(application[i], "//JobDescription/Application/Environment", innerRepresentation,optional, (std::string)(application[i]));
            Arc::EnvironmentType env;
            env.name_attribute = (std::string)(application[i].Attribute("name"));
            env.value = (std::string)(application[i]);
            innerRepresentation.Environment.push_back( env );
        }
        application.Destroy();

        application = node["JobDescription"]["Application"]["LRMSReRun"];
        if ( bool(application) && (std::string)(application) != "" ){
           Optional_attribute_check(application, "//JobDescription/Application/LRMSReRun", innerRepresentation,optional);
           innerRepresentation.LRMSReRun = stringtoi(application);
        }
        application.Destroy();

        application = node["JobDescription"]["Application"]["Prologue"];
        if ( bool(application) && (std::string)(application) != "" ){
           Optional_attribute_check(application,"//JobDescription/Application/Prologue", innerRepresentation,optional);
           std::string source_prologue = (std::string)(application);
           std::vector<std::string> parts;
           tokenize(source_prologue, parts);
           if ( !parts.empty() ) {
              Arc::XLogueType _prologue;
              _prologue.Name = parts[0];
              for (std::vector<std::string>::const_iterator it = ++parts.begin(); it != parts.end(); it++) {
                  _prologue.Arguments.push_back( *it );
              }
              innerRepresentation.Prologue = _prologue;
           }
        }
        application.Destroy();

        application = node["JobDescription"]["Application"]["Epilogue"];
        if ( bool(application) && (std::string)(application) != "" ){
           Optional_attribute_check(application, "//JobDescription/Application/Epilogue", innerRepresentation,optional);
           std::string source_epilogue = (std::string)(application);
           std::vector<std::string> parts;
           tokenize(source_epilogue, parts);
           if ( !parts.empty() ) {
              Arc::XLogueType _epilogue;
              _epilogue.Name = parts[0];
              for (std::vector<std::string>::const_iterator it = ++parts.begin(); it != parts.end(); it++) {
                  _epilogue.Arguments.push_back( *it );
              }
              innerRepresentation.Epilogue = _epilogue;
           }
        }
        application.Destroy();

        application = node["JobDescription"]["Application"]["SessionLifeTime"];
        if ( bool(application) && (std::string)(application) != "" ){
           Optional_attribute_check(application, "//JobDescription/Application/SessionLifeTime", innerRepresentation,optional);
           Period time((std::string)(application));
           innerRepresentation.SessionLifeTime = time;
        }
        application.Destroy();

        application = node["JobDescription"]["Application"]["AccessControl"];
        if ( bool(application) ){
           Optional_attribute_check(application, "//JobDescription/Application/AccessControl", innerRepresentation,optional);
           ((Arc::XMLNode)(application)).Child(0).New(innerRepresentation.AccessControl);
        }
        application.Destroy();

        application = node["JobDescription"]["Application"]["ProcessingStartTime"];
        if ( bool(application) && (std::string)(application) != "" ){
          Optional_attribute_check(application, "//JobDescription/Application/ProcessingStartTime", innerRepresentation,optional);
           Time stime((std::string)application);
           innerRepresentation.ProcessingStartTime = stime;
        }
        application.Destroy();

        application = node["JobDescription"]["Application"]["Notification"];
        for (int i=0; bool(application[i]); i++) {
           Optional_attribute_check(application[i], "//JobDescription/Application/Notification", innerRepresentation,optional);
            Arc::NotificationType ntype;
            Arc::XMLNodeList address = (application[i]).XPathLookup( (std::string) "//Address", nsList);
            for (std::list<Arc::XMLNode>::iterator it_addr = address.begin(); it_addr != address.end(); it_addr++) {             
                ntype.Address.push_back( (std::string)(*it_addr) );
            }
            Arc::XMLNodeList state = (application[i]).XPathLookup( (std::string) "//State", nsList);
            for (std::list<Arc::XMLNode>::iterator it_state = state.begin(); it_state != state.end(); it_state++) {             
                ntype.State.push_back( (std::string)(*it_state) );
            }
            innerRepresentation.Notification.push_back( ntype );
        }
        application.Destroy();

        application = node["JobDescription"]["Application"]["CredentialService"];
        if ( bool(application) && (std::string)(application) != "" ){
           Optional_attribute_check(application, "//JobDescription/Application/CredentialService", innerRepresentation,optional);
           URL curl((std::string)application);
           innerRepresentation.CredentialService = curl;
        }
        application.Destroy();

        application = node["JobDescription"]["Application"]["Join"];
        if ( bool(application) && Arc::upper((std::string)application) == "TRUE" ){
           Optional_attribute_check(application, "//JobDescription/Application/Join", innerRepresentation,optional);
           innerRepresentation.Join = true;
        }
        application.Destroy();

        //JobIdentification
        Arc::XMLNode jobidentification;
        jobidentification = node["JobDescription"]["JobIdentification"]["JobName"];
        if ( bool(jobidentification) && (std::string)(jobidentification) != "" ){
           Optional_attribute_check(jobidentification, "//JobDescription/JobIdentification/JobName", innerRepresentation,optional);
           innerRepresentation.JobName = (std::string)(jobidentification);
        }
        jobidentification.Destroy();

        jobidentification = node["JobDescription"]["JobIdentification"]["Description"];
        if ( bool(jobidentification) && (std::string)(jobidentification) != "" ){
           Optional_attribute_check(jobidentification, "//JobDescription/JobIdentification/Description", innerRepresentation,optional);
           innerRepresentation.Description = (std::string)(jobidentification);
        }
        jobidentification.Destroy();

        jobidentification = node["JobDescription"]["JobIdentification"]["JobProject"];
        if ( bool(jobidentification) && (std::string)(jobidentification) != "" ){
           Optional_attribute_check(jobidentification, "//JobDescription/JobIdentification/JobProject", innerRepresentation,optional);
           innerRepresentation.JobProject = (std::string)(jobidentification);
        }
        jobidentification.Destroy();

        jobidentification = node["JobDescription"]["JobIdentification"]["JobType"];
        if ( bool(jobidentification) && (std::string)(jobidentification) != "" ){
           Optional_attribute_check(jobidentification, "//JobDescription/JobIdentification/JobType", innerRepresentation,optional);
           innerRepresentation.JobType = (std::string)(jobidentification);
        }
        jobidentification.Destroy();

        jobidentification = node["JobDescription"]["JobIdentification"]["JobCategory"];
        if ( bool(jobidentification) && (std::string)(jobidentification) != "" ){
           Optional_attribute_check(jobidentification, "//JobDescription/JobIdentification/JobCategory", innerRepresentation,optional);
           innerRepresentation.JobCategory = (std::string)(jobidentification);
        }
        jobidentification.Destroy();

        jobidentification = node["JobDescription"]["JobIdentification"]["UserTag"];
        for (int i=0; bool(jobidentification[i])&& (std::string)(jobidentification[i]) != ""; i++) {
            Optional_attribute_check(jobidentification[i], "//JobDescription/JobIdentification/UserTag", innerRepresentation,optional, (std::string)jobidentification[i]);
            innerRepresentation.UserTag.push_back( (std::string)jobidentification[i]);
        }
        jobidentification.Destroy();

        //Resource
        Arc::XMLNode resource;
        resource = node["JobDescription"]["Resource"]["TotalCPUTime"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource,"//JobDescription/Resource/TotalCPUTime", innerRepresentation,optional);
           Period time((std::string)(resource));
           innerRepresentation.TotalCPUTime = time;
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["IndividualCPUTime"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/IndividualCPUTime", innerRepresentation,optional);
           Period time((std::string)(resource));
           innerRepresentation.IndividualCPUTime = time;
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["TotalWallTime"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/TotalWallTime", innerRepresentation,optional);
           Period _walltime((std::string)(resource));
           innerRepresentation.TotalWallTime = _walltime;
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["IndividualWallTime"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/IndividualWallTime", innerRepresentation,optional);
           Period _indwalltime((std::string)(resource));
           innerRepresentation.IndividualWallTime = _indwalltime;
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["ReferenceTime"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/ReferenceTime", innerRepresentation,optional);
           Arc::ReferenceTimeType _reference;
           _reference.value = (std::string)(resource);
           _reference.benchmark_attribute = (std::string)((Arc::XMLNode)(resource).Attribute("benchmark"));
           _reference.value_attribute = (std::string)((Arc::XMLNode)(resource).Attribute("value"));
           innerRepresentation.ReferenceTime = _reference;
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["ExclusiveExecution"];
        if ( bool(resource) && (std::string)(resource) != "" ){
            Optional_attribute_check(resource, "//JobDescription/Resource/ExclusiveExecution", innerRepresentation,optional);
            Arc::ReferenceTimeType _reference;
            if ( Arc::upper((std::string)(resource)) == "TRUE" ) {
                innerRepresentation.ExclusiveExecution = true;
            }
            else if ( Arc::upper((std::string)(resource)) == "FALSE" ) {
                innerRepresentation.ExclusiveExecution = false;
            }
            else {
                logger.msg(DEBUG, "Invalid \"/JobDescription/Resource/ExclusiveExecution\" value: %s", (std::string)(resource));
                return false;
            }
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["NetworkInfo"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/NetworkInfo", innerRepresentation,optional);
           innerRepresentation.NetworkInfo = (std::string)(resource);
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["OSFamily"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/OSFamily", innerRepresentation,optional);
           innerRepresentation.OSFamily = (std::string)(resource);
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["OSName"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/OSName", innerRepresentation,optional);
           innerRepresentation.OSName = (std::string)(resource);
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["OSVersion"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/OSVersion", innerRepresentation,optional);
           innerRepresentation.OSVersion = (std::string)(resource);
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["Platform"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/Platform", innerRepresentation,optional);
           innerRepresentation.Platform = (std::string)(resource);
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["IndividualPhysicalMemory"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/IndividualPhysicalMemory", innerRepresentation,optional);
           innerRepresentation.IndividualPhysicalMemory = stringtoi(resource);
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["IndividualVirtualMemory"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/IndividualVirtualMemory", innerRepresentation,optional);
           innerRepresentation.IndividualVirtualMemory = stringtoi(resource);
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["DiskSpace"]["IndividualDiskSpace"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/DiskSpace/IndividualDiskSpace", innerRepresentation,optional);
           innerRepresentation.IndividualDiskSpace = stringtoi(resource);
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["DiskSpace"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/DiskSpace", innerRepresentation,optional);
           innerRepresentation.DiskSpace = stringtoi(resource);
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["DiskSpace"]["CacheDiskSpace"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/DiskSpace/CacheDiskSpace", innerRepresentation,optional);
           innerRepresentation.CacheDiskSpace = stringtoi(resource);
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["DiskSpace"]["SessionDiskSpace"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/DiskSpace/SessionDiskSpace", innerRepresentation,optional);
           innerRepresentation.SessionDiskSpace = stringtoi(resource);
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["CandidateTarget"]["Alias"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/CandidateTarget/Alias", innerRepresentation,optional);
           innerRepresentation.Alias = (std::string)(resource);
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["CandidateTarget"]["EndPointURL"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/CandidateTarget/EndPointURL", innerRepresentation,optional);
           URL url((std::string)(resource));
           innerRepresentation.EndPointURL = url;
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["CandidateTarget"]["QueueName"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/CandidateTarget/QueueName", innerRepresentation,optional);
           innerRepresentation.QueueName = (std::string)(resource);
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["Location"]["Country"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/Location/Country", innerRepresentation,optional);
           innerRepresentation.Country = (std::string)(resource);
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["Location"]["Place"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/Location/Place", innerRepresentation,optional);
           innerRepresentation.Place = (std::string)(resource);
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["Location"]["PostCode"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/Location/PostCode", innerRepresentation,optional);
           innerRepresentation.PostCode = (std::string)(resource);
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["Location"]["Latitude"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/Location/Latitude", innerRepresentation,optional);
           innerRepresentation.Latitude = (std::string)(resource);
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["Location"]["Longitude"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/Location/Longitude", innerRepresentation,optional);
           innerRepresentation.Longitude = (std::string)(resource);
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["CEType"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/CEType", innerRepresentation,optional);
           innerRepresentation.CEType = (std::string)(resource);
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["Slots"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/Slots", innerRepresentation,optional);
           innerRepresentation.Slots = stringtoi(resource);
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["Slots"]["ProcessPerHost"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/Slots/ProcessPerHost", innerRepresentation,optional);
           innerRepresentation.ProcessPerHost = stringtoi(resource);
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["Slots"]["NumberOfProcesses"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/Slots/NumberOfProcesses", innerRepresentation,optional);
           innerRepresentation.NumberOfProcesses = stringtoi(resource);
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["Slots"]["ThreadPerProcesses"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/Slots/ThreadPerProcesses", innerRepresentation,optional);
           innerRepresentation.ThreadPerProcesses = stringtoi(resource);
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["Slots"]["SPMDVariation"];
        if ( bool(resource) && (std::string)(resource) != "" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/Slots/SPMDVariation", innerRepresentation,optional);
          innerRepresentation.SPMDVariation = (std::string)(resource);
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["RunTimeEnvironment"];
        for (int i=0; bool(resource[i]); i++) {
//            Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/RunTimeEnvironment", innerRepresentation,optional, (std::string)(*it));
            Arc::RunTimeEnvironmentType _runtimeenv;
            if ( (bool)(resource[i]["Name"]) ){
                _runtimeenv.Name = (std::string)resource[i]["Name"];
                Optional_attribute_check(resource[i]["Name"], "//JobDescription/Resource/RunTimeEnvironment/Name", innerRepresentation,optional, (std::string)(resource[i]["Name"]));
            }
            else{
                logger.msg(DEBUG, "Not found  \"/JobDescription/Resource/RunTimeEnvironment/Name\" element");
                return false;
            }
            Arc::XMLNodeList version = resource[i].XPathLookup((std::string)"//Version", nsList);
            for (std::list<Arc::XMLNode>::const_iterator it_v = version.begin(); it_v!= version.end(); it_v++) {
                Optional_attribute_check(*it_v, "//JobDescription/Resource/RunTimeEnvironment/Version", innerRepresentation,optional, (std::string)(*it_v));
                _runtimeenv.Version.push_back( (std::string)(*it_v) );
            }
            innerRepresentation.RunTimeEnvironment.push_back( _runtimeenv );
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["Homogeneous"];
        if ( bool(resource) && (std::string)(resource) == "true" ){
           Optional_attribute_check(resource, "//JobDescription/Resource/Homogeneous", innerRepresentation,optional);
           innerRepresentation.Homogeneous = true;
        }
        else {
           Optional_attribute_check(resource, "//JobDescription/Resource/Homogeneous", innerRepresentation,optional);
           innerRepresentation.Homogeneous = false;
        }
        resource.Destroy();

        resource = node["JobDescription"]["Resource"]["NodeAccess"];
        if ( (bool)((Arc::XMLNode)(resource)["InBound"]) && 
             (std::string)((Arc::XMLNode)(resource)["InBound"]) == "true" )
           innerRepresentation.InBound = true;
        if ( (bool)((Arc::XMLNode)(resource)["OutBound"]) && 
             (std::string)((Arc::XMLNode)(resource)["OutBound"]) == "true" )
           innerRepresentation.OutBound = true;
        resource.Destroy();


        //DataStaging
        Arc::XMLNode datastaging;
        datastaging = node["JobDescription"]["DataStaging"]["File"];
        for (int i=0; bool(datastaging[i]); i++) {
//           Optional_attribute_check(datastaging[i], "//JobDescription/DataStaging/File", innerRepresentation,optional);
            Arc::FileType _file;
            if ( (bool)(datastaging[i]["Name"]) )
               _file.Name = (std::string)datastaging[i]["Name"];
            Arc::XMLNodeList source = datastaging[i].XPathLookup((std::string)"//Source", nsList);
            for (std::list<Arc::XMLNode>::const_iterator it_source = source.begin(); it_source!= source.end(); it_source++) {
                Arc::SourceType _source;
                if ( (bool)((*it_source)["URI"]) ) {
                   _source.URI = (std::string)(*it_source)["URI"];
                }
                if ( (bool)((*it_source)["Threads"])) {
                   _source.Threads = stringtoi((std::string)(*it_source)["Threads"]);
                }
                else {
                   _source.Threads = -1;
                }
                _file.Source.push_back( _source );
            }
            Arc::XMLNodeList target = datastaging[i].XPathLookup((std::string)"//Target", nsList);
            for (std::list<Arc::XMLNode>::const_iterator it_target = target.begin(); it_target!= target.end(); it_target++) {
                Arc::TargetType _target;
                if ( (bool)((*it_target)["URI"]) ) {
                   _target.URI = (std::string)(*it_target)["URI"];
                }
                if ( (bool)((*it_target)["Threads"])) {
                   _target.Threads = stringtoi((std::string)(*it_target)["Threads"]);
                }
                else {
                   _target.Threads = -1;
                }
                if ( (bool)((*it_target)["Mandatory"])) {
                    if ( (std::string)(*it_target)["Mandatory"] == "true" ) {
                        _target.Mandatory = true;
                    }
                    else if ( (std::string)(*it_target)["Mandatory"] == "false" ) {
                        _target.Mandatory = false;
                    }
                    else {
                        logger.msg(DEBUG, "Invalid \"/JobDescription/DataStaging/File/Target/Mandatory\" value: %s", (std::string)(*it_target)["Mandatory"]);
                    }
                }
                else {
                   _target.Mandatory = false;
                }
                if ( (bool)((*it_target)["NeededReplicas"])) {
                   _target.NeededReplicas = stringtoi((std::string)(*it_target)["NeededReplicas"]);
                }
                else {
                   _target.NeededReplicas = -1;
                }
                _file.Target.push_back( _target );
            }
            if ( (bool)(datastaging[i]["KeepData"])) {
                   if ( (std::string)datastaging[i]["KeepData"] == "true" ) {
                      _file.KeepData = true;
                   }
                   else if ( (std::string)datastaging[i]["KeepData"] == "false" ) {
                      _file.KeepData = false;
                   }
                   else {
                      logger.msg(DEBUG, "Invalid \"/JobDescription/DataStaging/File/KeepData\" value: %s", (std::string)datastaging[i]["KeepData"]);
                   }
            }
            else {
                   _file.KeepData = false;
            }
            if ( (bool)(datastaging[i]["IsExecutable"])) {
                if ( (std::string)datastaging[i]["IsExecutable"] == "true" ) {
                    _file.IsExecutable = true;
                }
                else if ( (std::string)datastaging[i]["IsExecutable"] == "false" ) {
                    _file.IsExecutable = false;
                }
                else {
                    logger.msg(DEBUG, "Invalid \"/JobDescription/DataStaging/File/IsExecutable\" value: %s", (std::string)datastaging[i]["IsExecutable"]);
                }
            }
            else {
                _file.IsExecutable = false;
            }
            if ( (bool)(datastaging[i]["DataIndexingService"])) {
                URL uri((std::string)(datastaging[i]["DataIndexingService"]));
                _file.DataIndexingService = uri;
            }
            if ( (bool)(datastaging[i]["DownloadToCache"])) {
                if ( (std::string)datastaging[i]["DownloadToCache"] == "true" ) {
                    _file.DownloadToCache = true;
                }
                else if ( (std::string)datastaging[i]["DownloadToCache"] == "false" ) {
                    _file.DownloadToCache = false;
                } else {
                    logger.msg(DEBUG, "Invalid \"/JobDescription/DataStaging/File/DownloadToCache\" value: %s", (std::string)datastaging[i]["DownloadToCache"]);
                }
            } else {
                _file.DownloadToCache = false;
            }
            innerRepresentation.File.push_back( _file );
        }
        datastaging.Destroy();

        datastaging = node["JobDescription"]["DataStaging"]["Directory"];
        for (int i=0; bool(datastaging[i]); i++) {
//           Optional_attribute_check(datastaging[i], "//JobDescription/DataStaging/Directory", innerRepresentation,optional);
            Arc::DirectoryType _directory;
            if ( (bool)(datastaging[i]["Name"]) )
               _directory.Name = (std::string)datastaging[i]["Name"];
            Arc::XMLNodeList source = datastaging[i].XPathLookup((std::string)"//Source", nsList);
            for (std::list<Arc::XMLNode>::const_iterator it_source = source.begin(); it_source!= source.end(); it_source++) {
                Arc::SourceType _source;
                if ( (bool)((*it_source)["URI"]) ) {
                   _source.URI = (std::string)(*it_source)["URI"];
                }
                if ( (bool)((*it_source)["Threads"])) {
                   _source.Threads = stringtoi((std::string)(*it_source)["Threads"]);
                }
                else {
                   _source.Threads = -1;
                }
                _directory.Source.push_back( _source );
            }
            Arc::XMLNodeList target = datastaging[i].XPathLookup((std::string)"//Target", nsList);
            for (std::list<Arc::XMLNode>::const_iterator it_target = target.begin(); it_target!= target.end(); it_target++) {
                Arc::TargetType _target;
                if ( (bool)((*it_target)["URI"]) ) {
                   _target.URI = (std::string)(*it_target)["URI"];
                }
                if ( (bool)((*it_target)["Threads"])) {
                   _target.Threads = stringtoi((std::string)(*it_target)["Threads"]);
                }
                else {
                   _target.Threads = -1;
                }
                if ( (bool)((*it_target)["Mandatory"])) {
                    if ( (std::string)(*it_target)["Mandatory"] == "true" ) {
                        _target.Mandatory = true;
                    } else if ( (std::string)(*it_target)["Mandatory"] == "false" ) {
                        _target.Mandatory = false;
                    } else {
                        logger.msg(DEBUG, "Invalid \"/JobDescription/DataStaging/Directory/Target/Mandatory\" value: %s",  (std::string)(*it_target)["Mandatory"]);
                    }
                }
                else {
                    _target.Mandatory = false;
                }
                if ( (bool)((*it_target)["NeededReplicas"])) {
                    _target.NeededReplicas = stringtoi((std::string)(*it_target)["NeededReplicas"]);
                } else {
                    _target.NeededReplicas = -1;
                }
                _directory.Target.push_back( _target );
            }
            if ( (bool)(datastaging[i]["KeepData"])) {
                if ( (std::string)datastaging[i]["KeepData"] == "true" ) {
                    _directory.KeepData = true;
                } else if ( (std::string)datastaging[i]["KeepData"] == "false" ) {
                    _directory.KeepData = false;
                } else {
                    logger.msg(DEBUG, "Invalid \"/JobDescription/DataStaging/Directory/KeepData\" value: %s", (std::string)datastaging[i]["KeepData"]);
                }
            }
            else {
                _directory.KeepData = false;
            }
            if ( (bool)(datastaging[i]["IsExecutable"])) {
                if ( (std::string)datastaging[i]["IsExecutable"] == "true" ) {
                    _directory.IsExecutable = true;
                } else if ( (std::string)datastaging[i]["IsExecutable"] == "false" ) {
                    _directory.IsExecutable = false;
                } else {
                    logger.msg(DEBUG, "Invalid \"/JobDescription/DataStaging/Directory/IsExecutable\" value: %s", (std::string)datastaging[i]["IsExecutable"]);
                }
            }
            else {
                _directory.IsExecutable = false;
            }
            if ( (bool)(datastaging[i]["DataIndexingService"])) {
                URL uri((std::string)(datastaging[i]["DataIndexingService"]));
                _directory.DataIndexingService = uri;
            }
            if ( (bool)(datastaging[i]["DownloadToCache"])) {
                if ( (std::string)datastaging[i]["DownloadToCache"] == "true" ) {
                    _directory.DownloadToCache = true;
                } else if ( (std::string)datastaging[i]["DownloadToCache"] == "false" ) {
                    _directory.DownloadToCache = false;
                } else {
                    logger.msg(DEBUG, "Invalid \"/JobDescription/DataStaging/Directory/DownloadToCache\" value: %s", (std::string)datastaging[i]["DownloadToCache"]);
                }
            }
            else {
                _directory.DownloadToCache = false;
            }
            innerRepresentation.Directory.push_back( _directory );
        }
        datastaging.Destroy();

        return true;
    }

    bool JSDLParser::getProduct( const Arc::JobInnerRepresentation& innerRepresentation, std::string& product ) const {
        Arc::XMLNode jobDescription;
        if ( innerRepresentation.getXML(jobDescription) ) {
            jobDescription.GetDoc( product, true );
            return true;
        } else {
            logger.msg(DEBUG, "[JSDLParser] Job inner representation's root element has not found.");
            return false;
        }
    }

} // namespace Arc
