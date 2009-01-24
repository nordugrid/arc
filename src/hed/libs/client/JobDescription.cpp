#include <cstring>
#include <algorithm>
#include <arc/StringConv.h>
#include "JobDescription.h"

namespace Arc {
    JobDescriptionError::JobDescriptionError(const std::string& what) : std::runtime_error(what){  }


    // Remove the whitespace characters from the begin and end of the string then return with this "naked" one
    std::string StringManipulator::trim( const std::string original_string ) const {
        std::string whitespaces (" \t\f\v\n\r");
        if (original_string.length() == 0) return original_string;
        unsigned long first = original_string.find_first_not_of( whitespaces );
        unsigned long last = original_string.find_last_not_of( whitespaces );
        if ( first == std::string::npos || last == std::string::npos ) return ""; 
        return original_string.substr( first, last-first+1 );
    }

    // Return the given string in lower case
    std::string StringManipulator::toLowerCase( const std::string original_string ) const {
        std::string retVal = original_string;
        std::transform( original_string.begin(), original_string.end(), retVal.begin(), ::tolower );
        return retVal;
    }

    // Split the given string by the given delimiter and return its parts
    std::vector<std::string> StringManipulator::split( const std::string original_string, const std::string delimiter ) const{
        std::vector<std::string> retVal;
        unsigned long start=0;
        unsigned long end;
        while ( ( end = original_string.find( delimiter, start ) ) != std::string::npos ) {
            retVal.push_back( original_string.substr( start, end-start ) );
            start = end + 1;
        }
        retVal.push_back( original_string.substr( start ) );
        return retVal;
    }

    // Set the sourceString variable of the JobDescriptionOrderer instance
    void JobDescriptionOrderer::setSource( const std::string source ) {
        sourceString = source;
    }

    // A template for the vector sorting function //
    template<class T> class CompareDesc {
        public:
            CompareDesc(){};
            virtual ~CompareDesc(){};
            bool operator()(const T& T1, const T& T2) const {
                return T1.priority > T2.priority;
            }
    }; // End of template Compare

    // Initializing of the candidates vector / collection //
    void JobDescriptionOrderer::determinizeCandidates( std::vector<Candidate>& candidates ) {

        Candidate candidate;

        // POSIX JSDL attributes
        //Set defaults//
        candidate.extensions.clear();
        candidate.pattern.clear();
        candidate.negative_pattern.clear();
        candidate.priority = DEFAULT_PRIORITY; //Default value
        //End of setting defaults//
        candidate.typeName = "POSIXJSDL";
        candidate.extensions.push_back("jsdl");
        candidate.extensions.push_back("xml");
        candidate.pattern.push_back("<?xml ");
        candidate.pattern.push_back("<!--");
        candidate.pattern.push_back("-->");
        candidate.pattern.push_back("<JobDefinition");
        candidate.pattern.push_back("<JobDescription");
        candidate.pattern.push_back("<JobIdentification");
        candidate.pattern.push_back("<POSIXApplication");
        candidate.pattern.push_back("<Description");
        candidate.pattern.push_back("<Executable");
        candidate.pattern.push_back("<Argument");
        candidate.pattern.push_back("<Input");
        candidate.pattern.push_back("<Output");
        candidate.pattern.push_back("<Error");
        
        //candidate.pattern.push_back("<?xml ");
        //candidate.pattern.push_back("<JobDescription");
        //Save entry
        candidates.push_back(candidate);
        // End of POSIX JSDL
/*
        // JSDL attributes
        //Set defaults//
        candidate.extensions.clear();
        candidate.pattern.clear();
        candidate.negative_pattern.clear();
        candidate.priority = DEFAULT_PRIORITY; //Default value
        //End of setting defaults//
        candidate.typeName = "JSDL";
        candidate.extensions.push_back("jsdl");
        candidate.extensions.push_back("xml");
        candidate.pattern.push_back("<?xml ");
        candidate.pattern.push_back("<!--");
        candidate.pattern.push_back("-->");
        candidate.pattern.push_back("<JobDefinition");
        candidate.pattern.push_back("<JobDescription");
        candidate.pattern.push_back("<Meta");
        candidate.pattern.push_back("<JobIdentification");
        candidate.pattern.push_back("<Application");
        candidate.pattern.push_back("<Description");
        candidate.pattern.push_back("<Executable");
        candidate.pattern.push_back("<LogDir");
        candidate.pattern.push_back("<Argument");
        candidate.pattern.push_back("<Input");
        candidate.pattern.push_back("<Output");
        candidate.pattern.push_back("<Error");
        candidate.pattern.push_back("<OSName");
        candidate.pattern.push_back("<File");
        candidate.pattern.push_back("<Directory");
        //Save entry  
        candidates.push_back(candidate);
        // End of JSDL
*/
        // RSL attributes
        //Set defaults//
        candidate.extensions.clear();
        candidate.pattern.clear();
        candidate.negative_pattern.clear();
        candidate.priority = DEFAULT_PRIORITY; //Default value
        //End of setting defaults//
        candidate.typeName = "XRSL";
        //  candidate.extensions.push_back("xrsl");
        candidate.pattern.push_back("&(");
        candidate.pattern.push_back("(*");
        candidate.pattern.push_back("*)");
        candidate.pattern.push_back("=");
        candidate.pattern.push_back("(executable");
        candidate.pattern.push_back("(arguments");
        candidate.pattern.push_back("(inputFiles");
        candidate.pattern.push_back("(outputFiles");
        candidate.pattern.push_back("(stdin");
        candidate.pattern.push_back("(stdout");
        candidate.pattern.push_back("(stderr");
        //Save entry  
        candidates.push_back(candidate);
        // End of RSL & xRSL

        //Setting up the JDL attributes
        //Set defaults//
        candidate.extensions.clear();
        candidate.pattern.clear();
        candidate.negative_pattern.clear();
        candidate.priority = DEFAULT_PRIORITY; //Default value
        //End of setting defaults//
        candidate.typeName = "JDL";
        candidate.pattern.push_back("//");
        candidate.pattern.push_back(";");
        candidate.pattern.push_back("{");
        candidate.pattern.push_back("=");
        //  candidate.extensions.push_back("jdl");
        //Save entry  
        candidates.push_back(candidate);
        // End of JDL

    } // End of initiateCandidates

    // Generate a list of the possible source format the order them by the possibility with the help of a scoring algorithm 
    std::vector<Candidate> JobDescriptionOrderer::getCandidateList() {

        std::vector<Candidate> candidates;
        determinizeCandidates( candidates );

        //Examining the filename's extension
        /*
        int pos_of_last_dot = filename.find_last_of('.');
        if ( pos_of_last_dot != std::string::npos ) {
            std::string extension = filename.substr( pos_of_last_dot + 1, filename.length() - pos_of_last_dot -1 );
     
            //To lowercase
            for ( std::string::iterator i = extension.begin(); i != extension.end(); ++i ) *i = tolower(*i);
     
            // Removed //
            // if ( VERBOSEX ) std::cerr << "[JobDescriptionOrderer]\t[COMMENT] Extension: " << extension << std::endl;
        
            for ( std::vector<Candidate>::iterator i = candidates.begin(); i != candidates.end(); i++ ) {
                for ( std::vector<std::string>::const_iterator j = (*i).extensions.begin(); j != (*i).extensions.end(); j++ ) {
                    if ( (*j).compare(extension) == 0 ) {
                        if ( VERBOSEX ) std::cerr << "[JobDescriptionOrderer]\tExact extension matching: \"" << extension << "\" to " << (*i).typeName << std::endl;
                        (*i).priority += EXACT_EXTENSION_MATCHING;
                    } else if ( extension.find(*j) != std::string::npos || (*j).find(extension) != std::string::npos ) {
                        if ( VERBOSEX ) std::cerr << "[JobDescriptionOrderer]\tPartial extension matching: \"" << extension << "\" to " << (*i).typeName << std::endl;
                        (*i).priority += PARTIAL_EXTENSION_MATCHING;
                    }
                }
            }
        } else { if ( VERBOSEX ) std::cerr << "[JobDescriptionOrderer]There is no extension... ascpect skipped" << std::endl; }

        // Read the file's content
        std::ifstream jobDescriptionFile;
        std::string jobDescription;
        std::string jobDescriptionWithoutWhitespaces;

        jobDescriptionFile.open( filename.c_str() );
        if ( !jobDescriptionFile ) {
            if ( DEBUGX ) std::cerr << "[JobDescriptionOrderer] Unable to open this file: " << filename << std::endl;
            return false;
        }
        std::string line;
        while(std::getline(jobDescriptionFile,line)) jobDescription += line + "\n";
        jobDescriptionFile.close();
        */

        StringManipulator sm;

        std::string jobDescription = sm.toLowerCase(sourceString);

        //Creating the whitespace-free version of the jobDescription
        std::string jobDescriptionWithoutWhitespaces = jobDescription;
        for ( int i = 0, j; i < jobDescriptionWithoutWhitespaces.length( ); i++ ) {
            if ( jobDescriptionWithoutWhitespaces[i] == ' ' || jobDescriptionWithoutWhitespaces[i] == '\n' || jobDescriptionWithoutWhitespaces[i] == '\t') {
                for ( j = i + 1; j < jobDescriptionWithoutWhitespaces.length( ); j++ ) {
                    if ( jobDescriptionWithoutWhitespaces[j] != ' ' && jobDescriptionWithoutWhitespaces[j] != '\n' && jobDescriptionWithoutWhitespaces[j] != '\t' ) break;
                }
                jobDescriptionWithoutWhitespaces = jobDescriptionWithoutWhitespaces.erase( i, (j - i) ) ;
            }
        }

        //Examining the file's content for patterns
        for ( std::vector<Candidate>::iterator i = candidates.begin(); i != candidates.end(); i++ ) {
            for ( std::vector<std::string>::const_iterator j = (*i).pattern.begin(); j != (*i).pattern.end(); j++ ) {
                if ( jobDescription.find( sm.toLowerCase(*j) ) != std::string::npos ) {
                    if ( VERBOSEX ) std::cerr << "[JobDescriptionOrderer]\tText pattern matching: \"" << *j << "\" to " << (*i).typeName << std::endl;
                    (*i).priority += TEXT_PATTERN_MATCHING;
                } else {
                    if ( jobDescriptionWithoutWhitespaces.find( sm.toLowerCase(*j) ) != std::string::npos ) {
                        if ( VERBOSEX ) std::cerr << "[JobDescriptionOrderer]\tText pattern matching (just with removed whitespaces) : \"" << *j << "\" to " << (*i).typeName << std::endl;
                        (*i).priority += TEXT_PATTERN_MATCHING_WITHOUT_WHITESPACES;
                    }
                }
            }
            for ( std::vector<std::string>::const_iterator j = (*i).negative_pattern.begin(); j != (*i).negative_pattern.end(); j++ ) {
                if ( jobDescription.find( sm.toLowerCase(*j) ) != std::string::npos ) {
                    if ( VERBOSEX ) std::cerr << "[JobDescriptionOrderer]\tNegative text pattern matching: \"" << *j << "\" to " << (*i).typeName << std::endl;
                    (*i).priority += NEGATIVE_TEXT_PATTERN_MATCHING;
                } else {
                    if ( jobDescriptionWithoutWhitespaces.find( sm.toLowerCase(*j) ) != std::string::npos ) {
                        if ( VERBOSEX ) std::cerr << "[JobDescriptionOrderer]\tNegative text pattern matching (just with removed whitespaces): \"" << *j << "\" to " << (*i).typeName << std::endl;
                        (*i).priority += NEGATIVE_TEXT_PATTERN_MATCHING_WITHOUT_WHITESPACES;
                    }
                }
            }
        }

        // Sort candidates by priority
        std::sort(candidates.begin(), candidates.end(), CompareDesc<Candidate>() );

        return candidates;
    }

    // JobDescription default constructor
    JobDescription::JobDescription() {
        sourceFormat = "";
        sourceString = "";
        innerRepresentation = NULL;
    }

    JobDescription::JobDescription(const JobDescription& desc) {
        sourceString = desc.sourceString;
        sourceFormat = desc.sourceFormat;
        if ( desc.innerRepresentation != NULL ){
           innerRepresentation = new Arc::JobInnerRepresentation(*desc.innerRepresentation);
        }
        else {
           innerRepresentation = NULL;
        }
    }

    JobDescription::~JobDescription() {
        delete innerRepresentation;
    }

    // Set the sourceString variable of the JobDescription instance and parse it with the right parser
    bool JobDescription::setSource( const std::string source ) {
        sourceFormat = "";
        sourceString = source;
        
        // Initialize the necessary variables
        resetJobTree();

        // Parsing the source string depending on the Orderer's decision
        //
        // Get the candidate list of formats in the proper order
        if ( sourceString.empty() || sourceString.length() == 0 ){ 
           std::cerr << "There is nothing in the source. Cannot generate any product." << std::endl;
           return false;
        }

        jdOrderer.setSource( sourceString );
        std::vector<Candidate> candidates = jdOrderer.getCandidateList();

        // Try to parse the input string in the right order until it once success
        // (If not then just reset the jobTree variable and see the next one)
        for (std::vector<Candidate>::const_iterator it = candidates.begin(); it < candidates.end(); it++) {
            if ( sm.toLowerCase( (*it).typeName ) == "xrsl" ) {
                if ( VERBOSEX ) std::cerr << "[JobDescription] Try to parse as XRSL" << std::endl;
                XRSLParser parser;
                if ( parser.parse( *innerRepresentation, sourceString ) ) {
                    sourceFormat = "xrsl";
                    break;
                }
                //else
                resetJobTree();
            } else if ( sm.toLowerCase( (*it).typeName ) == "posixjsdl" ) {
                if ( VERBOSEX ) std::cerr << "[JobDescription] Try to parse as POSIX JSDL" << std::endl;
                PosixJSDLParser parser;
                if ( parser.parse( *innerRepresentation, sourceString ) ) {
                    sourceFormat = "posixjsdl";
                    break;
                }
                //else
                resetJobTree();
            } else if ( sm.toLowerCase( (*it).typeName ) == "jsdl" ) {
                if ( VERBOSEX ) std::cerr << "[JobDescription] Try to parse as JSDL" << std::endl;
                JSDLParser parser;
                if ( parser.parse( *innerRepresentation, sourceString ) ) {
                    sourceFormat = "jsdl";
                    break;
                }
                //else
                resetJobTree();
            } else if ( sm.toLowerCase( (*it).typeName ) == "jdl" ) {
                if ( VERBOSEX ) std::cerr << "[JobDescription] Try to parse as JDL" << std::endl;
                JDLParser parser;
                if ( parser.parse( *innerRepresentation, sourceString ) ) {
                    sourceFormat = "jdl";
                    break;
                }
                //else
                resetJobTree();
            }
        }
        if (sourceFormat.length() == 0){
           std::cerr << "The parsing of the source string was unsuccessful." << std::endl;
           return false;
        }
        return true;
    }

    // Create a new Arc::XMLNode with a single root element and out it into the JobDescription's jobTree variable
    void JobDescription::resetJobTree() {
        if (innerRepresentation != NULL) delete innerRepresentation;
        innerRepresentation = new JobInnerRepresentation();
    }

    // Returns with "true" and the jobTree XMLNode, or "false" if the innerRepresentation is emtpy.
    bool JobDescription::getXML( Arc::XMLNode& jobTree) {
        if ( innerRepresentation == NULL ) return false;
        if ( !(*innerRepresentation).getXML(jobTree) ) {
           std::cerr << "Error during the XML generation!" << std::endl;
           return false;
        }
        return true;
    }
    
    // Generate the output in the requested format
    bool JobDescription::getProduct( std::string& product, std::string format ) {
        // Initialize the necessary variables
        product = "";

        // Generate the output text with the right parser class
        if ( innerRepresentation == NULL || !this->isValid() ) {
            std::cerr << "There is no successfully parsed source" << std::endl;
            return false;
        }
        if ( sm.toLowerCase( format ) == sm.toLowerCase( sourceFormat ) && 
             sm.toLowerCase( sourceFormat ) != "xrsl" ) {
            product = sourceString;
            return true;
        }
        if ( sm.toLowerCase( format ) == "jdl" ) {
            if ( VERBOSEX ) std::cerr << "[JobDescription] Generate JDL output" << std::endl;
            JDLParser parser;
            if ( !parser.getProduct( *innerRepresentation, product ) ) {
               std::cerr << "Generating " << format << " output was unsuccessful" << std::endl;
               return false;
            }
            return true;
        } else if ( sm.toLowerCase( format ) == "xrsl" ) {
            if ( VERBOSEX ) std::cerr << "[JobDescription] Generate XRSL output" << std::endl;
            XRSLParser parser;
            if ( !parser.getProduct( *innerRepresentation, product ) ) {
               std::cerr << "Generating " << format << " output was unsuccessful" << std::endl;
               return false;
            }
            return true;
        } else if ( sm.toLowerCase( format ) == "posixjsdl" ) {
            if ( VERBOSEX ) std::cerr << "[JobDescription] Generate POSIX JSDL output" << std::endl;
            PosixJSDLParser parser;
            if ( !parser.getProduct( *innerRepresentation, product ) ) {
               std::cerr << "Generating " << format << " output was unsuccessful" << std::endl;
               return false;
            }
            return true;
        } else if ( sm.toLowerCase( format ) == "jsdl" ) {
            if ( VERBOSEX ) std::cerr << "[JobDescription] Generate JSDL output" << std::endl;
            JSDLParser parser;
            if ( !parser.getProduct( *innerRepresentation, product ) ) {
               std::cerr << "Generating " << format << " output was unsuccessful" << std::endl;
               return false;
            }
            return true;
        } else {
            std::cerr << "Unknown output format: " << format << std::endl;
            return false;
        }
    }

    bool JobDescription::getSourceFormat( std::string& _sourceFormat ) {
        if (!isValid()) {
           std::cerr << "There is no input defined yet or it's format can be determinized." << std::endl;
           return false;
         } else {
           _sourceFormat = sourceFormat;
           return true;
        } 
    }

    bool JobDescription::getUploadableFiles(std::vector< std::pair< std::string, std::string > >& sourceFiles ) {
        if (!isValid()) {
           std::cerr << "There is no input defined yet or it's format can be determinized." << std::endl;
           return false;
        }
        // Get the URI's from the DataStaging/File/Source files
        Arc::XMLNode xml_repr;
        if ( !getXML(xml_repr) ) return false;
        Arc::XMLNode sourceNode = xml_repr["JobDescription"]["DataStaging"]["File"];
        while ((bool)sourceNode) {
            if ((bool)sourceNode["Source"]) {
                std::string uri ((std::string) sourceNode["Source"]["URI"]);
                std::string filename ((std::string) sourceNode["Name"]);
                Arc::URL sourceUri (uri);
                if (sourceUri.Protocol() == "file") {
                    std::pair< std::string, std::string > item(filename,uri);
                    sourceFiles.push_back(item);
                }
            }
            ++sourceNode;
        }
        return true;
    }

    long get_limit(Arc::XMLNode range)
    {
       if (!range) return -1;
       Arc::XMLNode n = range["LowerBound"];
       if ((bool)n) {
          return Arc::stringto<long>((std::string)n);
       }
       n = range["UpperBound"];
       if ((bool)n) {
          return Arc::stringto<long>((std::string)n);
       }
       return -1;
    } 

    bool PosixJSDLParser::parse( Arc::JobInnerRepresentation& innerRepresentation, const std::string source ) { 
        
        Arc::XMLNode node(source);
        Arc::NS nsList;
        nsList.insert(std::pair<std::string, std::string>("jsdl","http://schemas.ggf.org/jsdl/2005/11/jsdl"));
        nsList.insert(std::pair<std::string, std::string>("jsdlPOSIX","http://schemas.ggf.org/jsdl/2005/11/jsdl-posix"));
        nsList.insert(std::pair<std::string, std::string>("jsdlARC","http://www.nordugrid.org/ws/schemas/jsdl-arc"));
      
        node.Namespaces(nsList);

        Arc::XMLNode jobdescription = node["JobDescription"];

        if (bool(jobdescription["LocalLogging"]["Directory"])) {
           innerRepresentation.LogDir = (std::string)jobdescription["LocalLogging"]["Directory"];
        }

        if (bool(jobdescription["RemoteLogging"]["URL"])) {
           URL url((std::string)jobdescription["RemoteLogging"]["URL"]);
           innerRepresentation.RemoteLogging = url;
        }

        if (bool(jobdescription["CredentialServer"]["URL"])) {
           URL url((std::string)jobdescription["CredentialServer"]["URL"]);
           innerRepresentation.CredentialService = url;
        }

        if (bool(jobdescription["ProcessingStartTime"])) {
           Time stime((std::string)jobdescription["ProcessingStartTime"]);
           innerRepresentation.ProcessingStartTime = stime;
        }

        if (bool(jobdescription["Reruns"])) {
           innerRepresentation.LRMSReRun = stringtoi((std::string)jobdescription["Reruns"]);
        }

        Arc::XMLNode jobidentification = node["JobDescription"]["JobIdentification"];

        if (bool(jobidentification["JobName"])) {
           innerRepresentation.JobName = (std::string)jobidentification["JobName"];
        }

        if (bool(jobidentification["AccessControl"]["Content"])) {
           innerRepresentation.AccessControl = jobidentification["AccessControl"]["Content"];
        }

        if (bool(jobidentification["Notify"])) {
//TODO:
//           innerRepresentation.Notification.? = jobidentification["Notify"];
        }

        if (bool(jobidentification["JobProject"])) {
           innerRepresentation.JobProject = (std::string)jobidentification["JobProject"];
        }

        Arc::XMLNode application = node["JobDescription"]["Application"]["POSIXApplication"];
 
        if (bool(application["Executable"])) {
           innerRepresentation.Executable = (std::string)application["Executable"];
        }

        for ( int i=0; (bool)(application["Argument"][i]); i++ ) { 
           std::string value = (std::string) application["Argument"][i];
           innerRepresentation.Argument.push_back(value);
        }   

        if (bool(application["Input"]))  {
           innerRepresentation.Input = (std::string)application["Input"];
        }

        if (bool(application["Output"]))  {
           innerRepresentation.Output = (std::string)application["Output"];
        }

        if (bool(application["Error"]))  {
           innerRepresentation.Error = (std::string)application["Error"];
        }

        for (int i=0; (bool)(application["Environment"][i]); i++) {
           Arc::XMLNode env = application["Environment"][i];
           Arc::XMLNode name = env.Attribute("name");
           if(!name) {
               std::cerr << "[PosixJSDLParser] Error during the parsing: missed the name attributes of the \"" << (std::string)env << "\" Environment" << std::endl;
               return false;
           } 
           Arc::EnvironmentType env_tmp;
           env_tmp.name_attribute = (std::string)name;
           env_tmp.value = (std::string)env;
           innerRepresentation.Environment.push_back(env_tmp);
        }

        if (bool(application["VirtualMemoryLimit"])) {
           innerRepresentation.IndividualVirtualMemory = Arc::stringto<int>((std::string)application["VirtualMemoryLimit"]);
        }

        if (bool(application["CPUTimeLimit"])) {
           Period time((std::string)application["CPUTimeLimit"]);
           innerRepresentation.TotalCPUTime = time;
        }

        if (bool(application["WallTimeLimit"])) {
           Period time((std::string)application["WallTimeLimit"]);
           innerRepresentation.TotalWallTime = time;
        }

        if (bool(application["MemoryLimit"])) {
           innerRepresentation.IndividualPhysicalMemory = Arc::stringto<int>((std::string)application["MemoryLimit"]);
        }

        if (bool(application["ProcessCountLimit"])) {
           innerRepresentation.Slots = stringtoi((std::string)application["ProcessCountLimit"]);
           innerRepresentation.NumberOfProcesses = stringtoi((std::string)application["ProcessCountLimit"]);
        }

        if (bool(application["ThreadCountLimit"])) {
           innerRepresentation.ThreadPerProcesses = stringtoi((std::string)application["ThreadCountLimit"]);
        }

        Arc::XMLNode resource = node["JobDescription"]["Resource"];

        if (bool(resource["SessionLifeTime"])) {
           Period time((std::string)resource["SessionLifeTime"]);
           innerRepresentation.SessionLifeTime = time;
        }

        if (bool(resource["TotalCPUTime"])) {
           int value = get_limit((std::string)resource["TotalCPUTime"]);
           if (value != -1)
              innerRepresentation.TotalCPUTime = value;
        }

        if (bool(resource["IndividualCPUTime"])) {
           int value = get_limit((std::string)resource["IndividualCPUTime"]);
           if (value != -1)
              innerRepresentation.IndividualCPUTime = value;
        }

        if (bool(resource["IndividualPhysicalMemory"])) {
           int value = get_limit((std::string)resource["IndividualPhysicalMemory"]);
           if (value != -1)
              innerRepresentation.IndividualPhysicalMemory = value;
        }

        if (bool(resource["IndividualVirtualMemory"])) {
           int value = get_limit((std::string)resource["IndividualVirtualMemory"]);
           if (value != -1)
              innerRepresentation.IndividualVirtualMemory = value;
        }

        if (bool(resource["IndividualDiskSpace"])) {
           int value = get_limit((std::string)resource["IndividualDiskSpace"]);
           if (value != -1)
              innerRepresentation.IndividualDiskSpace = value;
        }

        if (bool(resource["DiskSpace"])) {
           int value = get_limit((std::string)resource["DiskSpace"]);
           if (value != -1)
              innerRepresentation.DiskSpace = value;
        }

        if (bool(resource["GridTimeLimit"])) {
           innerRepresentation.ReferenceTime.value = (std::string)resource["GridTimeLimit"];
        }

        if (bool(resource["ExclusiveExecution"])) {
           StringManipulator sm;

           if ( sm.toLowerCase((std::string)resource["ExclusiveExecution"]) == "true"){
              innerRepresentation.ExclusiveExecution = true;
           }else{
              innerRepresentation.ExclusiveExecution = false;
           }
        }

        if (bool(resource["IndividualNetworkBandwidth"])) {
           long bits_per_sec = get_limit(resource["IndividualNetworkBandwidth"]);
           std::string value = "";
           long network = 1024*1024;
           if (bits_per_sec < 100*network) {
              value = "100megabitethernet";
           }
           else if (bits_per_sec < 1024*network) {
              value = "gigabitethernet";
           }
           else if (bits_per_sec < 10*1024*network) {
              value = "myrinet";
           }
           else {
              value = "infiniband";
           }
           innerRepresentation.NetworkInfo = value;
        }

        if (bool(resource["OperatingSystem"]["OperatingSystemType"]["OperatingSystemName"])) {
           innerRepresentation.OSName = (std::string)resource["OperatingSystem"]["OperatingSystemType"]["OperatingSystemName"];
        }

        if (bool(resource["OperatingSystem"]["OperatingSystemVersion"])) {
           innerRepresentation.OSVersion = (std::string)resource["OperatingSystem"]["OperatingSystemVersion"];
        }

        if (bool(resource["CPUArchitecture"]["CPUArchitectureName"])) {
           innerRepresentation.Platform = (std::string)resource["CPUArchitecture"]["CPUArchitectureName"];
        }

        if (bool(resource["CandidateTarget"]["HostName"])) {
           URL host_url((std::string)resource["CandidateTarget"]["HostName"]);
           innerRepresentation.EndPointURL = host_url;
        }

        if (bool(resource["CandidateTarget"]["QueueName"])) {
           innerRepresentation.QueueName = (std::string)resource["CandidateTarget"]["QueueName"];
        }

        if (bool(resource["Middleware"])) {
           innerRepresentation.CEType = (std::string)resource["Middleware"];
        }

        if (bool(resource["TotalCPUCount"])) {
           innerRepresentation.ProcessPerHost = stringtoi((std::string)resource["TotalCPUCount"]);
        }

        if (bool(resource["RunTimeEnvironment"])) {
           Arc::RunTimeEnvironmentType rt;
           rt.Name = (std::string)resource["RunTimeEnvironment"];
           innerRepresentation.RunTimeEnvironment.push_back(rt);
        }

        Arc::XMLNode datastaging = node["JobDescription"]["DataStaging"];
       
        for(int i=0; datastaging[i]; i++) {
           Arc::XMLNode ds = datastaging[i];
           Arc::XMLNode source_uri = ds["Source"]["URI"];
           Arc::XMLNode target_uri = ds["Target"]["URI"];
           Arc::XMLNode filenameNode = ds["FileName"];

           Arc::FileType file;

           if (filenameNode) {
              file.Name = (std::string) filenameNode;
              if (bool(source_uri)) {
                 Arc::SourceType source;
                 URL uri;
                 uri.ChangePath((std::string)source_uri);
                 source.URI = uri;
                 file.Source.push_back(source);
              }
              if (bool(target_uri)) {
                 Arc::TargetType target;
                 URL uri;
                 uri.ChangePath((std::string)target_uri);
                 target.URI = uri;
                 file.Target.push_back(target);
              }

              StringManipulator sm;

              if (sm.toLowerCase(((std::string)ds["IsExecutable"])) == "true"){
                 file.IsExecutable = true;
              }else{
                 file.IsExecutable = false;
              }
              file.KeepData = false;
              file.DownloadToCache = false;
              innerRepresentation.File.push_back(file);
           }
        }
        return true;
    }

    bool PosixJSDLParser::getProduct( const Arc::JobInnerRepresentation& innerRepresentation, std::string& product ) {
        Arc::NS nsList;        
        nsList.insert(std::pair<std::string, std::string>("jsdl","http://schemas.ggf.org/jsdl/2005/11/jsdl"));
        nsList.insert(std::pair<std::string, std::string>("jsdlPOSIX","http://schemas.ggf.org/jsdl/2005/11/jsdl-posix"));
        nsList.insert(std::pair<std::string, std::string>("jsdlARC","http://www.nordugrid.org/ws/schemas/jsdl-arc"));
           
   
        Arc::XMLNode jobdefinition("<JobDefinition/>");
       
        jobdefinition.Namespaces(nsList);

        Arc::XMLNode jobdescription = jobdefinition.NewChild("JobDescription");

        Arc::XMLNode jobidentification = jobdescription.NewChild("JobIdentification");

        jobidentification.NewChild("JobName") = innerRepresentation.JobName;

        Arc::XMLNode application = jobdescription.NewChild("Application").NewChild("POSIXApplication");

        application.NewChild("Executable") = innerRepresentation.Executable;
        application.NewChild("Input") = innerRepresentation.Input;
        application.NewChild("Output") = innerRepresentation.Output;
        application.NewChild("Error") = innerRepresentation.Error;

        for (std::list<std::string>::const_iterator it=innerRepresentation.Argument.begin();
                 it!=innerRepresentation.Argument.end(); it++) {
             application.NewChild("Argument") = *it;
        }

        Arc::XMLNode datastaging = jobdescription.NewChild("DataStaging");

        for (std::list<Arc::FileType>::const_iterator it=innerRepresentation.File.begin();
                 it!=innerRepresentation.File.end(); it++) {
            datastaging.NewChild("FileName") = (*it).Name;
            if ((*it).Source.size() != 0) {
               std::list<Arc::SourceType>::const_iterator it2;
               it2 = ((*it).Source).begin();
               datastaging.NewChild("Source").NewChild("URI") = ((*it2).URI).fullstr();
            }
            if ((*it).Target.size() != 0) {
               std::list<Arc::TargetType>::const_iterator it3;
               it3 = ((*it).Target).begin();
               datastaging.NewChild("Target").NewChild("URI") = ((*it3).URI).fullstr();
           }      
           if ((*it).IsExecutable)
              datastaging.NewChild("IsExecutable") = "true";
       }

       jobdefinition.GetDoc( product, true );

       return true;
    }

    bool JSDLParser::parse( Arc::JobInnerRepresentation& innerRepresentation, const std::string source ) {
        //TODO: wath is the new GIN namespace  (it is now temporary namespace)      
        Arc::XMLNode node(source);
        Arc::NS nsList;
        nsList.insert(std::pair<std::string, std::string>("jsdl","http://schemas.ggf.org/jsdl/2005/11/jsdl"));
        nsList.insert(std::pair<std::string, std::string>("jsdlPOSIX","http://schemas.ggf.org/jsdl/2005/11/jsdl-posix"));
        nsList.insert(std::pair<std::string, std::string>("jsdlARC","http://www.nordugrid.org/ws/schemas/jsdl-arc"));

        //Meta
        Arc::XMLNodeList meta;
        meta = node.XPathLookup( (std::string) "//Meta/OptionalElement", nsList);
        for (std::list<Arc::XMLNode>::iterator it = meta.begin(); it != meta.end(); it++) {
                 innerRepresentation.OptionalElement.push_back( (std::string)(*it) );
        }
        meta.clear();

        meta = node.XPathLookup((std::string)"//Meta/Author", nsList);
        if ( !meta.empty() )
           innerRepresentation.Author = (std::string)(*meta.begin());
        meta.clear();

        meta = node.XPathLookup((std::string)"//Meta/DocumentExpiration", nsList);
        if ( !meta.empty() ) {
           Time time((std::string)(*meta.begin()));
           innerRepresentation.DocumentExpiration = time;
        }
        meta.clear();

        meta = node.XPathLookup((std::string)"//Meta/Rank", nsList);
        if ( !meta.empty() )
           innerRepresentation.Rank = (std::string)(*meta.begin());
        meta.clear();

        meta = node.XPathLookup((std::string)"//Meta/FuzzyRank", nsList);
        if ( !meta.empty() )
           if ( (std::string)(*meta.begin()) == "true" ) {
              innerRepresentation.FuzzyRank = true;
           }
           else if ( (std::string)(*meta.begin()) == "false" ) {
              innerRepresentation.FuzzyRank = false;
           }
           else {
              std::cerr << "Invalid \"/Meta/FuzzyRank\" value:" << (std::string)(*meta.begin()) << std::endl;              
           }
        meta.clear();

        //Application         
        Arc::XMLNodeList application;
        application = node.XPathLookup((std::string)"//JobDescription/Application/Executable", nsList);
        if ( !application.empty() )
           innerRepresentation.Executable = (std::string)(*application.begin());
        application.clear();

        application = node.XPathLookup((std::string)"//JobDescription/Application/LogDir", nsList);
        if ( !application.empty() )
           innerRepresentation.LogDir = (std::string)(*application.begin());
        application.clear();

        application = node.XPathLookup( (std::string) "//JobDescription/Application/Argument", nsList);
        for (std::list<Arc::XMLNode>::iterator it = application.begin(); it != application.end(); it++) {
                innerRepresentation.Argument.push_back( (std::string)(*it) );
        }
        application.clear();

        application = node.XPathLookup((std::string)"//JobDescription/Application/Input", nsList);
        if ( !application.empty() )
           innerRepresentation.Input = (std::string)(*application.begin());
        application.clear();

        application = node.XPathLookup((std::string)"//JobDescription/Application/Output", nsList);
        if ( !application.empty() )
           innerRepresentation.Output = (std::string)(*application.begin());
        application.clear();

        application = node.XPathLookup((std::string)"//JobDescription/Application/Error", nsList);
        if ( !application.empty() )
          innerRepresentation.Error = (std::string)(*application.begin());
        application.clear();

        application = node.XPathLookup((std::string)"//JobDescription/Application/RemoteLogging", nsList);
        if ( !application.empty() ) {
           URL url((std::string)(*application.begin()));
           innerRepresentation.RemoteLogging = url;
        }
        application.clear();

        application = node.XPathLookup( (std::string) "//JobDescription/Application/Environment", nsList);
        for (std::list<Arc::XMLNode>::iterator it = application.begin(); it != application.end(); it++) {
                Arc::EnvironmentType env;
                env.name_attribute = (std::string)(*it).Attribute("name");
                env.value = (std::string)(*it);
                innerRepresentation.Environment.push_back( env );
        }
        application.clear();

        application = node.XPathLookup((std::string)"//JobDescription/Application/LRMSReRun", nsList);
        if ( !application.empty() )
           innerRepresentation.LRMSReRun = stringtoi(*application.begin());
        application.clear();

        application = node.XPathLookup((std::string)"//JobDescription/Application/Prologue", nsList);
        if ( !application.empty() ) {
           std::string source_prologue = (std::string)(*application.begin());
           std::vector<std::string> parts = sm.split( source_prologue, " " );
           if ( !parts.empty() ) {
              Arc::XLogueType _prologue;
              _prologue.Name = parts[0];
              for (std::vector<std::string>::const_iterator it = ++parts.begin(); it != parts.end(); it++) {
                  _prologue.Arguments.push_back( *it );
              }
              innerRepresentation.Prologue = _prologue;
           }
        }
        application.clear();

        application = node.XPathLookup((std::string)"//JobDescription/Application/Epilogue", nsList);
        if ( !application.empty() ) {
           std::string source_epilogue = (std::string)(*application.begin());
           std::vector<std::string> parts = sm.split( source_epilogue, " " );
           if ( !parts.empty() ) {
              Arc::XLogueType _epilogue;
              _epilogue.Name = parts[0];
              for (std::vector<std::string>::const_iterator it = ++parts.begin(); it != parts.end(); it++) {
                  _epilogue.Arguments.push_back( *it );
              }
              innerRepresentation.Epilogue = _epilogue;
           }
        }
        application.clear();

        application = node.XPathLookup((std::string)"//JobDescription/Application/SessionLifeTime", nsList);
        if ( !application.empty() ) {
           Period time((std::string)(*application.begin()));
           innerRepresentation.SessionLifeTime = time;
        }
        application.clear();

        application = node.XPathLookup((std::string)"//JobDescription/Application/AccessControl", nsList);
        if ( !application.empty() )
           ((Arc::XMLNode)(*application.begin())).Child(0).New(innerRepresentation.AccessControl);
        application.clear();

        application = node.XPathLookup((std::string)"//JobDescription/Application/ProcessingStartTime", nsList);
        if ( !application.empty() ) {
           Time stime((std::string)*application.begin());
           innerRepresentation.ProcessingStartTime = stime;
        }
        application.clear();

        application = node.XPathLookup( (std::string) "//JobDescription/Application/Notification", nsList);
        for (std::list<Arc::XMLNode>::iterator it = application.begin(); it != application.end(); it++) {
            Arc::NotificationType ntype;
            Arc::XMLNodeList address = (*it).XPathLookup( (std::string) "//Address", nsList);
            for (std::list<Arc::XMLNode>::iterator it_addr = address.begin(); it_addr != address.end(); it_addr++) {             
                ntype.Address.push_back( (std::string)(*it_addr) );
            }
            Arc::XMLNodeList state = (*it).XPathLookup( (std::string) "//State", nsList);
            for (std::list<Arc::XMLNode>::iterator it_state = state.begin(); it_state != state.end(); it_state++) {             
                ntype.State.push_back( (std::string)(*it_state) );
            }
            innerRepresentation.Notification.push_back( ntype );
        }
        application.clear();

        application = node.XPathLookup((std::string)"//JobDescription/Application/CredentialService", nsList);
        if ( !application.empty() ) {
           URL curl((std::string)*application.begin());
           innerRepresentation.CredentialService = curl;
        }
        application.clear();

        application = node.XPathLookup((std::string)"//JobDescription/Application/Join", nsList);
        if ( !application.empty() && Arc::upper((std::string)*application.begin()) == "TRUE" )
           innerRepresentation.Join = true;
        application.clear();

        //JobIdentification
        Arc::XMLNodeList jobidentification;
        jobidentification = node.XPathLookup((std::string)"//JobDescription/JobIdentification/JobName", nsList);
        if ( !jobidentification.empty() )
           innerRepresentation.JobName = (std::string)(*jobidentification.begin());
        jobidentification.clear();

        jobidentification = node.XPathLookup((std::string)"//JobDescription/JobIdentification/Description", nsList);
        if ( !jobidentification.empty() )
           innerRepresentation.Description = (std::string)(*jobidentification.begin());
        jobidentification.clear();

        jobidentification = node.XPathLookup((std::string)"//JobDescription/JobIdentification/JobProject", nsList);
        if ( !jobidentification.empty() )
           innerRepresentation.JobProject = (std::string)(*jobidentification.begin());
        jobidentification.clear();

        jobidentification = node.XPathLookup((std::string)"//JobDescription/JobIdentification/JobType", nsList);
        if ( !jobidentification.empty() )
           innerRepresentation.JobType = (std::string)(*jobidentification.begin());
        jobidentification.clear();

        jobidentification = node.XPathLookup((std::string)"//JobDescription/JobIdentification/JobCategory", nsList);
        if ( !jobidentification.empty() )
           innerRepresentation.JobCategory = (std::string)(*jobidentification.begin());
        jobidentification.clear();

        jobidentification = node.XPathLookup((std::string)"//JobDescription/JobIdentification/UserTag", nsList);
        for (std::list<Arc::XMLNode>::const_iterator it = jobidentification.begin(); it != jobidentification.end(); it++) {
            innerRepresentation.UserTag.push_back( (std::string)(*it));
        }
        jobidentification.clear();

        //Resource
        Arc::XMLNodeList resource;
        resource = node.XPathLookup((std::string)"//JobDescription/Resource/TotalCPUTime", nsList);
        if ( !resource.empty() ) {
           Period time((std::string)(*resource.begin()));
           innerRepresentation.TotalCPUTime = time;
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/IndividualCPUTime", nsList);
        if ( !resource.empty() ) {
           Period time((std::string)(*resource.begin()));
           innerRepresentation.IndividualCPUTime = time;
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/TotalWallTime", nsList);
        if ( !resource.empty() ) {
           Period _walltime((std::string)(*resource.begin()));
           innerRepresentation.TotalWallTime = _walltime;
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/IndividualWallTime", nsList);
        if ( !resource.empty() ) {
           Period _indwalltime((std::string)(*resource.begin()));
           innerRepresentation.IndividualWallTime = _indwalltime;
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/ReferenceTime", nsList);
        if ( !resource.empty() ) {
           Arc::ReferenceTimeType _reference;
           _reference.value = (std::string)(*resource.begin());
           _reference.benchmark_attribute = (std::string)((Arc::XMLNode)(*resource.begin()).Attribute("benchmark"));
           _reference.value_attribute = (std::string)((Arc::XMLNode)(*resource.begin()).Attribute("value"));
           innerRepresentation.ReferenceTime = _reference;
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/ExclusiveExecution", nsList);
        if ( !resource.empty() ) {
           Arc::ReferenceTimeType _reference;
           if ( Arc::upper((std::string)(*resource.begin())) == "TRUE" ) {
              innerRepresentation.ExclusiveExecution = true;
           }
           else if ( Arc::upper((std::string)(*resource.begin())) == "FALSE" ) {
              innerRepresentation.ExclusiveExecution = false;
           }
           else {
              std::cerr << "Invalid \"/JobDescription/Resource/ExclusiveExecution\" value:" << (std::string)(*resource.begin()) << std::endl;
           }
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/NetworkInfo", nsList);
        if ( !resource.empty() )
           innerRepresentation.NetworkInfo = (std::string)(*resource.begin());
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/OSFamily", nsList);
        if ( !resource.empty() )
           innerRepresentation.OSFamily = (std::string)(*resource.begin());
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/OSName", nsList);
        if ( !resource.empty() )
           innerRepresentation.OSName = (std::string)(*resource.begin());
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/OSVersion", nsList);
        if ( !resource.empty() )
           innerRepresentation.OSVersion = (std::string)(*resource.begin());
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/Platform", nsList);
        if ( !resource.empty() )
           innerRepresentation.Platform = (std::string)(*resource.begin());
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/IndividualPhysicalMemory", nsList);
        if ( !resource.empty() )
           innerRepresentation.IndividualPhysicalMemory = stringtoi(*resource.begin());
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/IndividualVirtualMemory", nsList);
        if ( !resource.empty() )
           innerRepresentation.IndividualVirtualMemory = stringtoi(*resource.begin());
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/IndividualDiskSpace", nsList);
        if ( !resource.empty() )
           innerRepresentation.IndividualDiskSpace = stringtoi(*resource.begin());
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/DiskSpace", nsList);
        if ( !resource.empty())
           innerRepresentation.DiskSpace = stringtoi(*resource.begin());
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/CacheDiskSpace", nsList);
        if ( !resource.empty())
           innerRepresentation.CacheDiskSpace = stringtoi(*resource.begin());
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/SessionDiskSpace", nsList);
        if ( !resource.empty())
           innerRepresentation.SessionDiskSpace = stringtoi(*resource.begin());
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/CandidateTarget/EndPointURL", nsList);
        if ( !resource.empty() ) {
           URL url((std::string)(*resource.begin()));
           innerRepresentation.EndPointURL = url;
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/CandidateTarget/QueueName", nsList);
        if ( !resource.empty() )
           innerRepresentation.QueueName = (std::string)(*resource.begin());
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/CEType", nsList);
        if ( !resource.empty() )
           innerRepresentation.CEType = (std::string)(*resource.begin());
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/Slots/ProcessPerHost", nsList);
        if ( !resource.empty() )
           innerRepresentation.ProcessPerHost = stringtoi(*resource.begin());
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/Slots/NumberOfProcesses", nsList);
        if ( !resource.empty() )
           innerRepresentation.NumberOfProcesses = stringtoi(*resource.begin());
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/Slots/ThreadPerProcesses", nsList);
        if ( !resource.empty() )
           innerRepresentation.ThreadPerProcesses = stringtoi(*resource.begin());
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/Slots/SPMDVariation", nsList);
        if ( !resource.empty() )
           innerRepresentation.SPMDVariation = (std::string)(*resource.begin());
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/RunTimeEnvironment", nsList);
        for (std::list<Arc::XMLNode>::const_iterator it = resource.begin(); it != resource.end(); it++) {
            Arc::RunTimeEnvironmentType _runtimeenv;
            if ( (bool)((*it)["Name"]) )
               _runtimeenv.Name = (std::string)(*it)["Name"];
            Arc::XMLNodeList version = (*it).XPathLookup((std::string)"//Version", nsList);
            for (std::list<Arc::XMLNode>::const_iterator it_v = version.begin(); it_v!= version.end(); it_v++) {
                _runtimeenv.Version.push_back( (std::string)(*it_v) );
            }
            innerRepresentation.RunTimeEnvironment.push_back( _runtimeenv );
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/NodeAccess", nsList);
        if ( (bool)((Arc::XMLNode)(*resource.begin())["InBound"]) && 
             (std::string)((Arc::XMLNode)(*resource.begin())["InBound"]) == "true" )
           innerRepresentation.InBound = true;
        if ( (bool)((Arc::XMLNode)(*resource.begin())["OutBound"]) && 
             (std::string)((Arc::XMLNode)(*resource.begin())["OutBound"]) == "true" )
           innerRepresentation.OutBound = true;
        resource.clear();


        //DataStaging
        Arc::XMLNodeList datastaging;
        datastaging = node.XPathLookup((std::string)"//JobDescription/DataStaging/File", nsList);
        for (std::list<Arc::XMLNode>::const_iterator it = datastaging.begin(); it != datastaging.end(); it++) {
            Arc::FileType _file;
            if ( (bool)((*it)["Name"]) )
               _file.Name = (std::string)(*it)["Name"];
            Arc::XMLNodeList source = (*it).XPathLookup((std::string)"//Source", nsList);
            for (std::list<Arc::XMLNode>::const_iterator it_source = source.begin(); it_source!= source.end(); it_source++) {
                Arc::SourceType _source;
                if ( (bool)((*it_source)["URI"]) ) {
                   URL uri;
                   uri.ChangePath((std::string)(*it_source)["URI"]);
                   _source.URI = uri;
                }
                if ( (bool)((*it_source)["Threads"])) {
                   _source.Threads = stringtoi((std::string)(*it_source)["Threads"]);
                }
                else {
                   _source.Threads = -1;
                }
                _file.Source.push_back( _source );
            }
            Arc::XMLNodeList target = (*it).XPathLookup((std::string)"//Target", nsList);
            for (std::list<Arc::XMLNode>::const_iterator it_target = target.begin(); it_target!= target.end(); it_target++) {
                Arc::TargetType _target;
                if ( (bool)((*it_target)["URI"]) ) {
                   URL uri;
                   uri.ChangePath((std::string)(*it_target)["URI"]);
                   _target.URI = uri;
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
                       std::cerr << "Invalid \"/JobDescription/DataStaging/File/Target/Mandatory\" value:" << (std::string)(*it_target)["Mandatory"] << std::endl;
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
            if ( (bool)((*it)["KeepData"])) {
                   if ( (std::string)(*it)["KeepData"] == "true" ) {
                      _file.KeepData = true;
                   }
                   else if ( (std::string)(*it)["KeepData"] == "false" ) {
                      _file.KeepData = false;
                   }
                   else {
                       std::cerr << "Invalid \"/JobDescription/DataStaging/File/KeepData\" value:" << (std::string)(*it)["KeepData"] << std::endl;
                   }
            }
            else {
                   _file.KeepData = false;
            }
            if ( (bool)((*it)["IsExecutable"])) {
                   if ( (std::string)(*it)["IsExecutable"] == "true" ) {
                      _file.IsExecutable = true;
                   }
                   else if ( (std::string)(*it)["IsExecutable"] == "false" ) {
                      _file.IsExecutable = false;
                   }
                   else {
                       std::cerr << "Invalid \"/JobDescription/DataStaging/File/IsExecutable\" value:" << (std::string)(*it)["IsExecutable"] << std::endl;
                   }
            }
            else {
                   _file.IsExecutable = false;
            }
            if ( (bool)((*it)["DataIndexingService"])) {
                URL uri((std::string)((*it)["DataIndexingService"]));
                _file.DataIndexingService = uri;
            }
            if ( (bool)((*it)["DownloadToCache"])) {
                   if ( (std::string)(*it)["DownloadToCache"] == "true" ) {
                      _file.DownloadToCache = true;
                   }
                   else if ( (std::string)(*it)["DownloadToCache"] == "false" ) {
                      _file.DownloadToCache = false;
                   }
                   else {
                       std::cerr << "Invalid \"/JobDescription/DataStaging/File/DownloadToCache\" value:" << (std::string)(*it)["DownloadToCache"] << std::endl;
                   }
            }
            else {
                   _file.DownloadToCache = false;
            }
            innerRepresentation.File.push_back( _file );
        }
        datastaging.clear();

        datastaging = node.XPathLookup((std::string)"//JobDescription/DataStaging/Directory", nsList);
        for (std::list<Arc::XMLNode>::const_iterator it = datastaging.begin(); it != datastaging.end(); it++) {
            Arc::DirectoryType _directory;
            if ( (bool)((*it)["Name"]) )
               _directory.Name = (std::string)(*it)["Name"];
            Arc::XMLNodeList source = (*it).XPathLookup((std::string)"//Source", nsList);
            for (std::list<Arc::XMLNode>::const_iterator it_source = source.begin(); it_source!= source.end(); it_source++) {
                Arc::SourceType _source;
                if ( (bool)((*it_source)["URI"]) ) {
                   URL uri;
                   uri.ChangePath((std::string)(*it_source)["URI"]);
                   _source.URI = uri;
                }
                if ( (bool)((*it_source)["Threads"])) {
                   _source.Threads = stringtoi((std::string)(*it_source)["Threads"]);
                }
                else {
                   _source.Threads = -1;
                }
                _directory.Source.push_back( _source );
            }
            Arc::XMLNodeList target = (*it).XPathLookup((std::string)"//Target", nsList);
            for (std::list<Arc::XMLNode>::const_iterator it_target = target.begin(); it_target!= target.end(); it_target++) {
                Arc::TargetType _target;
                if ( (bool)((*it_target)["URI"]) ) {
                   URL uri;
                   uri.ChangePath((std::string)(*it_target)["URI"]);
                   _target.URI = uri;
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
                       std::cerr << "Invalid \"/JobDescription/DataStaging/Directory/Target/Mandatory\" value:" << (std::string)(*it_target)["Mandatory"] << std::endl;
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
                _directory.Target.push_back( _target );
            }
            if ( (bool)((*it)["KeepData"])) {
                   if ( (std::string)(*it)["KeepData"] == "true" ) {
                      _directory.KeepData = true;
                   }
                   else if ( (std::string)(*it)["KeepData"] == "false" ) {
                      _directory.KeepData = false;
                   }
                   else {
                       std::cerr << "Invalid \"/JobDescription/DataStaging/Directory/KeepData\" value:" << (std::string)(*it)["KeepData"] << std::endl;
                   }
            }
            else {
                   _directory.KeepData = false;
            }
            if ( (bool)((*it)["IsExecutable"])) {
                   if ( (std::string)(*it)["IsExecutable"] == "true" ) {
                      _directory.IsExecutable = true;
                   }
                   else if ( (std::string)(*it)["IsExecutable"] == "false" ) {
                      _directory.IsExecutable = false;
                   }
                   else {
                       std::cerr << "Invalid \"/JobDescription/DataStaging/Directory/IsExecutable\" value:" << (std::string)(*it)["IsExecutable"] << std::endl;
                   }
            }
            else {
                   _directory.IsExecutable = false;
            }
            if ( (bool)((*it)["DataIndexingService"])) {
                URL uri((std::string)((*it)["DataIndexingService"]));
                _directory.DataIndexingService = uri;
            }
            if ( (bool)((*it)["DownloadToCache"])) {
                   if ( (std::string)(*it)["DownloadToCache"] == "true" ) {
                      _directory.DownloadToCache = true;
                   }
                   else if ( (std::string)(*it)["DownloadToCache"] == "false" ) {
                      _directory.DownloadToCache = false;
                   }
                   else {
                       std::cerr << "Invalid \"/JobDescription/DataStaging/Directory/DownloadToCache\" value:" << (std::string)(*it)["DownloadToCache"] << std::endl;
                   }
            }
            else {
                   _directory.DownloadToCache = false;
            }
            innerRepresentation.Directory.push_back( _directory );
        }
        datastaging.clear();

        datastaging = node.XPathLookup((std::string)"//JobDescription/DataStaging/Defaults/DataIndexingService", nsList);
        if ( !datastaging.empty() ) {
           URL url((std::string)*datastaging.begin());
           innerRepresentation.DataIndexingService = url;
        }
        datastaging.clear();

        datastaging = node.XPathLookup((std::string)"//JobDescription/DataStaging/Defaults/StagingInBaseURI", nsList);
        if ( !datastaging.empty() ) {
           URL url((std::string)*datastaging.begin());
           innerRepresentation.StagingInBaseURI = url;
        }
        datastaging.clear();

        datastaging = node.XPathLookup((std::string)"//JobDescription/DataStaging/Defaults/StagingOutBaseURI", nsList);
        if ( !datastaging.empty() ) {
           URL url((std::string)*datastaging.begin());
           innerRepresentation.StagingOutBaseURI = url;
        }
        datastaging.clear();

        return true;
    }

    bool JSDLParser::getProduct( const Arc::JobInnerRepresentation& innerRepresentation, std::string& product ) {
        Arc::XMLNode jobDescription;
        if ( innerRepresentation.getXML(jobDescription) ) {
            jobDescription.GetDoc( product, true );
            return true;
        } else {
            if ( DEBUGX ) std::cerr << "[JSDLParser] Job inner representation's root element has not found." << std::endl;
            return false;
        }
    }

    std::string XRSLParser::simpleXRSLvalue(std::string attributeValue) {
        std::string quote_mark = "";
        std::string first_char_of_attributeValue = attributeValue.substr(0,1);

        //Ruler character examination
        if (first_char_of_attributeValue == "^") quote_mark = attributeValue.substr(0,2);
        else if (first_char_of_attributeValue == "\"") quote_mark = "\"";
        else if (first_char_of_attributeValue == "'") quote_mark = "'";
  
        if (quote_mark != "") attributeValue = attributeValue.substr(attributeValue.find_first_of(quote_mark)+quote_mark.length(),
            attributeValue.find_last_of(quote_mark.substr(0,1))-quote_mark.length()-attributeValue.find_first_of(quote_mark) );
  
        return attributeValue;
    }

    std::vector<std::string> XRSLParser::listXRSLvalue(std::string attributeValue) {

        std::string whitespaces (" \t\f\v\n\r");

        std::vector<std::string> attributes;
        unsigned long start_pointer;
        for ( unsigned long end_pointer = start_pointer = attributeValue.find_first_not_of(whitespaces); start_pointer != std::string::npos; ) {
            std::string next_char = attributeValue.substr(start_pointer,1);
            //If we have a custom quotation mark
            if ( next_char == "^" ) {
                std::string other_half = attributeValue.substr(start_pointer+1,1);
                end_pointer = attributeValue.find_first_of("^", start_pointer+1);
                while ( attributeValue.substr(end_pointer+1,1) != other_half ) {
                    end_pointer = attributeValue.find_first_of("^", end_pointer+1);
                }
                attributes.push_back( attributeValue.substr(start_pointer+2, end_pointer-start_pointer-2) );
                start_pointer = attributeValue.find_first_not_of(whitespaces, end_pointer+2);
            } else if ( next_char == "\"" || next_char == "'" ) {
                end_pointer = attributeValue.find_first_of( next_char, start_pointer+1 );
                while ( attributeValue.substr(end_pointer-1,1) == "\\" ) {
                    end_pointer = attributeValue.find_first_of( next_char, end_pointer+1);
                }
                attributes.push_back( attributeValue.substr(start_pointer+1, end_pointer-start_pointer-1) );
                start_pointer = attributeValue.find_first_not_of(whitespaces, end_pointer+1);
            } else {
                end_pointer = attributeValue.find_first_of(whitespaces, start_pointer+1);
                attributes.push_back( attributeValue.substr(start_pointer, end_pointer-start_pointer) );
                start_pointer = attributeValue.find_first_not_of(whitespaces, end_pointer);
            }
        }

        return attributes;
    }

    std::vector< std::vector<std::string> > XRSLParser::doubleListXRSLvalue(std::string attributeValue ) {

        std::vector< std::vector<std::string> > new_attributeValue;
        std::string whitespaces (" \t\f\v\n\r");
        std::string letters ("abcdefghijklmnopqrstuvwxyz");

        unsigned long outer_start_pointer;
        unsigned long outer_end_pointer = 0;
        for ( outer_start_pointer = attributeValue.find_first_of("(", outer_end_pointer); outer_start_pointer != std::string::npos; outer_start_pointer = attributeValue.find_first_of("(", outer_end_pointer) ) {
            unsigned long old_outer_end_pointer = outer_end_pointer;
            outer_end_pointer = attributeValue.find_first_of(")", outer_start_pointer);
            if ( attributeValue.find_first_of("(", old_outer_end_pointer+1) < outer_end_pointer &&
                 attributeValue.find_first_of(")", outer_end_pointer+1) != std::string::npos ) {
               outer_end_pointer = attributeValue.find_first_of(")", outer_end_pointer+1);
            }

            std::string inner_attributeValue = attributeValue.substr(outer_start_pointer+1,outer_end_pointer-outer_start_pointer-1);
            std::vector<std::string> attributes;
            unsigned long start_pointer;
            for ( unsigned long end_pointer = start_pointer = inner_attributeValue.find_first_not_of(whitespaces); start_pointer != std::string::npos; ) {
                std::string next_char = inner_attributeValue.substr(start_pointer,1);
                //If we have a custom quotation mark
                if ( next_char == "^" ) {
                    std::string other_half = inner_attributeValue.substr(start_pointer+1,1);
                    end_pointer = inner_attributeValue.find_first_of("^", start_pointer+1);
                    while ( inner_attributeValue.substr(end_pointer+1,1) != other_half ) {
                        end_pointer = inner_attributeValue.find_first_of("^", end_pointer+1);
                    }
                    attributes.push_back( inner_attributeValue.substr(start_pointer+2, end_pointer-start_pointer-2) );
                    start_pointer = inner_attributeValue.find_first_not_of(whitespaces, end_pointer+2);
                } else if ( next_char == "\"" || next_char == "'" ) {
                    end_pointer = inner_attributeValue.find_first_of( next_char, start_pointer+1 );
                    while ( end_pointer > 0 && inner_attributeValue.substr(end_pointer-1,1) == "\\" ) {
                        end_pointer = inner_attributeValue.find_first_of( next_char, end_pointer+1);
                    }
                    attributes.push_back( inner_attributeValue.substr(start_pointer+1, end_pointer-start_pointer-1) );
                    start_pointer = inner_attributeValue.find_first_not_of(whitespaces, end_pointer+1);
                } else {
                    end_pointer = inner_attributeValue.find_first_of(whitespaces, start_pointer+1);
                    if ( end_pointer != std::string::npos ) attributes.push_back( inner_attributeValue.substr(start_pointer, end_pointer-start_pointer) );
                    else attributes.push_back( inner_attributeValue.substr(start_pointer) );
                    start_pointer = inner_attributeValue.find_first_not_of(whitespaces, end_pointer);
                }
            }
            new_attributeValue.push_back( attributes );
        }
        return new_attributeValue;
    }

    bool XRSLParser::handleXRSLattribute(std::string attributeName, std::string attributeValue, Arc::JobInnerRepresentation& innerRepresentation) {

        // To do the attributes name case-insensitive do them lowercase and remove the quotiation marks
        attributeName = simpleXRSLvalue( sm.toLowerCase( attributeName ) );  

        if ( attributeName == "executable" ) {
            innerRepresentation.Executable = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "arguments") {
            std::vector<std::string> attributes = listXRSLvalue( attributeValue );
            for (std::vector<std::string>::const_iterator it=attributes.begin(); it<attributes.end(); it++) {
                innerRepresentation.Argument.push_back( (*it) );
            }
            return true;
        } else if ( attributeName == "stdin" ) {
            innerRepresentation.Input =  simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "stdout" ) {
            innerRepresentation.Output = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "stderr" ) {
            innerRepresentation.Error = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "inputfiles" ) {
            std::vector< std::vector<std::string> > attributesDoubleVector = doubleListXRSLvalue( attributeValue );
            for (std::vector< std::vector<std::string> >::const_iterator it=attributesDoubleVector.begin(); it<attributesDoubleVector.end(); it++) {
                Arc::FileType file;
                file.Name = (*it)[0];
                Arc::SourceType source;
                Arc::URL url;
                if ( (*it)[1] != "" ) url.ChangePath((*it)[1]);
                else url.ChangePath((*it)[0]);
                source.URI = url;
                source.Threads = -1;
                file.Source.push_back(source);
                //initializing this variables
                file.KeepData = false;
                file.IsExecutable = false;
                file.DownloadToCache = innerRepresentation.cached;
                innerRepresentation.File.push_back(file);
            }
            return true;
        } else if ( attributeName == "executables" ) {
            std::vector<std::string> attributes = listXRSLvalue( attributeValue );
            for (std::vector<std::string>::const_iterator it=attributes.begin(); it<attributes.end(); it++) {
                for (std::list<Arc::FileType>::iterator it2 = innerRepresentation.File.begin(); 
                                                        it2 != innerRepresentation.File.end(); it2++) {
                    if ( (*it2).Name == (*it) ) (*it2).IsExecutable = true;
                }
            }
            return true;
        } else if ( attributeName == "cache" ) {
            if ( Arc::upper(simpleXRSLvalue( attributeValue )) == "YES"){
               innerRepresentation.cached = true;
               for (std::list<Arc::FileType>::iterator it = innerRepresentation.File.begin(); 
                                                       it != innerRepresentation.File.end(); it++) {
                   if ( !(*it).Source.empty() ) (*it).DownloadToCache = true;
               }
            }
            return true;
        } else if ( attributeName == "outputfiles" ) {
            std::vector< std::vector<std::string> > attributesDoubleVector = doubleListXRSLvalue( attributeValue );
            for (std::vector< std::vector<std::string> >::const_iterator it=attributesDoubleVector.begin(); it<attributesDoubleVector.end(); it++) {
                Arc::FileType file;
                file.Name = (*it)[0];
                Arc::TargetType target;
                Arc::URL url;
                if ( (*it)[1] != "" ) url.ChangePath((*it)[1]);
                else url.ChangePath((*it)[0]);
                target.URI = url;
                target.Threads = -1;
                target.Mandatory = false;
                target.NeededReplicas = -1;
                file.Target.push_back(target);
                //initializing this variables
                file.KeepData = false;
                file.IsExecutable = false;
                file.DownloadToCache = false;
                innerRepresentation.File.push_back(file);
            }
            return true;
        } else if ( attributeName == "queue" ) {
            innerRepresentation.QueueName = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "starttime" ) {
            Time time(simpleXRSLvalue( attributeValue ) );
            innerRepresentation.ProcessingStartTime = time;
            return true;
        } else if ( attributeName == "lifetime" ) {
            Period time(simpleXRSLvalue( attributeValue ) );
            innerRepresentation.SessionLifeTime = time;
            return true;
        } else if ( attributeName == "cputime" ) {
            Period time(simpleXRSLvalue( attributeValue ) );
            innerRepresentation.TotalCPUTime = time;
            return true;
        } else if ( attributeName == "walltime" ) {
            Period time(simpleXRSLvalue( attributeValue ) );
            innerRepresentation.TotalWallTime = time;
            return true;
        } else if ( attributeName == "gridtime" ) {
            innerRepresentation.ReferenceTime.value = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "benchmarks" ) {
            //for example: (benchmarks=("mybenchmark" "10" "1 hour, 30 minutes"))
            std::vector<std::string> attributes = listXRSLvalue( attributeValue );
            if ( (int)attributes.size() > 2 ) { 
               innerRepresentation.ReferenceTime.benchmark_attribute = attributes[0];
               innerRepresentation.ReferenceTime.value_attribute = attributes[1];
               innerRepresentation.ReferenceTime.value = attributes[2];
            }
            return true;
        } else if ( attributeName == "memory" ) {
            innerRepresentation.IndividualPhysicalMemory = stringtoi(simpleXRSLvalue( attributeValue ));
            return true;
        } else if ( attributeName == "disk" ) {
            innerRepresentation.DiskSpace = stringtoi(simpleXRSLvalue( attributeValue ));
            return true;
        } else if ( attributeName == "runtimeenvironment" ) {
            Arc::RunTimeEnvironmentType runtime;
            runtime.Name =  simpleXRSLvalue( attributeValue );
            //runtime.Version = ?
            innerRepresentation.RunTimeEnvironment.push_back( runtime );
            return true;
        } else if ( attributeName == "middleware" ) {
            // Not supported yet, only store it
            innerRepresentation.XRSL_elements["middleware"] = simpleXRSLvalue( attributeValue ); 
            return true;
        } else if ( attributeName == "opsys" ) {
            innerRepresentation.OSName = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "join" ) {
            if ( Arc::upper(simpleXRSLvalue( attributeValue )) == "TRUE")
               innerRepresentation.Join = true;
            return true;
        } else if ( attributeName == "gmlog" ) {
            innerRepresentation.LogDir = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "jobname" ) {
            innerRepresentation.JobName = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "ftpthreads" ) {
            int threads = stringtoi( simpleXRSLvalue( attributeValue ) );
            //set the File
            std::list<Arc::FileType>::iterator iter;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
              std::list<Arc::SourceType>::iterator it_source;
              for (it_source = (*iter).Source.begin(); it_source != (*iter).Source.end(); it_source++){
                  if ( (*it_source).Threads > threads || (*it_source).Threads == -1 )
                     (*it_source).Threads = threads;
              }
              std::list<Arc::TargetType>::iterator it_target;
              for (it_target = (*iter).Target.begin(); it_target != (*iter).Target.end(); it_target++){
                  if ( (*it_target).Threads > threads || (*it_target).Threads == -1 )
                     (*it_target).Threads = threads;
              }
            }
            //set the Directory
            std::list<Arc::DirectoryType>::iterator iterd;
            for (iterd = innerRepresentation.Directory.begin(); iterd != innerRepresentation.Directory.end(); iterd++){
              std::list<Arc::SourceType>::iterator it_source;
              for (it_source = (*iterd).Source.begin(); it_source != (*iterd).Source.end(); it_source++){
                  if ( (*it_source).Threads > threads || (*it_source).Threads == -1 )
                     (*it_source).Threads = threads;
              }
              std::list<Arc::TargetType>::iterator it_target;
              for (it_target = (*iter).Target.begin(); it_target != (*iter).Target.end(); it_target++){
                  if ( (*it_target).Threads > threads || (*it_target).Threads == -1 )
                     (*it_target).Threads = threads;
              }
            }
            return true;
        } else if ( attributeName == "acl" ) {
            Arc::XMLNode node(simpleXRSLvalue( attributeValue ));
            node.New(innerRepresentation.AccessControl);
            return true;
        } else if ( attributeName == "cluster" ) {
            // Not supported yet, only store it
            innerRepresentation.XRSL_elements["cluster"] = simpleXRSLvalue( attributeValue ); 
            return true;
        } else if ( attributeName == "notify" ) {
            std::string value = simpleXRSLvalue( attributeValue );
            std::vector<std::string> parts = sm.split( value, " " );
            Arc::NotificationType nofity;

            for (std::vector< std::string >::const_iterator it=parts.begin(); it<parts.end(); it++) {
              //flags or e-mail address
              if ( it == parts.begin() ){
                 //flag conversion to Job State
                 for (int i=0; i!= (*it).length(); i++) {
                     switch ( (*it)[i] ){ 
                         case 'b': nofity.State.push_back( "PREPARING" ); break;
                         case 'q': nofity.State.push_back( "INLRMS" ); break;
                         case 'f': nofity.State.push_back( "FINISHING" ); break;
                         case 'e': nofity.State.push_back( "FINISHED" ); break;
                         case 'c': nofity.State.push_back( "CANCELLED" ); break;
                         case 'd': nofity.State.push_back( "DELETED" ); break;
                         default: std::cerr << "Invalid notify attribute: " << (*it)[i] << std::endl;
                                  return false;
                     }
                 }
              }
              else {
                 nofity.Address.push_back( *it );
              }
            }
            innerRepresentation.Notification.push_back( nofity );
            return true;
        } else if ( attributeName == "replicacollection" ) {
            Arc::URL url(simpleXRSLvalue( attributeValue ));
            innerRepresentation.DataIndexingService = url;
            return true;
        } else if ( attributeName == "rerun" ) {
            innerRepresentation.LRMSReRun = stringtoi(simpleXRSLvalue( attributeValue ));
            return true;
        } else if ( attributeName == "architecture" ) {
            innerRepresentation.Platform = simpleXRSLvalue( attributeValue );
            return true;
        } else if ( attributeName == "nodeaccess" ) {
            if ( simpleXRSLvalue( attributeValue ) == "inbound") {
               innerRepresentation.InBound = true;
            } else if ( simpleXRSLvalue( attributeValue ) == "outbound") {
               innerRepresentation.OutBound = true;
            } else {
               std::cerr << "Invalid NodeAccess value: " << simpleXRSLvalue( attributeValue ) << std::endl;
               return false;
            }
            return true;
        } else if ( attributeName == "dryrun" ) {
            // Not supported yet, only store it
            innerRepresentation.XRSL_elements["dryrun"] = simpleXRSLvalue( attributeValue ); 
            return true;
        } else if ( attributeName == "rsl_substitution" ) {
            // for example: (inputfiles=("myfile" $(ATLAS)/data/somefile))
            std::vector< std::vector<std::string> > attributesDoubleVector = doubleListXRSLvalue( attributeValue );
            for (std::vector< std::vector<std::string> >::const_iterator it=attributesDoubleVector.begin();
                                       it<attributesDoubleVector.end(); it++) {
                std::string internal_name = "$(" +(*it)[0] + ")";
                std::string replace_value = (*it)[1];
                size_t found;

                std::list<Arc::FileType>::iterator iter;
                for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                     std::list<Arc::SourceType>::iterator it_source;
                     for (it_source = (*iter).Source.begin(); it_source != (*iter).Source.end(); it_source++){
                          std::string uri = (*it_source).URI.fullstr();
                          found=uri.find(internal_name);
                          if (found != std::string::npos ) {
                             //replace the URI with the new path
                             uri.replace(found,internal_name.length(),replace_value);
                             (*it_source).URI.ChangePath(uri);
                          }
                     }
                }
            }
            return true;
        } else if ( attributeName == "environment" ) {
            std::vector< std::vector<std::string> > attributesDoubleVector = doubleListXRSLvalue( attributeValue );
            for (std::vector< std::vector<std::string> >::const_iterator it=attributesDoubleVector.begin();
                                       it<attributesDoubleVector.end(); it++) {
                Arc::EnvironmentType env;
                env.name_attribute = (*it)[1];
                env.value = (*it)[0];
                innerRepresentation.Environment.push_back(env);
           }
            return true;
        } else if ( attributeName == "count" ) {
            innerRepresentation.ProcessPerHost = stringtoi(simpleXRSLvalue( attributeValue ));
            return true;
        } else if ( attributeName == "jobreport" ) {
            Arc::URL url(simpleXRSLvalue( attributeValue ));
            innerRepresentation.RemoteLogging = url;
            return true;
        } else if ( attributeName == "credentialserver" ) {
            Arc::URL url(simpleXRSLvalue( attributeValue ));
            innerRepresentation.CredentialService = url;
            return true;
        }
        if ( DEBUGX ) std::cerr << "[XRSLParser] Unknown XRSL attribute: " << attributeName << std::endl;
        return false;
    }

    bool XRSLParser::parse( Arc::JobInnerRepresentation& innerRepresentation, const std::string source ) {
        //First step: trim the text
        //Second step: distinguis the different attributes & convert string to C char array
        int depth = 0;
        std::string xrsl_text = source.substr(source.find_first_of("&")+1);
        bool comment = false;
        std::string actual_argument = "";
  
        for (unsigned long pos=0; pos < xrsl_text.size(); pos++) {
            char next_char = xrsl_text[pos];
            if ( comment && next_char == ')' && pos >= 1 && xrsl_text[pos-1] == '*' ) comment = false;
            else if ( !comment ) {
                if ( next_char == '*' && pos >= 1 && xrsl_text[pos-1] == '(' ) {
                    comment = true;
                    depth--;
                } else if ( next_char == '(' ) {
                    if ( depth == 0 ) actual_argument.clear();
                    else actual_argument += next_char;
                    depth++;
                } else if ( next_char == ')' ) {
                    depth--;
                    if ( depth == 0 ) {
                        //Third step: You can handle the different attribute blocks here.
                        //Trim the unnecassary whitespaces
                        std::string wattr = sm.trim( actual_argument );
                        //Split the bracket's content by the equal sign & trim the two half
                        unsigned long eqpos = wattr.find_first_of("=");
                        if ( eqpos == std::string::npos ) {
                            if ( DEBUGX ) std::cerr << "[XRSLParser] XRSL Syntax error (attribute declaration without equal sign)" << std::endl;
                            return false;
                        }
                        if ( !handleXRSLattribute( sm.trim( wattr.substr(0,eqpos) ) , sm.trim( wattr.substr(eqpos+1) ) , innerRepresentation) ) return false;
                    } else actual_argument += next_char;
                } else {
                    actual_argument += next_char;
                }
            }
        }
  
        if (depth != 0 ) {
            if ( DEBUGX ) std::cerr << "[XRSLParser] XRSL Syntax error (bracket mistake)" << std::endl;
            return false;
        }
        return true;
    }

    bool XRSLParser::getProduct( const Arc::JobInnerRepresentation& innerRepresentation, std::string& product ) {
        if (!innerRepresentation.Executable.empty()) {
            product += "( executable = ";//TODO:
            product += innerRepresentation.Executable;
            product += " )\n";
        }
        if (!innerRepresentation.Argument.empty()) {
            for (std::list<std::string>::const_iterator it=innerRepresentation.Argument.begin();
                            it!=innerRepresentation.Argument.end(); it++) {
                product += "( arguments = ";//TODO:
                product += *it;
                product += " )\n";
            }
        }
        if (!innerRepresentation.Input.empty()) {
            product += "( stdin = ";
            product += innerRepresentation.Input;
            product += " )\n";
        }
        if (!innerRepresentation.Output.empty()) {
            product += "( stdout = ";
            product += innerRepresentation.Output;
            product += " )\n";
        }
        if (!innerRepresentation.Error.empty()) {
            product += "( stderr = ";
            product += innerRepresentation.Error;
            product += " )\n";
        }
        if (innerRepresentation.TotalCPUTime != -1) {
            std::string outputValue;
            std::stringstream ss;
            ss << innerRepresentation.TotalCPUTime.GetPeriod();
            ss >> outputValue;
            product += "( cputime = ";//TODO:
            product += outputValue;
            product += " )\n";
        }
        if (innerRepresentation.TotalWallTime != -1) {
            std::string outputValue;
            std::stringstream ss;
            ss << innerRepresentation.TotalWallTime.GetPeriod();
            ss >> outputValue;
            product += "( walltime = ";//TODO:
            product += outputValue;
            product += " )\n";
        }
        if (innerRepresentation.IndividualPhysicalMemory != -1) {
            std::string outputValue;
            std::stringstream ss;
            ss << innerRepresentation.IndividualPhysicalMemory;
            ss >> outputValue;
            product += "( memory = ";
            product += outputValue;
            product += " )\n";
        }
        if (!innerRepresentation.Environment.empty()) {
            for (std::list<Arc::EnvironmentType>::const_iterator it = innerRepresentation.Environment.begin(); 
                 it != innerRepresentation.Environment.end(); it++) {
               product += "( environment = (";
               product += (*it).value + " " + (*it).name_attribute;
               product += ") )\n";
            }
        }
        if (!innerRepresentation.File.empty()) {
            product += "( inputfiles =";//TODO:
            std::list<Arc::FileType>::const_iterator iter;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                std::list<Arc::SourceType>::const_iterator it_source;
                for (it_source = (*iter).Source.begin(); it_source != (*iter).Source.end(); it_source++){
                    product += " (" + (*iter).Name;
//TODO:StagingInBaseURI added
                    product += " " +  (*it_source).URI.fullstr() + " )";
                }
            }
            product += " )\n";
        }
        if (!innerRepresentation.File.empty()) {
            product += "( executables =";
            std::list<Arc::FileType>::const_iterator iter;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                std::list<Arc::TargetType>::const_iterator it_target;
                if ( (*iter).IsExecutable ) {
                    product += " \"" + (*iter).Name + "\"";
                }
            }
            product += " )\n";
        }
        if (!innerRepresentation.File.empty()) {
            product += "( outputfiles =";
            std::list<Arc::FileType>::const_iterator iter;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                std::list<Arc::TargetType>::const_iterator it_target;
                for (it_target = (*iter).Target.begin(); it_target != (*iter).Target.end(); it_target++){
                    product += " (" + (*iter).Name;
//TODO:StagingOutBaseURI added
                    product += " " +  (*it_target).URI.fullstr() + " )";
                }
            }
            product += " )\n";
        }
        if (!innerRepresentation.QueueName.empty()) {
            product += "( queue = ";
            product +=  innerRepresentation.QueueName;
            product += " )\n";
        }
        if (innerRepresentation.LRMSReRun > -1) {
            product += "( rerun = ";
            std::ostringstream oss;
            oss << innerRepresentation.LRMSReRun;
            product += oss.str();
            product += " )\n";
        }
        if (innerRepresentation.SessionLifeTime != -1) {
            std::string outputValue;
            std::stringstream ss;
            ss << innerRepresentation.SessionLifeTime.GetPeriod();
            ss >> outputValue;
            product += "( lifetime = ";
            product += outputValue;
            product += " )\n";
        }
        if (innerRepresentation.DiskSpace > -1) {
            product += "( disk = ";//TODO:
            std::ostringstream oss;
            oss << innerRepresentation.DiskSpace;
            product += oss.str();
            product += " )\n";
        }
        if (!innerRepresentation.RunTimeEnvironment.empty()) {
            std::list<Arc::RunTimeEnvironmentType>::const_iterator iter;
            for (iter = innerRepresentation.RunTimeEnvironment.begin();
                 iter != innerRepresentation.RunTimeEnvironment.end(); iter++){
                product += "( runtimeenvironment = ";//TODO:
                product +=  (*iter).Name;
                product += " )\n";
            }
        }
        if (!innerRepresentation.OSName.empty()) {
            product += "( opsys = ";
            product +=  innerRepresentation.OSName;
            product += " )\n";
        }
        if (!innerRepresentation.Platform.empty()) {
            product += "( architecture = ";//TODO:
            product +=  innerRepresentation.Platform;
            product += " )\n";
        }
        if (innerRepresentation.ProcessPerHost > -1) {
            product += "( count = ";
            std::ostringstream oss;
            oss << innerRepresentation.ProcessPerHost;
            product += oss.str();
            product += " )\n";
        }
        if (innerRepresentation.ProcessingStartTime != -1) {
            product += "( starttime = ";
            product += innerRepresentation.ProcessingStartTime.str(Arc::MDSTime);
            product += " )\n";
        }
        if (innerRepresentation.Join) {
            product += "( join = true )\n";//TODO:
        }
        if (!innerRepresentation.LogDir.empty()) {
            product += "( bmlog = ";
            product +=  innerRepresentation.LogDir;
            product += " )\n";
        }
        if (!innerRepresentation.JobName.empty()) {
            product += "( jobname = ";
            product +=  innerRepresentation.JobName;
            product += " )\n";
        }
        if (bool(innerRepresentation.AccessControl)) {
            product += "( acl = ";
            std::string acl;
            innerRepresentation.AccessControl.GetXML(acl, true);
            product +=  acl;
            product += " )\n";
        }
        if (!innerRepresentation.Notification.empty()) {
            for (std::list<Arc::NotificationType>::const_iterator it=innerRepresentation.Notification.begin();
                                     it!=innerRepresentation.Notification.end(); it++) {
                product += "( notify = ";
                for (std::list<std::string>::const_iterator iter=(*it).State.begin();
                            iter!=(*it).State.end(); iter++) {
                     if ( *iter == "PREPARING" ) { product +=  "b"; }
                     else if ( *iter == "INLRMS" ) { product +=  "q"; }
                     else if ( *iter == "FINISHING" ) { product +=  "f"; }
                     else if ( *iter == "FINISHED" ) { product +=  "e"; }
                     else if ( *iter == "CANCELLED" ) { product +=  "c"; }
                     else if ( *iter == "DELETED" ) { product +=  "d"; }
                     else { std::cerr << "Invalid State [" <<  *iter << "] in the Notification!" << std::endl; }
                }
                for (std::list<std::string>::const_iterator iter=(*it).Address.begin();
                            iter!=(*it).Address.end(); iter++) {
                     product +=  " " + (*iter);
                }
                product += " )\n";
            }
        }
        if (bool(innerRepresentation.RemoteLogging)) {
            product += "( jobreport = ";
            product +=  innerRepresentation.RemoteLogging.fullstr();
            product += " )\n";
        }
        if (bool(innerRepresentation.CredentialService)) {
            product += "( credentialserver = ";
            product +=  innerRepresentation.CredentialService.fullstr();
            product += " )\n";
        }
        if (!innerRepresentation.File.empty()) {
            bool _true = false;
            bool _false = false;
            std::list<Arc::FileType>::const_iterator iter;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                std::list<Arc::TargetType>::const_iterator it_target;
                if ( (*iter).DownloadToCache ){
                    _true = true;
                }
                else {
                    _false = true;
                }
            }
            if ( _true && !_false) {
                product += "( cahce = \"true\" )\n";
            }
        }
        if (!innerRepresentation.ReferenceTime.value.empty()) {
            product += "( benchmarks = (\"";//TODO:
            if ( !innerRepresentation.ReferenceTime.benchmark_attribute.empty() && 
                 !innerRepresentation.ReferenceTime.value_attribute.empty()) {
                product += innerRepresentation.ReferenceTime.benchmark_attribute;
                product += "\" \"" + innerRepresentation.ReferenceTime.value;
                product += "\" \"" + innerRepresentation.ReferenceTime.value_attribute;
            }
            else {
                product += "frequency\" \"2,8GHz\" \"" + innerRepresentation.ReferenceTime.value;
            }
            product += "\") )\n";
        }
        if (!innerRepresentation.File.empty() || !innerRepresentation.Directory.empty()) {
            bool set_Threads = false;
            int minimum;

            // all File elements checking
            std::list<Arc::FileType>::const_iterator iter;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                // checking the Source elements
                std::list<Arc::SourceType>::const_iterator it_source;
                for (it_source = (*iter).Source.begin(); it_source != (*iter).Source.end(); it_source++){
                    if ( (*it_source).Threads > -1 && !set_Threads ) {
                       set_Threads = true;
                       minimum = (*it_source).Threads;
                    }
                    else if ( (*it_source).Threads > -1 && set_Threads && (*it_source).Threads < minimum ) {
                       minimum = (*it_source).Threads;
                    }
                }
                // checking the Target elements
                std::list<Arc::TargetType>::const_iterator it_target;
                for (it_target = (*iter).Target.begin(); it_target != (*iter).Target.end(); it_target++){
                    if ( (*it_target).Threads > -1 && !set_Threads ) {
                       set_Threads = true;
                       minimum = (*it_target).Threads;
                    }
                    else if ( (*it_target).Threads > -1 && set_Threads && (*it_target).Threads < minimum ) {
                       minimum = (*it_target).Threads;
                    }
                }
            }
            // all Directory elements checking
            std::list<Arc::DirectoryType>::const_iterator iter_d;
            for (iter_d = innerRepresentation.Directory.begin(); iter_d != innerRepresentation.Directory.end(); iter_d++){
                // checking the Source elements
                std::list<Arc::SourceType>::const_iterator it_source;
                for (it_source = (*iter_d).Source.begin(); it_source != (*iter_d).Source.end(); it_source++){
                    if ( (*it_source).Threads > -1 && !set_Threads ) {
                       set_Threads = true;
                       minimum = (*it_source).Threads;
                    }
                    else if ( (*it_source).Threads > -1 && set_Threads && (*it_source).Threads < minimum ) {
                       minimum = (*it_source).Threads;
                    }
                }
                // checking the Target elements
                std::list<Arc::TargetType>::const_iterator it_target;
                for (it_target = (*iter_d).Target.begin(); it_target != (*iter_d).Target.end(); it_target++){
                    if ( (*it_target).Threads > -1 && !set_Threads ) {
                       set_Threads = true;
                       minimum = (*it_target).Threads;
                    }
                    else if ( (*it_target).Threads > -1 && set_Threads && (*it_target).Threads < minimum ) {
                       minimum = (*it_target).Threads;
                    }
                }
            }

            std::ostringstream oss;
            oss << minimum;
            if ( set_Threads ) product += "( ftpthreads = \"" + oss.str() +"\" )\n" ;
        }
        if (bool(innerRepresentation.DataIndexingService )) {
            product += "( replicacollection = \"";
            product +=  innerRepresentation.DataIndexingService .fullstr();
            product += "\" )\n";
        }
        if (innerRepresentation.InBound) {
            product += "( nodeaccess = \"inbound\")\n";//TODO:
        }
        if (innerRepresentation.OutBound) {
            product += "( nodeaccess = \"outbound\")\n";//TODO:
        }
        if (!innerRepresentation.ReferenceTime.value.empty() && 
             innerRepresentation.ReferenceTime.value_attribute.empty() && 
             innerRepresentation.ReferenceTime.benchmark_attribute.empty() && 
             innerRepresentation.TotalCPUTime == -1 && 
             innerRepresentation.TotalWallTime == -1 ) {
            product += "( gridtime = \"";//TODO:
            product += innerRepresentation.ReferenceTime.value;
            product += "\" )\n";
        }
        if ( !innerRepresentation.XRSL_elements.empty() ) {
          std::map<std::string, std::string>::const_iterator it;
          for ( it = innerRepresentation.XRSL_elements.begin(); it != innerRepresentation.XRSL_elements.end(); it++ ) {
              product += "( ";
              product += it->first;
              product += " = \"";
              product += it->second;
              product += "\" )\n";
          }
        }
        if ( DEBUGX ) std::cerr << "[XRSLParser] Converting to XRSL" << std::endl;
        return true;
    }

    bool JDLParser::splitJDL(std::string original_string, std::vector<std::string>& lines) {

        // Clear the return variable
        lines.clear();
      
        std::string jdl_text = original_string;
      
        bool quotation = false;
        std::vector<char> stack;
        std::string actual_line;
  
        for (int i=0; i<jdl_text.size()-1; i++) {
            // Looking for control character marks the line end
            if ( jdl_text[i] == ';' && !quotation && stack.empty() ) {
                lines.push_back( actual_line );
                actual_line.clear();
                continue;
            }
            // Determinize the quotations
            if ( jdl_text[i] == '"' ) {
                if ( !quotation ) quotation = true;
                else if ( jdl_text[i-1] != '\\' ) quotation = false;
            }
            if ( !quotation ) {
                if ( jdl_text[i] == '{' || jdl_text[i] == '[' ) stack.push_back( jdl_text[i] );
                if ( jdl_text[i] == '}' ) {
                    if ( stack.back() == '{') stack.pop_back();
                    else return false;
                }
                if ( jdl_text[i] == ']' ) {
                    if ( stack.back() == '[') stack.pop_back();
                    else return false;
                }
            }
            actual_line += jdl_text[i];
        }
        return true;
    }

    std::string JDLParser::simpleJDLvalue(std::string attributeValue) {
        std::string whitespaces (" \t\f\v\n\r");
        unsigned long last_pos = attributeValue.find_last_of( "\"" );
        // If the text is not between quotation marks, then return with the original form
        if (attributeValue.substr( attributeValue.find_first_not_of( whitespaces ), 1 ) != "\"" || last_pos == std::string::npos )
            return sm.trim( attributeValue );
        // Else remove the marks and return with the quotation's content
        else 
            return attributeValue.substr( attributeValue.find_first_of( "\"" )+1,  last_pos - attributeValue.find_first_of( "\"" ) -1 );
    }

    std::vector<std::string> JDLParser::listJDLvalue( std::string attributeValue ) {
        std::vector<std::string> elements;
        unsigned long first_bracket = attributeValue.find_first_of( "{" );
        if ( first_bracket == std::string::npos ) {
            elements.push_back( simpleJDLvalue( attributeValue ) );
            return elements;
        }
        unsigned long last_bracket = attributeValue.find_last_of( "}" );
        if ( last_bracket == std::string::npos ) {
            elements.push_back( simpleJDLvalue( attributeValue ) );
            return elements;
        }
        attributeValue = attributeValue.substr(first_bracket+1, last_bracket - first_bracket - 1 );
        std::vector<std::string> listElements = sm.split( attributeValue, "," );
        for (std::vector<std::string>::const_iterator it=listElements.begin(); it<listElements.end(); it++) {
            elements.push_back( simpleJDLvalue( *it ) );
        }
        return elements;
    }

    bool JDLParser::handleJDLattribute(std::string attributeName, std::string attributeValue, Arc::JobInnerRepresentation& innerRepresentation) {

        // To do the attributes name case-insensitive do them lowercase and remove the quotiation marks
        attributeName = sm.toLowerCase( attributeName );
        if ( attributeName == "type" ) {
            std::string value = sm.toLowerCase( simpleJDLvalue( attributeValue ) );
            if ( value == "job" ) return true;
            if ( value == "dag" ) {
                if ( DEBUGX ) std::cerr << "[JDLParser] This kind of JDL decriptor is not supported yet: " << value << std::endl;
                return false; // This kind of JDL decriptor is not supported yet
            }
            if ( value == "collection" ) {
                if ( DEBUGX ) std::cerr << "[JDLParser] This kind of JDL decriptor is not supported yet: " << value << std::endl;
                return false; // This kind of JDL decriptor is not supported yet
            }
            if ( DEBUGX ) std::cerr << "[JDLParser] Attribute name: " << attributeName << ", has unknown value: " << value << std::endl;
            return false; // Unknown attribute value - error
        } else if ( attributeName == "jobtype") {
            return true; // Skip this attribute
        } else if ( attributeName == "executable" ) {
            innerRepresentation.Executable = simpleJDLvalue( attributeValue );
            return true;
        } else if ( attributeName == "arguments" ) {
            std::string value = simpleJDLvalue( attributeValue );
            std::vector<std::string> parts = sm.split( value, " " );
            for ( std::vector<std::string>::const_iterator it = parts.begin(); it < parts.end(); it++ ) {
                innerRepresentation.Argument.push_back( (*it) );
            }
            return true;
        } else if ( attributeName == "stdinput" ) {
            innerRepresentation.Input =  simpleJDLvalue( attributeValue );
            return true;
        } else if ( attributeName == "stdoutput" ) {
            innerRepresentation.Output = simpleJDLvalue( attributeValue );
            return true;
        } else if ( attributeName == "stderror" ) {
            innerRepresentation.Error = simpleJDLvalue( attributeValue );
            return true;
        } else if ( attributeName == "inputsandbox" ) {
            std::vector<std::string> inputfiles = listJDLvalue( attributeValue );
            for (std::vector<std::string>::const_iterator it=inputfiles.begin(); it<inputfiles.end(); it++) {
                Arc::FileType file;
                file.Name = (*it);
                Arc::SourceType source;
                Arc::URL url;
                url.ChangePath(*it);
                source.URI = url;
                source.Threads = -1;
                file.Source.push_back(source);
                //initializing this variables
                file.KeepData = false;
                file.IsExecutable = false;
                file.DownloadToCache = false;
                innerRepresentation.File.push_back(file);
            }
            return true;
        } else if ( attributeName == "inputsandboxbaseuri" ) {
            Arc::URL url(simpleJDLvalue( attributeValue ));
            innerRepresentation.StagingInBaseURI = url;
            return true;
        } else if ( attributeName == "outputsandbox" ) {
            std::vector<std::string> inputfiles = listJDLvalue( attributeValue );
            for (std::vector<std::string>::const_iterator it=inputfiles.begin(); it<inputfiles.end(); it++) {
                Arc::FileType file;
                file.Name = (*it);
                Arc::TargetType target;
                Arc::URL url;
                url.ChangePath(*it);
                target.URI = url;
                target.Threads = -1;
                target.Mandatory = false;
                target.NeededReplicas = -1;
                file.Target.push_back(target);
                //initializing this variables
                file.KeepData = false;
                file.IsExecutable = false;
                file.DownloadToCache = false;
                innerRepresentation.File.push_back(file);
            }
            return true;
        } else if ( attributeName == "outputsandboxdesturi" ) {
            std::vector< std::string > value = listJDLvalue( attributeValue );
            int i=0;
            for (std::list<Arc::FileType>::iterator it=innerRepresentation.File.begin(); 
                                                          it!=innerRepresentation.File.end(); it++) {
                if ( !(*it).Target.empty() ) {
                   if ( i < value.size() ) {
                      Arc::URL url;
                      url.ChangePath(value[i]);
                      (*it).Target.begin()->URI = url;
                      i++;
                   }
                   else {
                      std::cerr << "Not enought outputsandboxdesturi element!" << std::endl;
                      return false;
                   }

                }
            }
            return true;
        } else if ( attributeName == "outputsandboxbaseuri" ) {
            Arc::URL url(simpleJDLvalue( attributeValue ));
            innerRepresentation.StagingOutBaseURI = url;
            return true;
        } else if ( attributeName == "batchsystem" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["BatchSystem"] = "\"" + simpleJDLvalue( attributeValue ) + "\""; 
            return true;
        } else if ( attributeName == "prologue" ) {
            innerRepresentation.Prologue.Name = simpleJDLvalue( attributeValue );
            return true;
        } else if ( attributeName == "prologuearguments" ) {
            std::string value = simpleJDLvalue( attributeValue );
            std::vector<std::string> parts = sm.split( value, " " );
            for ( std::vector<std::string>::const_iterator it = parts.begin(); it < parts.end(); it++ ) {
                innerRepresentation.Prologue.Arguments.push_back( *it );
            }
            return true;
        } else if ( attributeName == "epilogue" ) {
            innerRepresentation.Epilogue.Name = simpleJDLvalue( attributeValue );
            return true;    
        } else if ( attributeName == "epiloguearguments" ) {
            std::string value = simpleJDLvalue( attributeValue );
            std::vector<std::string> parts = sm.split( value, " " );
            for ( std::vector<std::string>::const_iterator it = parts.begin(); it < parts.end(); it++ ) {
                innerRepresentation.Epilogue.Arguments.push_back( *it );
            }
            return true;
        } else if ( attributeName == "allowzippedisb" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["AllowZippedISB"] = simpleJDLvalue( attributeValue ); 
            return true;
        } else if ( attributeName == "zippedisb" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["ZippedISB"] = "\"" + simpleJDLvalue( attributeValue ) + "\""; 
            return true;
        } else if ( attributeName == "expirytime" ) {
            int lifeTimeSeconds = atoi( simpleJDLvalue( attributeValue ).c_str() );
            int nowSeconds = time(NULL);
            std::stringstream ss;
            ss << lifeTimeSeconds - nowSeconds;
            ss >> attributeValue;
    
            Period time( attributeValue );
            innerRepresentation.SessionLifeTime = time;
            return true;
        } else if ( attributeName == "environment" ) {
            std::vector<std::string> variables = listJDLvalue( attributeValue );
    
            for (std::vector<std::string>::const_iterator it = variables.begin(); it < variables.end(); it++) {
                unsigned long equal_pos = (*it).find_first_of( "=" );
                if ( equal_pos != std::string::npos ) {
                    Arc::EnvironmentType env;
                    env.name_attribute = (*it).substr( 0,equal_pos );
                    env.value = (*it).substr( equal_pos+1, std::string::npos );
                    innerRepresentation.Environment.push_back(env);
                } else {
                    if ( DEBUGX ) std::cerr << "[JDLParser] Environment variable has been defined without any equal sign." << std::endl;
                    return false;
                }
            }
            return true;
        } else if ( attributeName == "perusalfileenable" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["PerusalFileEnable"] = simpleJDLvalue( attributeValue ); 
            return true;
        } else if ( attributeName == "perusaltimeinterval" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["PerusalTimeInterval"] = simpleJDLvalue( attributeValue ); 
            return true;
        } else if ( attributeName == "perusalfilesdesturi" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["PerusalFilesDestURI"] = "\"" + simpleJDLvalue( attributeValue ) + "\""; 
            return true;    
        } else if ( attributeName == "inputdata" ) {
            // Not supported yet
            // will be soon deprecated
            return true;
        } else if ( attributeName == "outputdata" ) {
            // Not supported yet
            // will be soon deprecated
            return true;
        } else if ( attributeName == "storageindex" ) {
            // Not supported yet
            // will be soon deprecated
            return true;
        } else if ( attributeName == "datacatalog" ) {
            // Not supported yet
            // will be soon deprecated
            return true;
        } else if ( attributeName == "datarequirements" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["DataRequirements"] = simpleJDLvalue( attributeValue ); 
            return true;
        } else if ( attributeName == "dataaccessprotocol" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["DataAccessProtocol"] = simpleJDLvalue( attributeValue ); 
            return true;
        } else if ( attributeName == "virtualorganisation" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["VirtualOrganisation"] = "\"" + simpleJDLvalue( attributeValue ) + "\""; 
            return true;
        } else if ( attributeName == "queuename" ) {
            innerRepresentation.QueueName = simpleJDLvalue( attributeValue );
            return true;
        } else if ( attributeName == "retrycount" ) {
            int count = atoi( simpleJDLvalue( attributeValue ).c_str() );
            if ( innerRepresentation.LRMSReRun > count ) count = innerRepresentation.LRMSReRun;
            innerRepresentation.LRMSReRun = count;
            return true;
        } else if ( attributeName == "shallowretrycount" ) {
            int count = atoi( simpleJDLvalue( attributeValue ).c_str() );
            if ( innerRepresentation.LRMSReRun > count ) count = innerRepresentation.LRMSReRun;
            innerRepresentation.LRMSReRun = count;
            return true;
        } else if ( attributeName == "lbaddress" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["LBAddress"] = "\"" + simpleJDLvalue( attributeValue ) +"\""; 
            return true;
        } else if ( attributeName == "myproxyserver" ) {
            Arc::URL url(simpleJDLvalue( attributeValue ));
            innerRepresentation.CredentialService = url;
            return true;
        } else if ( attributeName == "hlrlocation" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["HLRLocation"] = "\"" + simpleJDLvalue( attributeValue ) +"\""; 
            return true;
        } else if ( attributeName == "jobprovenance" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["JobProvenance"] = "\"" + simpleJDLvalue( attributeValue ) +"\""; 
            return true;
        } else if ( attributeName == "nodenumber" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["NodeNumber"] = simpleJDLvalue( attributeValue ); 
            return true;
        } else if ( attributeName == "jobsteps" ) {
            // Not supported yet
            // will be soon deprecated
            return true;
        } else if ( attributeName == "currentstep" ) {
            // Not supported yet
            // will be soon deprecated
            return true;
        } else if ( attributeName == "jobstate" ) {
            // Not supported yet
            // will be soon deprecated
            return true;
        } else if ( attributeName == "listenerport" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["ListenerPort"] = simpleJDLvalue( attributeValue ); 
            return true;    
        } else if ( attributeName == "listenerhost" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["ListenerHost"] = "\"" + simpleJDLvalue( attributeValue ) +"\""; 
            return true;
        } else if ( attributeName == "listenerpipename" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["ListenerPipeName"] = "\"" + simpleJDLvalue( attributeValue ) +"\""; 
            return true;
        } else if ( attributeName == "requirements" ) {
            // It's too complicated to determinize the right conditions, because the definition language is
            // LRMS specific.
            // Only store it.
            innerRepresentation.JDL_elements["Requirements"] = "\"" + simpleJDLvalue( attributeValue ) +"\""; 
            return true;
        } else if ( attributeName == "rank" ) {
            innerRepresentation.Rank = simpleJDLvalue( attributeValue );
            return true;
        } else if ( attributeName == "fuzzyrank" ) {
            bool fuzzyrank = false;
            if (  Arc::upper(simpleJDLvalue( attributeValue )) == "TRUE") fuzzyrank = true;
            innerRepresentation.FuzzyRank = fuzzyrank;
            return true;
        } else if ( attributeName == "usertags" ) {
            // They have no standard and no meaning.
            innerRepresentation.UserTag.push_back( simpleJDLvalue( attributeValue ) );
            return true;
        } else if ( attributeName == "outputse" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["OutputSE"] = "\"" + simpleJDLvalue( attributeValue ) +"\""; 
            return true;
        } else if ( attributeName == "shortdeadlinejob" ) {
            // Not supported yet, only store it
            innerRepresentation.JDL_elements["ShortDeadlineJob"] = simpleJDLvalue( attributeValue ); 
            return true;
        }
        if ( DEBUGX ) std::cerr << "[JDL Parser]: Unknown attribute name: '" << attributeName << "', with value: " << attributeValue << std::endl;
        return false;
    }

    bool JDLParser::parse( Arc::JobInnerRepresentation& innerRepresentation, const std::string source ) {
        unsigned long first = source.find_first_of( "[" );
        unsigned long last = source.find_last_of( "]" );
        if ( first == std::string::npos || last == std::string::npos ) {
            if ( DEBUGX ) std::cerr << "[JDLParser] There is at least one necessary ruler character missing. ('[' or ']')" << std::endl;
            return false;
        }
        std::string input_text = source.substr( first+1, last-first-1 );

        //Remove multiline comments
        unsigned long comment_start = 0;
        while ( (comment_start = input_text.find("/*", comment_start)) != std::string::npos) {
            input_text.erase(input_text.begin() + comment_start, input_text.begin() + input_text.find("*/", comment_start) + 2);
        }

        std::string wcpy = "";
        std::vector<std::string> lines = sm.split(input_text, "\n");
        for ( unsigned long i=0; i < lines.size(); ) {
            // Remove empty lines
            std::string trimmed_line = sm.trim(lines[i]);
            if ( trimmed_line.length() == 0) lines.erase(lines.begin() +i);
            // Remove lines starts with '#' - Comments
            else if ( trimmed_line.length() >= 1 && trimmed_line.substr(0,1) == "#") lines.erase(lines.begin() +i);
            // Remove lines starts with '//' - Comments
            else if ( trimmed_line.length() >= 2 && trimmed_line.substr(0,2) == "//") lines.erase(lines.begin() +i);
            else {
                wcpy += lines[i++] + "\n";
            }
        }
  
        if ( !splitJDL(wcpy, lines) ) {
            if ( DEBUGX ) std::cerr << "[JDLParser] Syntax error found during the split function." << std::endl;
            return false;
        }
        if (lines.size() <= 0) {
            if ( DEBUGX ) std::cerr << "[JDLParser] Lines count is zero or other funny error has occurred." << std::endl;
            return false;
        }
        
        for ( unsigned long i=0; i < lines.size(); i++) {
            unsigned long equal_pos = lines[i].find_first_of("=");
            if ( equal_pos == std::string::npos ) {
                if ( i == lines.size()-1 ) continue;
                else {
                    if ( DEBUGX ) std::cerr << "[JDLParser] JDL syntax error. There is at least one equal sign missing where it would be expected." << std::endl;
                    return false;
                }
            }    
            if (!handleJDLattribute( sm.trim( lines[i].substr(0, equal_pos) ), sm.trim( lines[i].substr(equal_pos+1, std::string::npos) ), innerRepresentation)) return false;
        }
        return true;
    }

    bool JDLParser::getProduct( const Arc::JobInnerRepresentation& innerRepresentation, std::string& product ) {
        product = "[\n  Type = \"job\";\n";
        if (!innerRepresentation.Executable.empty()) {
            product += "  Executable = \"";
            product += innerRepresentation.Executable;
            product += "\";\n";
        }
        if (!innerRepresentation.Argument.empty()) {
            product += "  Arguments = \"";
            bool first = true;
            for (std::list<std::string>::const_iterator it=innerRepresentation.Argument.begin();
                            it!=innerRepresentation.Argument.end(); it++) {
                 if ( !first ) product += " ";
                else first = false;
                product += *it;
            }
            product += "\";\n";
        }
        if (!innerRepresentation.Input.empty()) {
            product += "  StdInput = \"";
            product += innerRepresentation.Input;
            product += "\";\n";
        }
        if (!innerRepresentation.Output.empty()) {
            product += "  StdOutput = \"";
            product += innerRepresentation.Output;
            product += "\";\n";
        }
        if (!innerRepresentation.Error.empty()) {
            product += "  StdError = \"";
            product += innerRepresentation.Error;
            product += "\";\n";
        }
        if (!innerRepresentation.Environment.empty()) {
            product += "  Environment = {\n";
            for (std::list<Arc::EnvironmentType>::const_iterator it = innerRepresentation.Environment.begin(); 
                 it != innerRepresentation.Environment.end(); it++) {
                product += "    \"" + (*it).name_attribute + "=" + (*it).value + "\"";
                if (it++ != innerRepresentation.Environment.end()) { 
                  product += ",";
                  it--;
                }
                product += "\n";
            }
            product += "    };\n";
        }
        if (!innerRepresentation.Prologue.Name.empty()) {
            product += "  Prologue = \"";
            product += innerRepresentation.Prologue.Name.empty();
            product += "\";\n";
        }
        if (!innerRepresentation.Prologue.Arguments.empty()) {
            product += "  PrologueArguments = \"";
            bool first = true;
            for (std::list<std::string>::const_iterator iter = innerRepresentation.Prologue.Arguments.begin();
                               iter != innerRepresentation.Prologue.Arguments.end(); iter++){
                if ( !first ) product += " ";
                else first = false;
                product += *iter;
            }
            product += "\";\n";
        }
        if (!innerRepresentation.Epilogue.Name.empty()) {
            product += "  Epilogue = \"";
            product += innerRepresentation.Epilogue.Name;
            product += "\";\n";
        }
        if (!innerRepresentation.Epilogue.Arguments.empty()) {
            product += "  EpilogueArguments = \"";
            bool first = true;
            for (std::list<std::string>::const_iterator iter = innerRepresentation.Epilogue.Arguments.begin();
                               iter != innerRepresentation.Epilogue.Arguments.end(); iter++){
                if ( !first ) product += " ";
                else first = false;
                product += *iter;
            }
            product += "\";\n";
        }
        if (!innerRepresentation.File.empty()) {
            product += "  InputSandbox = {\n";
            std::list<Arc::FileType>::const_iterator iter;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                std::list<Arc::SourceType>::const_iterator it_source;
                for (it_source = (*iter).Source.begin(); it_source != (*iter).Source.end(); it_source++){
                    product += "    \"" +  (*it_source).URI.fullstr() + "\"";
                    if (it_source++ != (*iter).Source.end()) {
                       product += ",";
                       it_source--;
                    }
                    product += "\n";
                }
            }
            product += "    };\n";
         }
        if (bool(innerRepresentation.StagingInBaseURI)) {
            product += "  InputSandboxBaseURI = \"";
            product +=  innerRepresentation.StagingInBaseURI.fullstr();
            product += "\";\n";
        }
        if (!innerRepresentation.File.empty()) {
            product += "  OutputSandbox = {";
            std::list<Arc::FileType>::const_iterator iter;
            bool first = true;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                if ( !(*iter).Target.empty()) {
                   if (!first) {
                      product += ",";
                   }
                   else {
                      first = false;
                   }
                   product += " \"" +  (*iter).Name + "\"";
                }
            }
            product += " };\n";
        }
        if (!innerRepresentation.File.empty()) {
            product += "  OutputSandboxDestURI = {\n";
            std::list<Arc::FileType>::const_iterator iter;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                std::list<Arc::TargetType>::const_iterator it_target;
                for (it_target = (*iter).Target.begin(); it_target != (*iter).Target.end(); it_target++){
                    product += "    \"" +  (*it_target).URI.fullstr() + "\"";
                    if (it_target++ != (*iter).Target.end()) {
                       product += ",";
                       it_target--;
                    }
                    product += "\n";
                }
            }
            product += "    };\n";
        }
        if (bool(innerRepresentation.StagingOutBaseURI)) {
            product += "  OutputSandboxBaseURI = \"";
            product +=  innerRepresentation.StagingOutBaseURI.fullstr();
            product += "\";\n";
        }
        if (!innerRepresentation.QueueName.empty()) {
            product += "  QueueName = \"";
            product +=  innerRepresentation.QueueName;
            product += "\";\n";
        }
        if (innerRepresentation.LRMSReRun > -1) {
            product += "  RetryCount = \"";
            std::ostringstream oss;
            oss << innerRepresentation.LRMSReRun;
            product += oss.str();
            product += "\";\n";
        }
        if (innerRepresentation.SessionLifeTime != -1) {
            std::string outputValue;
            int attValue = atoi( ((std::string) innerRepresentation.SessionLifeTime).c_str() );
            int nowSeconds = time(NULL);
            std::stringstream ss;
            ss << attValue + nowSeconds;
            ss >> outputValue;
            product += "  ExpiryTime = \"";
            product += outputValue;
            product += "\";\n";
        }
        if (bool(innerRepresentation.CredentialService)) {
            product += "  MyProxyServer = \"";
            product +=  innerRepresentation.CredentialService.fullstr();
            product += "\";\n";
        }
        if (!innerRepresentation.Rank.empty()) {
            product += "  Rank = \"";
            product +=  innerRepresentation.Rank;
            product += "\";\n";
        }
        if (innerRepresentation.FuzzyRank) {
            product += "  FuzzyRank = true;\n";
        }
        if (!innerRepresentation.UserTag.empty()) {
            product += "  UserTag = ";
            bool first = true;
            std::list<std::string>::const_iterator iter;
            for (iter = innerRepresentation.UserTag.begin(); iter != innerRepresentation.UserTag.end(); iter++){
                if (!first) {
                   product += ",";
                }
                else {
                   first = false;
                }
                product +=  *iter;
            }
            product += ";\n";
        }
        if ( !innerRepresentation.JDL_elements.empty() ) {
          std::map<std::string, std::string>::const_iterator it;
          for ( it = innerRepresentation.JDL_elements.begin(); it != innerRepresentation.JDL_elements.end(); it++ ) {
              product += "  ";
              product += it->first;
              product += " = ";
              product += it->second;
              product += ";\n";
          }
        }
        product += "]";

        return true;
    }
    
    bool JobDescription::getInnerRepresentation( Arc::JobInnerRepresentation& job ){
        if ( innerRepresentation == NULL) return false;
        job = *innerRepresentation;
        return true;
    }
} // namespace Arc
