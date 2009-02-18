#include <cstring>
#include <algorithm>
#include <sys/stat.h>
#include <fcntl.h>
#include <exception>

#include <arc/StringConv.h>
#include <arc/job/runtimeenvironment.h>
#include <arc/data/CheckSum.h>

#include "JobDescription.h"


namespace Arc {

    Arc::Logger JobDescription::logger(Arc::Logger::getRootLogger(), "jobdescription");

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
        candidate.extensions.push_back("posix");
        candidate.extensions.push_back("jsdl");
        candidate.extensions.push_back("jsdl-arc");
        candidate.extensions.push_back("xml");
        candidate.pattern.push_back("<?xml ");
        candidate.pattern.push_back("<!--");
        candidate.pattern.push_back("-->");
        candidate.pattern.push_back("=\"http://schemas.ggf.org/jsdl/2005/11/jsdl\"");
        candidate.pattern.push_back("=\"http://schemas.ggf.org/jsdl/2005/11/jsdl-posix\"");
        candidate.pattern.push_back("=\"http://www.nordugrid.org/ws/schemas/jsdl-arc\"");
        candidate.pattern.push_back("JobDefinition");
        candidate.pattern.push_back("JobDescription");
        candidate.pattern.push_back("JobIdentification");
        candidate.pattern.push_back("POSIXApplication");
        candidate.pattern.push_back("Executable");
        candidate.pattern.push_back("Argument");
        candidate.pattern.push_back("LocalLogging");
        candidate.pattern.push_back("RemoteLogging");
        candidate.pattern.push_back("URL");
        candidate.pattern.push_back("Reruns");
        candidate.pattern.push_back("CPUTimeLimit");
        candidate.pattern.push_back("WallTimeLimit");
        candidate.pattern.push_back("GridTimeLimit");
        candidate.pattern.push_back("IndividualNetworkBandwidth");
        candidate.pattern.push_back("OperatingSystemName");
        candidate.pattern.push_back("OperatingSystemVersion");
        candidate.pattern.push_back("CPUArchitectureName");
        candidate.pattern.push_back("MemoryLimit");
        candidate.pattern.push_back("VirtualMemoryLimit");
        candidate.pattern.push_back("Middleware");
        candidate.pattern.push_back("ProcessCountLimit");
        candidate.pattern.push_back("TotalCPUCount");
        candidate.pattern.push_back("ThreadCountLimit");
        candidate.pattern.push_back("ApplicationName");
        candidate.pattern.push_back("ApplicationVersion");
        candidate.pattern.push_back("IndividualCPUSpeed");
        candidate.pattern.push_back("IndividualCPUCount");
        candidate.pattern.push_back("TotalPhysicalMemory");
        candidate.pattern.push_back("TotalVirtualMemory");
        candidate.pattern.push_back("TotalDiskSpace");
        candidate.pattern.push_back("TotalResourceCount");
        candidate.pattern.push_back("FileSystemName");
        candidate.pattern.push_back("CreationFlag");
        candidate.pattern.push_back("DeleteOnTermination");
        candidate.pattern.push_back("posix:");
        candidate.pattern.push_back("arc:");

        candidate.negative_pattern.push_back("Meta");
        candidate.negative_pattern.push_back("OptionalElement");
        candidate.negative_pattern.push_back("Author");
        //Save entry
        candidates.push_back(candidate);
        // End of POSIX JSDL

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
        candidate.extensions.push_back("gin");
        candidate.pattern.push_back("<?xml ");
        candidate.pattern.push_back("<!--");
        candidate.pattern.push_back("-->");
        //candidate.pattern.push_back("=\"http://www. /schemas/gin\""); //here can the GIN schema URL
        candidate.pattern.push_back("JobDefinition");
        candidate.pattern.push_back("JobDescription");
        candidate.pattern.push_back("Meta");
        candidate.pattern.push_back("OptionalElement");
        candidate.pattern.push_back("Author");
        candidate.pattern.push_back("DocumentExpiration");
        candidate.pattern.push_back("JobIdentification");
        candidate.pattern.push_back("Application");
        candidate.pattern.push_back("Executable");
        candidate.pattern.push_back("LogDir");
        candidate.pattern.push_back("Argument");
        candidate.pattern.push_back("JobType");
        candidate.pattern.push_back("JobCategory");
        candidate.pattern.push_back("Notification");
        candidate.pattern.push_back("IndividualWallTime");
        candidate.pattern.push_back("ReferenceTime");
        candidate.pattern.push_back("NetworkInfo");
        candidate.pattern.push_back("OSFamily");
        candidate.pattern.push_back("OSName");
        candidate.pattern.push_back("OSVersion");
        candidate.pattern.push_back("Platform");
        candidate.pattern.push_back("CacheDiskSpace");
        candidate.pattern.push_back("SessionDiskSpace");
        candidate.pattern.push_back("Alias");
        candidate.pattern.push_back("EndPointURL");
        candidate.pattern.push_back("Location");
        candidate.pattern.push_back("Country");
        candidate.pattern.push_back("Place");
        candidate.pattern.push_back("PostCode");
        candidate.pattern.push_back("Latitude");
        candidate.pattern.push_back("Longitude");
        candidate.pattern.push_back("CEType");
        candidate.pattern.push_back("Slots");
        candidate.pattern.push_back("Homogeneous");
        candidate.pattern.push_back("File");
        candidate.pattern.push_back("Directory");

        candidate.negative_pattern.push_back("arc:");
        candidate.negative_pattern.push_back("posix:");
        candidate.negative_pattern.push_back("UpperBound");
        candidate.negative_pattern.push_back("LowerBound");
        candidate.negative_pattern.push_back("Exact");
        candidate.negative_pattern.push_back("Range");
        candidate.negative_pattern.push_back("POSIXApplication");
        candidate.negative_pattern.push_back("CPUArchitectureName");
        candidate.negative_pattern.push_back("LocalLogging");
        candidate.negative_pattern.push_back("Directory");
        candidate.negative_pattern.push_back("URL");
        candidate.negative_pattern.push_back("Reruns");
        candidate.negative_pattern.push_back("CreationFlag");
        candidate.negative_pattern.push_back("DeleteOnTermination");
        //Save entry  
        candidates.push_back(candidate);
        // End of JSDL

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
        candidate.pattern.push_back("/*");
        candidate.pattern.push_back(";");
        candidate.pattern.push_back("[");
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
		    JobDescription::logger.msg(DEBUG, "[JobDescriptionOrderer]\tText pattern matching: \"%s\" to %s", *j, (*i).typeName);
                    (*i).priority += TEXT_PATTERN_MATCHING;
                } else {
                    if ( jobDescriptionWithoutWhitespaces.find( sm.toLowerCase(*j) ) != std::string::npos ) {
		        JobDescription::logger.msg(DEBUG, "[JobDescriptionOrderer]\tText pattern matching: \"%s\" to %s", *j, (*i).typeName);
                        (*i).priority += TEXT_PATTERN_MATCHING_WITHOUT_WHITESPACES;
                    }
                }
            }
            for ( std::vector<std::string>::const_iterator j = (*i).negative_pattern.begin(); j != (*i).negative_pattern.end(); j++ ) {
                if ( jobDescription.find( sm.toLowerCase(*j) ) != std::string::npos ) {
		    JobDescription::logger.msg(DEBUG, "[JobDescriptionOrderer]\tText negative_pattern matching: \"%s\" to %s", *j, (*i).typeName);
                    (*i).priority += NEGATIVE_TEXT_PATTERN_MATCHING;
                } else {
                    if ( jobDescriptionWithoutWhitespaces.find( sm.toLowerCase(*j) ) != std::string::npos ) {
	                JobDescription::logger.msg(DEBUG, "[JobDescriptionOrderer]\tText negative_pattern matching: \"%s\" to %s", *j, (*i).typeName);
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
	   JobDescription::logger.msg(DEBUG, "There is nothing in the source. Cannot generate any product.");
           return false;
        }

        jdOrderer.setSource( sourceString );
        std::vector<Candidate> candidates = jdOrderer.getCandidateList();

        // Try to parse the input string in the right order until it once success
        // (If not then just reset the jobTree variable and see the next one)
        for (std::vector<Candidate>::const_iterator it = candidates.begin(); it < candidates.end(); it++) {
            if ( sm.toLowerCase( (*it).typeName ) == "xrsl" ) {
		JobDescription::logger.msg(DEBUG, "[JobDescription] Try to parse as XRSL");
                XRSLParser parser;
                if ( parser.parse( *innerRepresentation, sourceString ) ) {
                    sourceFormat = "xrsl";
                    break;
                }
                //else
                resetJobTree();
            } else if ( sm.toLowerCase( (*it).typeName ) == "posixjsdl" ) {
		JobDescription::logger.msg(DEBUG, "[JobDescription] Try to parse as POSIX JSDL");
                PosixJSDLParser parser;
                if ( parser.parse( *innerRepresentation, sourceString ) ) {
                    sourceFormat = "posixjsdl";
                    break;
                }
                //else
                resetJobTree();
            } else if ( sm.toLowerCase( (*it).typeName ) == "jsdl" ) {
		JobDescription::logger.msg(DEBUG, "[JobDescription] Try to parse as JSDL");
                JSDLParser parser;
                if ( parser.parse( *innerRepresentation, sourceString ) ) {
                    sourceFormat = "jsdl";
                    break;
                }
                //else
                resetJobTree();
            } else if ( sm.toLowerCase( (*it).typeName ) == "jdl" ) {
		JobDescription::logger.msg(DEBUG, "[JobDescription] Try to parse as JDL");
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
	   JobDescription::logger.msg(DEBUG, "The parsing of the source string was unsuccessful.");
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
	   JobDescription::logger.msg(DEBUG, "Error during the XML generation!");
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
	    JobDescription::logger.msg(DEBUG, "There is no successfully parsed source");
            return false;
        }
        if ( sm.toLowerCase( format ) == sm.toLowerCase( sourceFormat ) && 
             sm.toLowerCase( sourceFormat ) != "xrsl" ) {
            product = sourceString;
            return true;
        }
        if ( sm.toLowerCase( format ) == "jdl" ) {
	    JobDescription::logger.msg(DEBUG, "[JobDescription] Generate JDL output");
            JDLParser parser;
            if ( !parser.getProduct( *innerRepresentation, product ) ) {
	       JobDescription::logger.msg(DEBUG, "Generating %s output was unsuccessful", format);
               return false;
            }
            return true;
        } else if ( sm.toLowerCase( format ) == "xrsl" ) {
	    JobDescription::logger.msg(DEBUG, "[JobDescription] Generate XRSL output");
            XRSLParser parser;
            if ( !parser.getProduct( *innerRepresentation, product ) ) {
	       JobDescription::logger.msg(DEBUG, "Generating %s output was unsuccessful", format);
               return false;
            }
            return true;
        } else if ( sm.toLowerCase( format ) == "posixjsdl" ) {
	    JobDescription::logger.msg(DEBUG, "[JobDescription] Generate POSIX JSDL output");
            PosixJSDLParser parser;
            if ( !parser.getProduct( *innerRepresentation, product ) ) {
	       JobDescription::logger.msg(DEBUG, "Generating %s output was unsuccessful", format);
               return false;
            }
            return true;
        } else if ( sm.toLowerCase( format ) == "jsdl" ) {
	    JobDescription::logger.msg(DEBUG, "[JobDescription] Generate JSDL output");
            JSDLParser parser;
            if ( !parser.getProduct( *innerRepresentation, product ) ) {
	       JobDescription::logger.msg(DEBUG, "Generating %s output was unsuccessful", format);
               return false;
            }
            return true;
        } else {
	    JobDescription::logger.msg(DEBUG, "Unknown output format: %s", format) ;
            return false;
        }
    }

    bool JobDescription::getSourceFormat( std::string& _sourceFormat ) {
        if (!isValid()) {
	   JobDescription::logger.msg(DEBUG, "There is no input defined yet or it's format can be determinized.");
           return false;
         } else {
           _sourceFormat = sourceFormat;
           return true;
        } 
    }

    bool JobDescription::getUploadableFiles(std::vector< std::pair< std::string, std::string > >& sourceFiles ) {
        if (!isValid()) {
	   JobDescription::logger.msg(DEBUG, "There is no input defined yet or it's format can be determinized.");
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
       Arc::XMLNode n = range["LowerBoundRange"];
       if ((bool)n) {
          return Arc::stringto<long>((std::string)n);
       }
       n = range["UpperBoundRange"];
       if ((bool)n) {
          return Arc::stringto<long>((std::string)n);
       }
       return -1;
    } 

    // This function is search all elements name of the input XMLNode.
    // When one of them is in the other job description name set then the input XMNNode is not valid job description.
    bool Element_Search( const Arc::XMLNode node, bool posix ){
        std::string parser_name("[JSDLParser]");
        if (posix) parser_name = "[PosixJSDLParser]";
        // check all child nodes
        for (int i=0; bool(node.Child(i)); i++) {
            Arc::XMLNode it(node.Child(i));
            if ( it.Size() > 0 ){
               std::ostringstream message ;
               message << parser_name << " " << it.Name() << " has " << it.Size() << " child(s).";
	       JobDescription::logger.msg(DEBUG, message.str());
            }
            if ( i==0 || i>0 && node.Child(i).Name() != node.Child(i-1).Name() ){
               JobDescription::logger.msg(DEBUG, parser_name+ " element: " + it.Name());
               // The element is not valid in this job description.
               if ( posix ){
                  // POSIX-JSDL
                  int size = sizeof(GIN_elements)/sizeof(std::string) ;
                  if ( find(&GIN_elements[0],&GIN_elements[size-1],it.Name()) != &GIN_elements[size-1]){
                     JobDescription::logger.msg(DEBUG, parser_name + " Not valid JSDL: \"" + it.Name() + "\" is GIN-JSDL element!");
                     return false;
                  }
               }
               else{
                  // GIN-JSDL
                  int size = sizeof(JSDL_elements)/sizeof(std::string) ;
                  if ( find(&JSDL_elements[0],&JSDL_elements[size-1],it.Name()) != &JSDL_elements[size-1]){
                     JobDescription::logger.msg(DEBUG, parser_name + " Not valid GIN-JSDL: \"" + it.Name() + "\" is POSIX-JSDL element!");
                     return false;
                  }
               }
            }
            if ( !Element_Search(it, posix ) ) return false;
        }
        return true;
    }


    bool PosixJSDLParser::parse( Arc::JobInnerRepresentation& innerRepresentation, const std::string source ) { 
        //Source checking
        Arc::XMLNode node(source);
        if ( node.Size() == 0 ){
	    JobDescription::logger.msg(DEBUG, "[PosixJSDLParser] Wrong XML structure! ");
            return false;
        }
        if ( !Arc::Element_Search(node, true ) ) return false;

        // The source parsing start now.
        Arc::NS nsList;
        nsList.insert(std::pair<std::string, std::string>("jsdl","http://schemas.ggf.org/jsdl/2005/11/jsdl"));
        nsList.insert(std::pair<std::string, std::string>("jsdl-posix","http://schemas.ggf.org/jsdl/2005/11/jsdl-posix"));
        nsList.insert(std::pair<std::string, std::string>("jsdl-arc","http://www.nordugrid.org/ws/schemas/jsdl-arc"));
      
        node.Namespaces(nsList);

        Arc::XMLNode jobdescription = node["JobDescription"];

        if (bool(jobdescription["LocalLogging"])) {
           if (bool(jobdescription["LocalLogging"]["Directory"])) {
              innerRepresentation.LogDir = (std::string)jobdescription["LocalLogging"]["Directory"];
           }else {
	      JobDescription::logger.msg(DEBUG, "[PosixJSDLParser] Wrong XML: \"LocalLogging.Directory\" element missed!"); 
              return false;
           }
           if (bool(jobdescription["LocalLogging"][1])){
	      JobDescription::logger.msg(DEBUG, "[PosixJSDLParser] Wrong XML: too many \"LocalLogging\" elements!"); 
              return false;
           }
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

        if (bool(jobdescription["AccessControl"]["Content"])) {
           Arc::XMLNode accesscontrol(jobdescription["AccessControl"]["Content"]);
           accesscontrol.Child(0).New(innerRepresentation.AccessControl);
        }

        if (bool(jobdescription["Notify"])) {
           for (int i=0; bool(jobdescription["Notify"][i]); i++){
               Arc::NotificationType notify;
               for (int j=0; bool(jobdescription["Notify"][i]["Endpoint"][j]); j++){
                    notify.Address.push_back((std::string)jobdescription["Notify"][i]["Endpoint"][j]);
               } 

               for (int j=0; bool(jobdescription["Notify"][i]["State"][j]); j++){
                    notify.State.push_back((std::string)jobdescription["Notify"][i]["State"][j]);
               } 
               innerRepresentation.Notification.push_back(notify);
           }
        }

        // JobIdentification
        Arc::XMLNode jobidentification = node["JobDescription"]["JobIdentification"];

        if (bool(jobidentification["JobName"])) {
           innerRepresentation.JobName = (std::string)jobidentification["JobName"];
        }

        if (bool(jobidentification["Migration"])) {
           for ( int i=0; (bool)(jobidentification["OldJobID"][i]); i++ ) { 
               Arc::WSAEndpointReference old_job_epr(jobidentification["OldJobID"][i]);
               innerRepresentation.Migration.OldJobIDs.push_back(old_job_epr);
           }
           Arc::WSAEndpointReference migration_id(jobidentification["Migration"]["MigrationID"]);
           innerRepresentation.Migration.MigrationID = migration_id;
        }

        if (bool(jobidentification["JobProject"])) {
           innerRepresentation.JobProject = (std::string)jobidentification["JobProject"];
        }
        // end of JobIdentification

        // Application
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
	       JobDescription::logger.msg(DEBUG, "[PosixJSDLParser] Error during the parsing: missed the name attributes of the \"%s\" Environment", (std::string)env);
               return false;
           } 
           Arc::EnvironmentType env_tmp;
           env_tmp.name_attribute = (std::string)name;
           env_tmp.value = (std::string)env;
           innerRepresentation.Environment.push_back(env_tmp);
        }

        if (bool(application["VirtualMemoryLimit"])) {
           if (innerRepresentation.IndividualVirtualMemory < Arc::stringto<int>((std::string)application["VirtualMemoryLimit"]))
              innerRepresentation.IndividualVirtualMemory = Arc::stringto<int>((std::string)application["VirtualMemoryLimit"]);
        }

        if (bool(application["CPUTimeLimit"])) {
           Period time((std::string)application["CPUTimeLimit"]);
           if (innerRepresentation.TotalCPUTime < time)
              innerRepresentation.TotalCPUTime = time;
        }

        if (bool(application["WallTimeLimit"])) {
           Period time((std::string)application["WallTimeLimit"]);
           innerRepresentation.TotalWallTime = time;
        }

        if (bool(application["MemoryLimit"])) {
           if (innerRepresentation.IndividualPhysicalMemory < Arc::stringto<int>((std::string)application["MemoryLimit"]))
              innerRepresentation.IndividualPhysicalMemory = Arc::stringto<int>((std::string)application["MemoryLimit"]);
        }

        if (bool(application["ProcessCountLimit"])) {
           innerRepresentation.Slots = stringtoi((std::string)application["ProcessCountLimit"]);
           innerRepresentation.NumberOfProcesses = stringtoi((std::string)application["ProcessCountLimit"]);
        }

        if (bool(application["ThreadCountLimit"])) {
           innerRepresentation.ThreadPerProcesses = stringtoi((std::string)application["ThreadCountLimit"]);
        }
        // end of Application

        // Resources
        Arc::XMLNode resource = node["JobDescription"]["Resources"];

        if (bool(resource["SessionLifeTime"])) {
           Period time((std::string)resource["SessionLifeTime"]);
           innerRepresentation.SessionLifeTime = time;
        }

        if (bool(resource["TotalCPUTime"])) {
           long value = get_limit(resource["TotalCPUTime"]);
           if (value != -1){
              Period time((time_t)value);
              if (innerRepresentation.TotalCPUTime < time)
                 innerRepresentation.TotalCPUTime = time;
           }
        }

        if (bool(resource["IndividualCPUTime"])) {
           long value = get_limit(resource["IndividualCPUTime"]);
           if (value != -1){
              Period time((time_t)value);
              innerRepresentation.IndividualCPUTime = time;
           }
        }

        if (bool(resource["IndividualPhysicalMemory"])) {
           long value = get_limit(resource["IndividualPhysicalMemory"]);
           if (value != -1)
              if (innerRepresentation.IndividualPhysicalMemory < value)
                 innerRepresentation.IndividualPhysicalMemory = value;
        }

        if (bool(resource["IndividualVirtualMemory"])) {
           long value = get_limit(resource["IndividualVirtualMemory"]);
           if (value != -1)
              if (innerRepresentation.IndividualVirtualMemory < value)
                 innerRepresentation.IndividualVirtualMemory = value;
        }

        if (bool(resource["IndividualDiskSpace"])) {
           long value = get_limit(resource["IndividualDiskSpace"]);
           if (value != -1)
              innerRepresentation.IndividualDiskSpace = value;
        }

        if (bool(resource["FileSystem"]["DiskSpace"])) {
           long value = get_limit(resource["FileSystem"]["DiskSpace"]);
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

        if (bool(resource["Middleware"]["Name"])) {
           innerRepresentation.CEType = (std::string)resource["Middleware"]["Name"];
        }

        if (bool(resource["TotalCPUCount"])) {
           innerRepresentation.ProcessPerHost = (int)get_limit(resource["TotalCPUCount"]);
        }

        if (bool(resource["RunTimeEnvironment"])) {
	   StringManipulator sm;
	   for( int i=0; (bool)(resource["RunTimeEnvironment"][i]); i++ ) {
               Arc::RunTimeEnvironmentType rt;
               std::string version;
               rt.Name = sm.trim((std::string)resource["RunTimeEnvironment"][i]["Name"]);
               version = sm.trim((std::string)resource["RunTimeEnvironment"][i]["Version"]["Exact"]);
               rt.Version.push_back(version);
               innerRepresentation.RunTimeEnvironment.push_back(rt);
           }
        }
        // end of Resources

        // Datastaging
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
                 source.URI = (std::string)source_uri;
                 source.Threads = -1;
                 file.Source.push_back(source);
              }
              if (bool(target_uri)) {
                 Arc::TargetType target;
                 target.URI = (std::string)target_uri;
                 target.Threads = -1;
                 file.Target.push_back(target);
              }

              StringManipulator sm;

              if (sm.toLowerCase(((std::string)ds["IsExecutable"])) == "true"){
                 file.IsExecutable = true;
              }else{
                 file.IsExecutable = false;
              }
              if (sm.toLowerCase(((std::string)ds["DeleteOnTermination"])) == "true"){
                 file.KeepData = false;
              }else{
                 file.KeepData = true;
              }
              file.DownloadToCache = false;
              innerRepresentation.File.push_back(file);
           }
        }
        // end of Datastaging
        return true;
    }

    bool PosixJSDLParser::getProduct( const Arc::JobInnerRepresentation& innerRepresentation, std::string& product ) {
        Arc::NS nsList;        
        nsList.insert(std::pair<std::string, std::string>("jsdl","http://schemas.ggf.org/jsdl/2005/11/jsdl"));
        nsList.insert(std::pair<std::string, std::string>("jsdl-posix","http://schemas.ggf.org/jsdl/2005/11/jsdl-posix"));
        nsList.insert(std::pair<std::string, std::string>("jsdl-arc","http://www.nordugrid.org/ws/schemas/jsdl-arc"));
           
   
        Arc::XMLNode jobdefinition("<JobDefinition/>");
       
        jobdefinition.Namespaces(nsList);

        Arc::XMLNode jobdescription = jobdefinition.NewChild("JobDescription");

        // JobIdentification
        if (!innerRepresentation.JobName.empty()){
           if ( !bool( jobdescription["JobIdentification"] ) ) jobdescription.NewChild("JobIdentification");
           jobdescription["JobIdentification"].NewChild("JobName") = innerRepresentation.JobName;
        }

        if (!innerRepresentation.JobProject.empty()){
           if ( !bool( jobdescription["JobIdentification"] ) ) jobdescription.NewChild("JobIdentification");
           jobdescription["JobIdentification"].NewChild("JobProject") = innerRepresentation.JobProject;
        }
        // end of JobIdentification

        // Application
        if (!innerRepresentation.Executable.empty()){
           if ( !bool( jobdescription["Application"] ) ) jobdescription.NewChild("Application");
           if ( !bool( jobdescription["Application"]["POSIXApplication"] ) ) 
              jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
           jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:Executable") = innerRepresentation.Executable;
        }
        if (!innerRepresentation.Argument.empty()){
           if ( !bool( jobdescription["Application"] ) ) jobdescription.NewChild("Application");
           if ( !bool( jobdescription["Application"]["POSIXApplication"] ) ) 
              jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
           for (std::list<std::string>::const_iterator it=innerRepresentation.Argument.begin();
                 it!=innerRepresentation.Argument.end(); it++) {
               jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:Argument") = *it;
           }
        }
        if (!innerRepresentation.Input.empty()){
           if ( !bool( jobdescription["Application"] ) ) jobdescription.NewChild("Application");
           if ( !bool( jobdescription["Application"]["POSIXApplication"] ) ) 
              jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
           jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:Input") = innerRepresentation.Input;
        }
        if (!innerRepresentation.Output.empty()){
           if ( !bool( jobdescription["Application"] ) ) jobdescription.NewChild("Application");
           if ( !bool( jobdescription["Application"]["POSIXApplication"] ) ) 
              jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
           jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:Output") = innerRepresentation.Output;
        }
        if (!innerRepresentation.Error.empty()){
           if ( !bool( jobdescription["Application"] ) ) jobdescription.NewChild("Application");
           if ( !bool( jobdescription["Application"]["POSIXApplication"] ) ) 
              jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
           jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:Error") = innerRepresentation.Error;
        }
        for (std::list<Arc::EnvironmentType>::const_iterator it=innerRepresentation.Environment.begin();
                 it!=innerRepresentation.Environment.end(); it++) {
            if ( !bool( jobdescription["Application"] ) ) jobdescription.NewChild("Application");
            if ( !bool( jobdescription["Application"]["POSIXApplication"] ) ) 
               jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
            Arc::XMLNode environment = jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:Environment");
            environment = (*it).value;
            environment.NewAttribute("name") = (*it).name_attribute;
        }
        if (innerRepresentation.TotalWallTime != -1){
           if ( !bool( jobdescription["Application"] ) ) jobdescription.NewChild("Application");
           if ( !bool( jobdescription["Application"]["POSIXApplication"] ) ) 
              jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
           Period time((std::string)innerRepresentation.TotalWallTime);
           std::ostringstream oss;
           oss << time.GetPeriod();
           jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:WallTimeLimit") = oss.str();
        }
        if (innerRepresentation.NumberOfProcesses > -1 && 
            innerRepresentation.NumberOfProcesses >= innerRepresentation.Slots){
           if ( !bool( jobdescription["Application"] ) ) jobdescription.NewChild("Application");
           if ( !bool( jobdescription["Application"]["POSIXApplication"] ) ) 
              jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
           if ( !bool( jobdescription["Application"]["POSIXApplication"]["ProcessCountLimit"] ) ) 
              jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:ProcessCountLimit");
           std::ostringstream oss;
           oss << innerRepresentation.NumberOfProcesses;
           jobdescription["Application"]["POSIXApplication"]["ProcessCountLimit"] = oss.str();
        }
        if (innerRepresentation.Slots > -1 && 
            innerRepresentation.Slots >= innerRepresentation.NumberOfProcesses){
           if ( !bool( jobdescription["Application"] ) ) jobdescription.NewChild("Application");
           if ( !bool( jobdescription["Application"]["POSIXApplication"] ) ) 
              jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
           if ( !bool( jobdescription["Application"]["POSIXApplication"]["ProcessCountLimit"] ) ) 
              jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:ProcessCountLimit");
           std::ostringstream oss;
           oss << innerRepresentation.Slots;
           jobdescription["Application"]["POSIXApplication"]["ProcessCountLimit"] = oss.str();
        }
        if (innerRepresentation.ThreadPerProcesses > -1){
           if ( !bool( jobdescription["Application"] ) ) jobdescription.NewChild("Application");
           if ( !bool( jobdescription["Application"]["POSIXApplication"] ) ) 
              jobdescription["Application"].NewChild("jsdl-posix:POSIXApplication");
           std::ostringstream oss;
           oss << innerRepresentation.ThreadPerProcesses;
           jobdescription["Application"]["POSIXApplication"].NewChild("jsdl-posix:ThreadCountLimit") = oss.str();
        }
        // end of Application

        // DataStaging
        for (std::list<Arc::FileType>::const_iterator it=innerRepresentation.File.begin();
                 it!=innerRepresentation.File.end(); it++) {
            Arc::XMLNode datastaging = jobdescription.NewChild("DataStaging");
			StringManipulator sm;
            if ( !(*it).Name.empty())
               datastaging.NewChild("FileName") = (*it).Name;
            if ((*it).Source.size() != 0) {
               std::list<Arc::SourceType>::const_iterator it2;
               it2 = ((*it).Source).begin();
			   if (sm.trim(((*it2).URI).fullstr()) != "" )
                  datastaging.NewChild("Source").NewChild("URI") = ((*it2).URI).fullstr();
            }
            if ((*it).Target.size() != 0) {
               std::list<Arc::TargetType>::const_iterator it3;
               it3 = ((*it).Target).begin();
			   if (sm.trim(((*it3).URI).fullstr()) != "" )
                  datastaging.NewChild("Target").NewChild("URI") = ((*it3).URI).fullstr();
           }      
           if ((*it).IsExecutable || (*it).Name == innerRepresentation.Executable)
              datastaging.NewChild("jsdl-arc:IsExecutable") = "true";
           if ((*it).KeepData){
              datastaging.NewChild("DeleteOnTermination") = "false";
           }
           else {
              datastaging.NewChild("DeleteOnTermination") = "true";
           }
        }

        // Resources
        for (std::list<Arc::RunTimeEnvironmentType>::const_iterator it=innerRepresentation.RunTimeEnvironment.begin();
                 it!=innerRepresentation.RunTimeEnvironment.end(); it++) {
            Arc::XMLNode resources;
            if ( !bool(jobdescription["Resources"]) ){
               resources = jobdescription.NewChild("Resources");
            }else {
               resources = jobdescription["Resources"];
            }
            // When the version is not parsing
            if ( !(*it).Name.empty() && (*it).Version.empty()){
               resources.NewChild("jsdl-arc:RunTimeEnvironment").NewChild("jsdl-arc:Name") = (*it).Name;
            }

            // When more then one version are parsing
            for (std::list<std::string>::const_iterator it_version=(*it).Version.begin();
                 it_version!=(*it).Version.end(); it_version++) {
                Arc::XMLNode RTE = resources.NewChild("jsdl-arc:RunTimeEnvironment");
                RTE.NewChild("jsdl-arc:Name") = (*it).Name;
                Arc::XMLNode version = RTE.NewChild("jsdl-arc:Version").NewChild("jsdl-arc:Exact");
                version.NewAttribute("epsilon") = "0.5";
                version = *it_version;
            }
        }

        if (!innerRepresentation.CEType.empty()) {
            if ( !bool( jobdescription["Resources"] ) ) jobdescription.NewChild("Resources");
            if ( !bool( jobdescription["Resources"]["Middleware"] ) ) 
               jobdescription["Resources"].NewChild("jsdl-arc:Middleware");
            jobdescription["Resources"]["Middleware"].NewChild("jsdl-arc:Name") = innerRepresentation.CEType;
            //jobdescription["Resources"]["Middleware"].NewChild("Version") = ?;
        }

        if (innerRepresentation.SessionLifeTime != -1) {
            if ( !bool( jobdescription["Resources"] ) ) jobdescription.NewChild("Resources");
            if ( !bool( jobdescription["Resources"]["SessionLifeTime"] ) ) 
               jobdescription["Resources"].NewChild("jsdl-arc:SessionLifeTime");
            std::string outputValue;
            std::stringstream ss;
            ss << innerRepresentation.SessionLifeTime.GetPeriod();
            ss >> outputValue;
            jobdescription["Resources"]["SessionLifeTime"] = outputValue;
        }

        if ( !innerRepresentation.ReferenceTime.value.empty()) {
            if ( innerRepresentation.ReferenceTime.benchmark_attribute.empty() && 
                 innerRepresentation.ReferenceTime.value_attribute.empty() &&
                 isdigit(innerRepresentation.ReferenceTime.value[1])){
              if ( !bool( jobdescription["Resources"] ) ) jobdescription.NewChild("Resources");
              if ( !bool( jobdescription["Resources"]["GridTimeLimit"] ) ) 
                 jobdescription["Resources"].NewChild("jsdl-arc:GridTimeLimit");
              jobdescription["Resources"]["GridTimeLimit"] = innerRepresentation.ReferenceTime.value;
            }
            //TODO: what is the mapping, when the bechmark and the value attribute are not empty?
        }

        if (bool(innerRepresentation.EndPointURL)) {
           if ( !bool( jobdescription["Resources"] ) ) jobdescription.NewChild("Resources");
           if ( !bool( jobdescription["Resources"]["CandidateTarget"] ) ) 
              jobdescription["Resources"].NewChild("jsdl-arc:CandidateTarget");
           if ( !bool( jobdescription["Resources"]["CandidateTarget"]["HostName"] ) ) 
              jobdescription["Resources"]["CandidateTarget"].NewChild("jsdl-arc:HostName");
           jobdescription["Resources"]["CandidateTarget"]["HostName"] = innerRepresentation.EndPointURL.str();
        }

        if (!innerRepresentation.QueueName.empty()) {
           if ( !bool( jobdescription["Resources"] ) ) jobdescription.NewChild("Resources");
           if ( !bool( jobdescription["Resources"]["CandidateTarget"] ) ) 
              jobdescription["Resources"].NewChild("jsdl-arc:CandidateTarget");
           if ( !bool( jobdescription["Resources"]["CandidateTarget"]["QueueName"] ) ) 
              jobdescription["Resources"]["CandidateTarget"].NewChild("jsdl-arc:QueueName");
           jobdescription["Resources"]["CandidateTarget"]["QueueName"] = innerRepresentation.QueueName;
        }

        if (!innerRepresentation.Platform.empty()) {
           if ( !bool( jobdescription["Resources"] ) ) jobdescription.NewChild("Resources");
           if ( !bool( jobdescription["Resources"]["CPUArchitecture"] ) ) 
              jobdescription["Resources"].NewChild("CPUArchitecture");
           if ( !bool( jobdescription["Resources"]["CPUArchitecture"]["CPUArchitectureName"] ) ) 
              jobdescription["Resources"]["CPUArchitecture"].NewChild("CPUArchitectureName");
           jobdescription["Resources"]["CPUArchitecture"]["CPUArchitectureName"] = innerRepresentation.Platform;
        }

        if (!innerRepresentation.OSName.empty()) {
           if ( !bool( jobdescription["Resources"] ) ) jobdescription.NewChild("Resources");
           if ( !bool( jobdescription["Resources"]["OperatingSystem"] ) ) 
              jobdescription["Resources"].NewChild("OperatingSystem");
           if ( !bool( jobdescription["Resources"]["OperatingSystem"]["OperatingSystemType"] ) ) 
              jobdescription["Resources"]["OperatingSystem"].NewChild("OperatingSystemType");
            if ( !bool( jobdescription["Resources"]["OperatingSystem"]["OperatingSystemType"]["OperatingSystemName"] ) ) 
              jobdescription["Resources"]["OperatingSystem"]["OperatingSystemType"].NewChild("OperatingSystemName");
           jobdescription["Resources"]["OperatingSystem"]["OperatingSystemType"]["OperatingSystemName"] = innerRepresentation.OSName;
        }

        if (!innerRepresentation.OSVersion.empty()) {
           if ( !bool( jobdescription["Resources"] ) ) jobdescription.NewChild("Resources");
           if ( !bool( jobdescription["Resources"]["OperatingSystem"] ) ) 
              jobdescription["Resources"].NewChild("OperatingSystem");
           if ( !bool( jobdescription["Resources"]["OperatingSystem"]["OperatingSystemVersion"] ) ) 
              jobdescription["Resources"]["OperatingSystem"].NewChild("OperatingSystemVersion");
           jobdescription["Resources"]["OperatingSystem"]["OperatingSystemVersion"] = innerRepresentation.OSVersion;
        }

        if (innerRepresentation.ProcessPerHost > -1) {
           if ( !bool( jobdescription["Resources"] ) ) jobdescription.NewChild("Resources");
           if ( !bool( jobdescription["Resources"]["TotalCPUCount"] ) ) 
              jobdescription["Resources"].NewChild("TotalCPUCount");
            if ( !bool( jobdescription["Resources"]["TotalCPUCount"]["LowerBoundRange"] ) ) 
              jobdescription["Resources"]["TotalCPUCount"].NewChild("LowerBoundRange");
           std::ostringstream oss;
           oss << innerRepresentation.ProcessPerHost;
           jobdescription["Resources"]["TotalCPUCount"]["LowerBoundRange"] = oss.str();
        }

        if (innerRepresentation.IndividualPhysicalMemory > -1) {
           if ( !bool( jobdescription["Resources"] ) ) jobdescription.NewChild("Resources");
           if ( !bool( jobdescription["Resources"]["IndividualPhysicalMemory"] ) ) 
              jobdescription["Resources"].NewChild("IndividualPhysicalMemory");
           if ( !bool( jobdescription["Resources"]["IndividualPhysicalMemory"]["LowerBoundRange"] ) ) 
              jobdescription["Resources"]["IndividualPhysicalMemory"].NewChild("LowerBoundRange");
           std::ostringstream oss;
           oss << innerRepresentation.IndividualPhysicalMemory;
           jobdescription["Resources"]["IndividualPhysicalMemory"]["LowerBoundRange"] = oss.str();
        }

        if (innerRepresentation.IndividualVirtualMemory > -1) {
           if ( !bool( jobdescription["Resources"] ) ) jobdescription.NewChild("Resources");
           if ( !bool( jobdescription["Resources"]["IndividualVirtualMemory"] ) ) 
              jobdescription["Resources"].NewChild("IndividualVirtualMemory");
           if ( !bool( jobdescription["Resources"]["IndividualVirtualMemory"]["LowerBoundRange"] ) ) 
              jobdescription["Resources"]["IndividualVirtualMemory"].NewChild("LowerBoundRange");
           std::ostringstream oss;
           oss << innerRepresentation.IndividualVirtualMemory;
           jobdescription["Resources"]["IndividualVirtualMemory"]["LowerBoundRange"] = oss.str();
        }

        if (innerRepresentation.IndividualDiskSpace > -1) {
           if ( !bool( jobdescription["Resources"] ) ) jobdescription.NewChild("Resources");
           if ( !bool( jobdescription["Resources"]["IndividualDiskSpace"] ) ) 
              jobdescription["Resources"].NewChild("IndividualDiskSpace");
           if ( !bool( jobdescription["Resources"]["IndividualDiskSpace"]["LowerBoundRange"] ) ) 
              jobdescription["Resources"]["IndividualDiskSpace"].NewChild("LowerBoundRange");
           std::ostringstream oss;
           oss << innerRepresentation.IndividualDiskSpace;
           jobdescription["Resources"]["IndividualDiskSpace"]["LowerBoundRange"] = oss.str();
        }

        if (innerRepresentation.TotalCPUTime != -1) {
           if ( !bool( jobdescription["Resources"] ) ) jobdescription.NewChild("Resources");
           if ( !bool( jobdescription["Resources"]["TotalCPUTime"] ) ) 
              jobdescription["Resources"].NewChild("TotalCPUTime");
           if ( !bool( jobdescription["Resources"]["TotalCPUTime"]["LowerBoundRange"] ) ) 
              jobdescription["Resources"]["TotalCPUTime"].NewChild("LowerBoundRange");
           Period time((std::string)innerRepresentation.TotalCPUTime);
           std::ostringstream oss;
           oss << time.GetPeriod();
           jobdescription["Resources"]["TotalCPUTime"]["LowerBoundRange"] = oss.str();
        }

        if (innerRepresentation.IndividualCPUTime != -1) {
           if ( !bool( jobdescription["Resources"] ) ) jobdescription.NewChild("Resources");
           if ( !bool( jobdescription["Resources"]["IndividualCPUTime"] ) ) 
              jobdescription["Resources"].NewChild("IndividualCPUTime");
           if ( !bool( jobdescription["Resources"]["IndividualCPUTime"]["LowerBoundRange"] ) ) 
              jobdescription["Resources"]["IndividualCPUTime"].NewChild("LowerBoundRange");
           Period time((std::string)innerRepresentation.IndividualCPUTime);
           std::ostringstream oss;
           oss << time.GetPeriod();
           jobdescription["Resources"]["IndividualCPUTime"]["LowerBoundRange"] = oss.str();
        }

        if (innerRepresentation.DiskSpace > -1) {
           if ( !bool( jobdescription["Resources"] ) ) jobdescription.NewChild("Resources");
           if ( !bool( jobdescription["Resources"]["FileSystem"] ) ) 
              jobdescription["Resources"].NewChild("FileSystem");
           if ( !bool( jobdescription["Resources"]["FileSystem"]["DiskSpace"] ) ) 
              jobdescription["Resources"]["FileSystem"].NewChild("DiskSpace");
           if ( !bool( jobdescription["Resources"]["FileSystem"]["DiskSpace"]["LowerBoundRange"] ) ) 
              jobdescription["Resources"]["FileSystem"]["DiskSpace"].NewChild("LowerBoundRange");
           std::ostringstream oss;
           oss << innerRepresentation.DiskSpace;
           jobdescription["Resources"]["FileSystem"]["DiskSpace"]["LowerBoundRange"] = oss.str();
        }

        if (innerRepresentation.ExclusiveExecution) {
           if ( !bool( jobdescription["Resources"] ) ) jobdescription.NewChild("Resources");
           if ( !bool( jobdescription["Resources"]["ExclusiveExecution"] ) ) 
              jobdescription["Resources"].NewChild("ExclusiveExecution") = "true";
        }

        if (!innerRepresentation.NetworkInfo.empty()) {
           std::string value = "";
           if (innerRepresentation.NetworkInfo == "100megabitethernet"){
              value = "104857600.0";
           }
           else if (innerRepresentation.NetworkInfo == "gigabitethernet"){
              value = "1073741824.0";
           }
           else if (innerRepresentation.NetworkInfo == "myrinet"){
              value = "2147483648.0";
           }
           else if (innerRepresentation.NetworkInfo == "infiniband"){
              value = "10737418240.0";
           }
           else {
              value = "";
           }

           if ( value != "" ) {
              if ( !bool( jobdescription["Resources"] ) ) jobdescription.NewChild("Resources");
              if ( !bool( jobdescription["Resources"]["IndividualNetworkBandwidth"] ) ) 
                 jobdescription["Resources"].NewChild("IndividualNetworkBandwidth");
              if ( !bool( jobdescription["Resources"]["IndividualNetworkBandwidth"]["LowerBoundRange"] ) ) 
                 jobdescription["Resources"]["IndividualNetworkBandwidth"].NewChild("LowerBoundRange");
              jobdescription["Resources"]["IndividualNetworkBandwidth"]["LowerBoundRange"] = value;
           }
        }
        // end of Resources


        // AccessControl
        if (bool(innerRepresentation.AccessControl)) {
            if ( !bool( jobdescription["AccessControl"] ) ) jobdescription.NewChild("jsdl-arc:AccessControl");
            if ( !bool( jobdescription["AccessControl"]["Type"] ) ) 
               jobdescription["AccessControl"].NewChild("jsdl-arc:Type") = "GACL";
            jobdescription["AccessControl"].NewChild("jsdl-arc:Content").NewChild(innerRepresentation.AccessControl);
        }

        // Notify
        if (!innerRepresentation.Notification.empty()) {
            for (std::list<Arc::NotificationType>::const_iterator it=innerRepresentation.Notification.begin();
                 it!=innerRepresentation.Notification.end(); it++) {
                Arc::XMLNode notify = jobdescription.NewChild("jsdl-arc:Notify");
                notify.NewChild("jsdl-arc:Type") = "Email";
                for (std::list<std::string>::const_iterator it_address=(*it).Address.begin();
                                                it_address!=(*it).Address.end(); it_address++) {
                    if ( *it_address != "" ) notify.NewChild("jsdl-arc:Endpoint") = *it_address;
                }
                for (std::list<std::string>::const_iterator it_state=(*it).State.begin();
                                                it_state!=(*it).State.end(); it_state++) {
                    if ( *it_state != "" ) notify.NewChild("jsdl-arc:State") = *it_state;
                }
            }
        }

        // ProcessingStartTime
        if (innerRepresentation.ProcessingStartTime != -1) {
           if ( !bool( jobdescription["ProcessingStartTime"] ) ) jobdescription.NewChild("jsdl-arc:ProcessingStartTime");
           jobdescription["ProcessingStartTime"] = innerRepresentation.ProcessingStartTime.str();
        }

        // Reruns
        if (innerRepresentation.LRMSReRun != -1) {
           if ( !bool( jobdescription["Reruns"] ) ) jobdescription.NewChild("jsdl-arc:Reruns");
           std::ostringstream oss;
           oss << innerRepresentation.LRMSReRun;
           jobdescription["Reruns"] = oss.str();
        }

        // LocalLogging.Directory
        if (!innerRepresentation.LogDir.empty()) {
           if ( !bool( jobdescription["LocalLogging"] ) ) jobdescription.NewChild("jsdl-arc:LocalLogging");
           if ( !bool( jobdescription["LocalLogging"]["Directory"] ) ) jobdescription["LocalLogging"].NewChild("jsdl-arc:Directory");
           jobdescription["LocalLogging"]["Directory"] = innerRepresentation.LogDir;
        }

        // CredentialServer.URL
        if (bool(innerRepresentation.CredentialService)) {
           if ( !bool( jobdescription["CredentialServer"] ) ) jobdescription.NewChild("jsdl-arc:CredentialServer");
           if ( !bool( jobdescription["CredentialServer"]["URL"] ) ) jobdescription["CredentialServer"].NewChild("jsdl-arc:URL");
           jobdescription["CredentialServer"]["URL"] = innerRepresentation.CredentialService.fullstr();
        }

        // RemoteLogging.URL
        if (bool(innerRepresentation.RemoteLogging)) {
           if ( !bool( jobdescription["RemoteLogging"] ) ) jobdescription.NewChild("jsdl-arc:RemoteLogging");
           if ( !bool( jobdescription["RemoteLogging"]["URL"] ) ) jobdescription["RemoteLogging"].NewChild("jsdl-arc:URL");
           jobdescription["RemoteLogging"]["URL"] = innerRepresentation.RemoteLogging.fullstr();
        }

        jobdefinition.GetDoc( product, true );

        return true;
    }

    // Optional name attribute checker.
    // When the node has "isoptional" attribute and it is "true", then add this node name to
    // the list of OptionalElement.  
    void Optional_attribute_check(Arc::XMLNode node, std::string path, Arc::JobInnerRepresentation& innerRepresentation, Arc::OptionalElementType& optional_element, std::string value=""){
         StringManipulator sm;
         if (node.AttributesSize() > 0 &&  sm.toLowerCase((std::string)node.Attribute("isoptional")) == "true"){
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
	    JobDescription::logger.msg(DEBUG, "[JSDLParser] Wrong XML structure! ");
            return false;
        }
        if ( !Arc::Element_Search(node, false ) ) return false;

        // The source parsing start now.
        Arc::NS nsList;
        nsList.insert(std::pair<std::string, std::string>("jsdl","http://schemas.ggf.org/jsdl/2005/11/jsdl"));
        //TODO: wath is the new GIN namespace  (it is now temporary namespace)      
        //nsList.insert(std::pair<std::string, std::string>("gin","http://"));

        //Meta
        Arc::XMLNodeList meta;
        Arc::XMLNode _meta;
        Arc::OptionalElementType optional;

        _meta = node["Meta"]["Author"];
        if ( bool(_meta) && (std::string)(_meta) != "" ){
           Optional_attribute_check(_meta, "//Meta/Author", innerRepresentation,optional);
           innerRepresentation.Author = (std::string)(_meta);
        }
        _meta.Destroy();

        _meta = node["Meta"]["DocumentExpiration"];
        if ( bool(_meta) && (std::string)(_meta) != "" ){
           Optional_attribute_check(_meta, "//Meta/DocumentExpiration", innerRepresentation,optional);
           Time time((std::string)(_meta));
           innerRepresentation.DocumentExpiration = time;
        }
        _meta.Destroy();

        meta = node.XPathLookup((std::string)"//Meta/Rank", nsList);
           Optional_attribute_check(*(meta.begin()), "//Meta/Rank", innerRepresentation,optional);
        if ( !meta.empty() )
           innerRepresentation.Rank = (std::string)(*meta.begin());
        meta.clear();

        meta = node.XPathLookup((std::string)"//Meta/FuzzyRank", nsList);
        if ( !meta.empty() )
           Optional_attribute_check(*(meta.begin()), "//Meta/FuzzyRank", innerRepresentation,optional);
           if ( (std::string)(*meta.begin()) == "true" ) {
              innerRepresentation.FuzzyRank = true;
           }
           else if ( (std::string)(*meta.begin()) == "false" ) {
              innerRepresentation.FuzzyRank = false;
           }
           else {
	      JobDescription::logger.msg(DEBUG, "Invalid \"/Meta/FuzzyRank\" value: %s", (std::string)(*meta.begin()));
              return false;         
           }
        meta.clear();

        //Application         
        Arc::XMLNodeList application;
        application = node.XPathLookup((std::string)"//JobDescription/Application/Executable", nsList);
        if ( !application.empty() )
           Optional_attribute_check(*(application.begin()), "//JobDescription/Application/Executable", innerRepresentation,optional);
           innerRepresentation.Executable = (std::string)(*application.begin());
        application.clear();

        application = node.XPathLookup((std::string)"//JobDescription/Application/LogDir", nsList);
        if ( !application.empty() )
           Optional_attribute_check(*(application.begin()), "//JobDescription/Application/LogDir", innerRepresentation,optional);
           innerRepresentation.LogDir = (std::string)(*application.begin());
        application.clear();

        application = node.XPathLookup( (std::string) "//JobDescription/Application/Argument", nsList);
        for (std::list<Arc::XMLNode>::iterator it = application.begin(); it != application.end(); it++) {
           Optional_attribute_check(*it, "//JobDescription/Application/Argument", innerRepresentation,optional, (std::string)(*it));
            innerRepresentation.Argument.push_back( (std::string)(*it) );
        }
        application.clear();

        application = node.XPathLookup((std::string)"//JobDescription/Application/Input", nsList);
        if ( !application.empty() )
           Optional_attribute_check(*(application.begin()), "//JobDescription/Application/Input", innerRepresentation,optional);
           innerRepresentation.Input = (std::string)(*application.begin());
        application.clear();

        application = node.XPathLookup((std::string)"//JobDescription/Application/Output", nsList);
        if ( !application.empty() )
           Optional_attribute_check(*(application.begin()), "//JobDescription/Application/Output", innerRepresentation,optional);
           innerRepresentation.Output = (std::string)(*application.begin());
        application.clear();

        application = node.XPathLookup((std::string)"//JobDescription/Application/Error", nsList);
        if ( !application.empty() )
           Optional_attribute_check(*(application.begin()), "//JobDescription/Application/Error", innerRepresentation,optional);
           innerRepresentation.Error = (std::string)(*application.begin());
        application.clear();

        application = node.XPathLookup((std::string)"//JobDescription/Application/RemoteLogging", nsList);
        if ( !application.empty() ) {
           Optional_attribute_check(*(application.begin()), "//JobDescription/Application/RemoteLogging", innerRepresentation,optional);
           URL url((std::string)(*application.begin()));
           innerRepresentation.RemoteLogging = url;
        }
        application.clear();

        application = node.XPathLookup( (std::string) "//JobDescription/Application/Environment", nsList);
        for (std::list<Arc::XMLNode>::iterator it = application.begin(); it != application.end(); it++) {
           Optional_attribute_check(*it, "//JobDescription/Application/Environment", innerRepresentation,optional, (std::string)(*it));
            Arc::EnvironmentType env;
            env.name_attribute = (std::string)(*it).Attribute("name");
            env.value = (std::string)(*it);
            innerRepresentation.Environment.push_back( env );
        }
        application.clear();

        application = node.XPathLookup((std::string)"//JobDescription/Application/LRMSReRun", nsList);
        if ( !application.empty() ){
           Optional_attribute_check(*(application.begin()), "//JobDescription/Application/LRMSReRun", innerRepresentation,optional);
           innerRepresentation.LRMSReRun = stringtoi(*application.begin());
        }
        application.clear();

        application = node.XPathLookup((std::string)"//JobDescription/Application/Prologue", nsList);
        if ( !application.empty() ) {
           Optional_attribute_check(*(application.begin()), "//JobDescription/Application/Prologue", innerRepresentation,optional);
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
           Optional_attribute_check(*(application.begin()), "//JobDescription/Application/Epilogue", innerRepresentation,optional);
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
           Optional_attribute_check(*(application.begin()), "//JobDescription/Application/SessionLifeTime", innerRepresentation,optional);
           Period time((std::string)(*application.begin()));
           innerRepresentation.SessionLifeTime = time;
        }
        application.clear();

        application = node.XPathLookup((std::string)"//JobDescription/Application/AccessControl", nsList);
        if ( !application.empty() ){
           Optional_attribute_check(*(application.begin()), "//JobDescription/Application/AccessControl", innerRepresentation,optional);
           ((Arc::XMLNode)(*application.begin())).Child(0).New(innerRepresentation.AccessControl);
        }
        application.clear();

        application = node.XPathLookup((std::string)"//JobDescription/Application/ProcessingStartTime", nsList);
        if ( !application.empty() ) {
           Optional_attribute_check(*(application.begin()), "//JobDescription/Application/ProcessingStartTime", innerRepresentation,optional);
           Time stime((std::string)*application.begin());
           innerRepresentation.ProcessingStartTime = stime;
        }
        application.clear();

        application = node.XPathLookup( (std::string) "//JobDescription/Application/Notification", nsList);
        for (std::list<Arc::XMLNode>::iterator it = application.begin(); it != application.end(); it++) {
           Optional_attribute_check(*it, "//JobDescription/Application/Notification", innerRepresentation,optional);
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
           Optional_attribute_check(*(application.begin()), "//JobDescription/Application/CredentialService", innerRepresentation,optional);
           URL curl((std::string)*application.begin());
           innerRepresentation.CredentialService = curl;
        }
        application.clear();

        application = node.XPathLookup((std::string)"//JobDescription/Application/Join", nsList);
        if ( !application.empty() && Arc::upper((std::string)*application.begin()) == "TRUE" ){
           Optional_attribute_check(*(application.begin()), "//JobDescription/Application/Join", innerRepresentation,optional);
           innerRepresentation.Join = true;
        }
        application.clear();

        //JobIdentification
        Arc::XMLNodeList jobidentification;
        jobidentification = node.XPathLookup((std::string)"//JobDescription/JobIdentification/JobName", nsList);
        if ( !jobidentification.empty() ){
           Optional_attribute_check(*(jobidentification.begin()), "//JobDescription/JobIdentification/JobName", innerRepresentation,optional);
           innerRepresentation.JobName = (std::string)(*jobidentification.begin());
        }
        jobidentification.clear();

        jobidentification = node.XPathLookup((std::string)"//JobDescription/JobIdentification/Description", nsList);
        if ( !jobidentification.empty() ){
           Optional_attribute_check(*(jobidentification.begin()), "//JobDescription/JobIdentification/Description", innerRepresentation,optional);
           innerRepresentation.Description = (std::string)(*jobidentification.begin());
        }
        jobidentification.clear();

        jobidentification = node.XPathLookup((std::string)"//JobDescription/JobIdentification/JobProject", nsList);
        if ( !jobidentification.empty() ){
           Optional_attribute_check(*(jobidentification.begin()), "//JobDescription/JobIdentification/JobProject", innerRepresentation,optional);
           innerRepresentation.JobProject = (std::string)(*jobidentification.begin());
        }
        jobidentification.clear();

        jobidentification = node.XPathLookup((std::string)"//JobDescription/JobIdentification/JobType", nsList);
        if ( !jobidentification.empty() ){
           Optional_attribute_check(*(jobidentification.begin()), "//JobDescription/JobIdentification/JobType", innerRepresentation,optional);
           innerRepresentation.JobType = (std::string)(*jobidentification.begin());
        }
        jobidentification.clear();

        jobidentification = node.XPathLookup((std::string)"//JobDescription/JobIdentification/JobCategory", nsList);
        if ( !jobidentification.empty() ){
           Optional_attribute_check(*(jobidentification.begin()), "//JobDescription/JobIdentification/JobCategory", innerRepresentation,optional);
           innerRepresentation.JobCategory = (std::string)(*jobidentification.begin());
        }
        jobidentification.clear();

        jobidentification = node.XPathLookup((std::string)"//JobDescription/JobIdentification/UserTag", nsList);
        for (std::list<Arc::XMLNode>::const_iterator it = jobidentification.begin(); it != jobidentification.end(); it++) {
            Optional_attribute_check(*(jobidentification.begin()), "//JobDescription/JobIdentification/UserTag", innerRepresentation,optional, (std::string)(*it));
            innerRepresentation.UserTag.push_back( (std::string)(*it));
        }
        jobidentification.clear();

        //Resource
        Arc::XMLNodeList resource;
        resource = node.XPathLookup((std::string)"//JobDescription/Resource/TotalCPUTime", nsList);
        if ( !resource.empty() ) {
           Optional_attribute_check(*(resource.begin()),"//JobDescription/Resource/TotalCPUTime", innerRepresentation,optional);
           Period time((std::string)(*resource.begin()));
           innerRepresentation.TotalCPUTime = time;
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/IndividualCPUTime", nsList);
        if ( !resource.empty() ) {
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/IndividualCPUTime", innerRepresentation,optional);
           Period time((std::string)(*resource.begin()));
           innerRepresentation.IndividualCPUTime = time;
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/TotalWallTime", nsList);
        if ( !resource.empty() ) {
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/TotalWallTime", innerRepresentation,optional);
           Period _walltime((std::string)(*resource.begin()));
           innerRepresentation.TotalWallTime = _walltime;
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/IndividualWallTime", nsList);
        if ( !resource.empty() ) {
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/IndividualWallTime", innerRepresentation,optional);
           Period _indwalltime((std::string)(*resource.begin()));
           innerRepresentation.IndividualWallTime = _indwalltime;
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/ReferenceTime", nsList);
        if ( !resource.empty() ) {
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/ReferenceTime", innerRepresentation,optional);
           Arc::ReferenceTimeType _reference;
           _reference.value = (std::string)(*resource.begin());
           _reference.benchmark_attribute = (std::string)((Arc::XMLNode)(*resource.begin()).Attribute("benchmark"));
           _reference.value_attribute = (std::string)((Arc::XMLNode)(*resource.begin()).Attribute("value"));
           innerRepresentation.ReferenceTime = _reference;
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/ExclusiveExecution", nsList);
        if ( !resource.empty() ) {
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/ExclusiveExecution", innerRepresentation,optional);
           Arc::ReferenceTimeType _reference;
           if ( Arc::upper((std::string)(*resource.begin())) == "TRUE" ) {
              innerRepresentation.ExclusiveExecution = true;
           }
           else if ( Arc::upper((std::string)(*resource.begin())) == "FALSE" ) {
              innerRepresentation.ExclusiveExecution = false;
           }
           else {
	      JobDescription::logger.msg(DEBUG, "Invalid \"/JobDescription/Resource/ExclusiveExecution\" value: %s", (std::string)(*resource.begin()));
              return false;
           }
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/NetworkInfo", nsList);
        if ( !resource.empty() ){
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/NetworkInfo", innerRepresentation,optional);
           innerRepresentation.NetworkInfo = (std::string)(*resource.begin());
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/OSFamily", nsList);
        if ( !resource.empty() ){
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/OSFamily", innerRepresentation,optional);
           innerRepresentation.OSFamily = (std::string)(*resource.begin());
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/OSName", nsList);
        if ( !resource.empty() ){
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/OSName", innerRepresentation,optional);
           innerRepresentation.OSName = (std::string)(*resource.begin());
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/OSVersion", nsList);
        if ( !resource.empty() ){
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/OSVersion", innerRepresentation,optional);
           innerRepresentation.OSVersion = (std::string)(*resource.begin());
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/Platform", nsList);
        if ( !resource.empty() ){
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/Platform", innerRepresentation,optional);
           innerRepresentation.Platform = (std::string)(*resource.begin());
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/IndividualPhysicalMemory", nsList);
        if ( !resource.empty() ){
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/IndividualPhysicalMemory", innerRepresentation,optional);
           innerRepresentation.IndividualPhysicalMemory = stringtoi(*resource.begin());
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/IndividualVirtualMemory", nsList);
        if ( !resource.empty() ){
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/IndividualVirtualMemory", innerRepresentation,optional);
           innerRepresentation.IndividualVirtualMemory = stringtoi(*resource.begin());
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/DiskSpace/IndividualDiskSpace", nsList);
        if ( !resource.empty() ){
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/DiskSpace/IndividualDiskSpace", innerRepresentation,optional);
           innerRepresentation.IndividualDiskSpace = stringtoi(*resource.begin());
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/DiskSpace", nsList);
        if ( !resource.empty()){
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/DiskSpace", innerRepresentation,optional);
           innerRepresentation.DiskSpace = stringtoi(*resource.begin());
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/CacheDiskSpace", nsList);
        if ( !resource.empty()){
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/CacheDiskSpace", innerRepresentation,optional);
           innerRepresentation.CacheDiskSpace = stringtoi(*resource.begin());
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/SessionDiskSpace", nsList);
        if ( !resource.empty()){
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/SessionDiskSpace", innerRepresentation,optional);
           innerRepresentation.SessionDiskSpace = stringtoi(*resource.begin());
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/CandidateTarget/Alias", nsList);
        if ( !resource.empty() ){
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/CandidateTarget/Alias", innerRepresentation,optional);
           innerRepresentation.Alias = (std::string)(*resource.begin());
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/CandidateTarget/EndPointURL", nsList);
        if ( !resource.empty() ) {
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/CandidateTarget/EndPointURL", innerRepresentation,optional);
           URL url((std::string)(*resource.begin()));
           innerRepresentation.EndPointURL = url;
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/CandidateTarget/QueueName", nsList);
        if ( !resource.empty() ){
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/CandidateTarget/QueueName", innerRepresentation,optional);
           innerRepresentation.QueueName = (std::string)(*resource.begin());
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/Location/Country", nsList);
        if ( !resource.empty() ){
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/Location/Country", innerRepresentation,optional);
           innerRepresentation.Country = (std::string)(*resource.begin());
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/Location/Place", nsList);
        if ( !resource.empty() ){
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/Location/Place", innerRepresentation,optional);
           innerRepresentation.Place = (std::string)(*resource.begin());
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/Location/PostCode", nsList);
        if ( !resource.empty() ){
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/Location/PostCode", innerRepresentation,optional);
           innerRepresentation.PostCode = (std::string)(*resource.begin());
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/Location/Latitude", nsList);
        if ( !resource.empty() ){
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/Location/Latitude", innerRepresentation,optional);
           innerRepresentation.Latitude = (std::string)(*resource.begin());
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/Location/Longitude", nsList);
        if ( !resource.empty() ){
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/Location/Longitude", innerRepresentation,optional);
           innerRepresentation.Longitude = (std::string)(*resource.begin());
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/CEType", nsList);
        if ( !resource.empty() ){
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/CEType", innerRepresentation,optional);
           innerRepresentation.CEType = (std::string)(*resource.begin());
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/Slots", nsList);
        if ( !resource.empty() ){
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/Slots", innerRepresentation,optional);
           innerRepresentation.Slots = stringtoi(*resource.begin());
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/Slots/ProcessPerHost", nsList);
        if ( !resource.empty() ){
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/Slots/ProcessPerHost", innerRepresentation,optional);
           innerRepresentation.ProcessPerHost = stringtoi(*resource.begin());
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/Slots/NumberOfProcesses", nsList);
        if ( !resource.empty() ){
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/Slots/NumberOfProcesses", innerRepresentation,optional);
           innerRepresentation.NumberOfProcesses = stringtoi(*resource.begin());
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/Slots/ThreadPerProcesses", nsList);
        if ( !resource.empty() ){
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/Slots/ThreadPerProcesses", innerRepresentation,optional);
           innerRepresentation.ThreadPerProcesses = stringtoi(*resource.begin());
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/Slots/SPMDVariation", nsList);
        if ( !resource.empty() ){
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/Slots/SPMDVariation", innerRepresentation,optional);
          innerRepresentation.SPMDVariation = (std::string)(*resource.begin());
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/RunTimeEnvironment", nsList);
        for (std::list<Arc::XMLNode>::const_iterator it = resource.begin(); it != resource.end(); it++) {
//            Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/RunTimeEnvironment", innerRepresentation,optional, (std::string)(*it));
            Arc::RunTimeEnvironmentType _runtimeenv;
            if ( (bool)((*it)["Name"]) )
               _runtimeenv.Name = (std::string)(*it)["Name"];
            Arc::XMLNodeList version = (*it).XPathLookup((std::string)"//Version", nsList);
            for (std::list<Arc::XMLNode>::const_iterator it_v = version.begin(); it_v!= version.end(); it_v++) {
                Optional_attribute_check(*it_v, "//JobDescription/Resource/RunTimeEnvironment/Version", innerRepresentation,optional, (std::string)(*it_v));
                _runtimeenv.Version.push_back( (std::string)(*it_v) );
            }
            innerRepresentation.RunTimeEnvironment.push_back( _runtimeenv );
        }
        resource.clear();

        resource = node.XPathLookup((std::string)"//JobDescription/Resource/Homogeneous", nsList);
        if ( !resource.empty() &&
             (std::string)(*resource.begin()) == "true" ){
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/Homogeneous", innerRepresentation,optional);
           innerRepresentation.Homogeneous = true;
        }
        else {
           Optional_attribute_check(*(resource.begin()), "//JobDescription/Resource/Homogeneous", innerRepresentation,optional);
           innerRepresentation.Homogeneous = false;
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
//           Optional_attribute_check(*(datastaging.begin()), "//JobDescription/DataStaging/File", innerRepresentation,optional);
            Arc::FileType _file;
            if ( (bool)((*it)["Name"]) )
               _file.Name = (std::string)(*it)["Name"];
            Arc::XMLNodeList source = (*it).XPathLookup((std::string)"//Source", nsList);
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
            Arc::XMLNodeList target = (*it).XPathLookup((std::string)"//Target", nsList);
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
		      JobDescription::logger.msg(DEBUG, "Invalid \"/JobDescription/DataStaging/File/Target/Mandatory\" value: %s", (std::string)(*it_target)["Mandatory"]);
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
                      JobDescription::logger.msg(DEBUG, "Invalid \"/JobDescription/DataStaging/File/KeepData\" value: %s", (std::string)(*it)["KeepData"]);
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
		      JobDescription::logger.msg(DEBUG, "Invalid \"/JobDescription/DataStaging/File/IsExecutable\" value: %s", (std::string)(*it)["IsExecutable"]);
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
		      JobDescription::logger.msg(DEBUG, "Invalid \"/JobDescription/DataStaging/File/DownloadToCache\" value: %s", (std::string)(*it)["DownloadToCache"]);
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
//           Optional_attribute_check(*(datastaging.begin()), "//JobDescription/DataStaging/Directory", innerRepresentation);
            Arc::DirectoryType _directory;
            if ( (bool)((*it)["Name"]) )
               _directory.Name = (std::string)(*it)["Name"];
            Arc::XMLNodeList source = (*it).XPathLookup((std::string)"//Source", nsList);
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
            Arc::XMLNodeList target = (*it).XPathLookup((std::string)"//Target", nsList);
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
		      JobDescription::logger.msg(DEBUG, "Invalid \"/JobDescription/DataStaging/Directory/Target/Mandatory\" value: %s",  (std::string)(*it_target)["Mandatory"]);
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
		      JobDescription::logger.msg(DEBUG, "Invalid \"/JobDescription/DataStaging/Directory/KeepData\" value: %s", (std::string)(*it)["KeepData"]);
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
		      JobDescription::logger.msg(DEBUG, "Invalid \"/JobDescription/DataStaging/Directory/IsExecutable\" value: %s", (std::string)(*it)["IsExecutable"]);
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
		      JobDescription::logger.msg(DEBUG, "Invalid \"/JobDescription/DataStaging/Directory/DownloadToCache\" value: %s", (std::string)(*it)["DownloadToCache"]);
                   }
            }
            else {
                   _directory.DownloadToCache = false;
            }
            innerRepresentation.Directory.push_back( _directory );
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
	    JobDescription::logger.msg(DEBUG, "[JSDLParser] Job inner representation's root element has not found.");
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
                if ( (*it)[1] != "" )
                    source.URI = (*it)[1];
                else
                    source.URI = (*it)[0];
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
                if ( (*it)[1] != "" )
                    target.URI = (*it)[1];
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
            RuntimeEnvironment rt(simpleXRSLvalue( attributeValue ));
            runtime.Name =  rt.Name();
            if (!rt.Version().empty())
               runtime.Version.push_back(rt.Version());
            innerRepresentation.RunTimeEnvironment.push_back( runtime );
            return true;
        } else if ( attributeName == "middleware" ) {
            innerRepresentation.CEType = simpleXRSLvalue( attributeValue ); 
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
            URL url(simpleXRSLvalue( attributeValue ));
            innerRepresentation.EndPointURL = url; 
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
                         default: 
			    JobDescription::logger.msg(DEBUG, "Invalid notify attribute: %s", (*it)[i]);
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
            if ( simpleXRSLvalue( attributeValue ) != "" ){
               Arc::URL url(simpleXRSLvalue( attributeValue ));
               std::list<Arc::FileType>::iterator iter;
               for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                    (*iter).DataIndexingService = url;
               }
               std::list<Arc::DirectoryType>::iterator iter_d;
               for (iter_d = innerRepresentation.Directory.begin(); iter_d != innerRepresentation.Directory.end(); iter_d++){
                    (*iter_d).DataIndexingService = url;
               }
            }
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
	       JobDescription::logger.msg(DEBUG, "Invalid NodeAccess value: %s", simpleXRSLvalue( attributeValue ));
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
	JobDescription::logger.msg(DEBUG, "[XRSLParser] Unknown XRSL attribute: %s", attributeName);
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
			    JobDescription::logger.msg(DEBUG, "[XRSLParser] XRSL Syntax error (attribute declaration without equal sign)");
                            return false;
                        }
                        if ( !handleXRSLattribute( sm.trim( wattr.substr(0,eqpos) ) , sm.trim( wattr.substr(eqpos+1) ) , innerRepresentation) ) return false;
                    } else actual_argument += next_char;
                } else {
                    actual_argument += next_char;
                    if ( pos == xrsl_text.size()-1 ) return false;
                }
            }
        }
  
        if (depth != 0 ) {
	    JobDescription::logger.msg(DEBUG, "[XRSLParser] XRSL Syntax error (bracket mistake)");
            return false;
        }
        return true;
    }

    bool XRSLParser::getProduct( const Arc::JobInnerRepresentation& innerRepresentation, std::string& product ) {
        product = "&\n";
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
            bool first_time = true;
            std::list<Arc::FileType>::const_iterator iter;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                std::list<Arc::SourceType>::const_iterator it_source;
                for (it_source = (*iter).Source.begin(); it_source != (*iter).Source.end(); it_source++){
                    if (first_time){
                       product += "( inputfiles =";
                       first_time = false;
                    }
                    product += " ( \"" + (*iter).Name;

                    // file size added
                    struct stat fileStat;
                    if ( stat((*it_source).URI.Path().c_str(), &fileStat) == 0){
                       std::string filesize;
                       std::stringstream ss;
                       ss << fileStat.st_size;
                       ss >> filesize;
                       product += "\" \"" + filesize + "\"";
                    }
                    else {
                        product += "\" " +  (*it_source).URI.fullstr();
                    }

                    // checksum added
                    // It is optional!!!
                    int h=open((*it_source).URI.fullstr().c_str(),O_RDONLY);
                    if (h > -1) { /* if we can read that file job can checksum compute */
                       Arc::CRC32Sum crc;
                       char buffer[1024];
                       ssize_t l;
                       size_t ll = 0;
                       for(;;) {
                          if ((l=read(h,buffer,1024)) == -1) {
			     JobDescription::logger.msg(DEBUG, "Error reading file: %s", (*it_source).URI.fullstr());
			     JobDescription::logger.msg(DEBUG, "Could not read file to compute checksum.");
                          }
                          if(l==0) break; ll+=l;
                          crc.add(buffer,l);
                       }
                       close(h);
                       crc.end();
                       /* //it is not working now!
                       unsigned char *buf;
                       unsigned int len;
                       crc.result(buf,len);
                       std::string file_checksum;
                       std::stringstream ss;
                       ss << buf;
                       ss >> file_checksum;
                       if (!file_checksum.empty())
                          product += " \"" + file_checksum + "\"";
                       */
                    }
                    product +=  " )";
                }
            }
            if (!first_time)
               product += " )\n";
        }
        if (!innerRepresentation.File.empty()) {
            bool first_time = true;
            std::list<Arc::FileType>::const_iterator iter;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                if ( (*iter).IsExecutable ) {
                   if ( first_time ) {
                      product += "( executables =";
                      first_time = false;
                   }
                   product += " \"" + (*iter).Name + "\"";
                }
            }
            if ( !first_time ) 
               product += " )\n";
        }
        if (!innerRepresentation.File.empty()) {
            bool first_time = true;
            bool output(false),error(false);
            std::list<Arc::FileType>::const_iterator iter;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                std::list<Arc::TargetType>::const_iterator it_target;
                for (it_target = (*iter).Target.begin(); it_target != (*iter).Target.end(); it_target++){
                    if ( first_time ) {
                       product += "( outputfiles =";
                       first_time = false;
                    }
                    product += " ( \"" + (*iter).Name;
                    if ( (*iter).Name == innerRepresentation.Output ) output = true;
                    if ( (*iter).Name == innerRepresentation.Error ) error = true;
		    if ((*it_target).URI.fullstr().empty())
                       product += "\" \"\" )";
		    else 
                       product += "\" " +  (*it_target).URI.fullstr() + " )";
                }
            }
            if ( !first_time ){
               if (!innerRepresentation.Output.empty() && !output)
                  product += " ( \"" + innerRepresentation.Output + "\" \"\" )";
               if (!innerRepresentation.Error.empty() && !error)
                  product += " ( \"" + innerRepresentation.Error + "\" \"\" )";
               product += " )\n";
            }
            else {
               if (!innerRepresentation.Output.empty() ||
                   !innerRepresentation.Error.empty()) product += "( outputfiles =";
               
               if (!innerRepresentation.Output.empty()) product += " ( \"" + innerRepresentation.Output + "\" \"\" )";
               if (!innerRepresentation.Error.empty())  product += " ( \"" + innerRepresentation.Error + "\" \"\" )";
               if (!innerRepresentation.Output.empty() ||
                   !innerRepresentation.Error.empty()) product += " )\n";

            }
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
            std::list<std::string>::const_iterator viter;
            for (iter = innerRepresentation.RunTimeEnvironment.begin(); \
                 iter != innerRepresentation.RunTimeEnvironment.end(); iter++){
			    viter = ((*iter).Version).begin(); 
                product += "( runtimeenvironment = ";
                product +=  (*iter).Name + "-" + (*viter);
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
            product += "( jobname = \"";
            product +=  innerRepresentation.JobName;
            product += "\" )\n";
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
                     else { 
		          JobDescription::logger.msg(DEBUG, "Invalid State [\"%s\"] in the Notification!", *iter );
		     }
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
            product += "( benchmarks = ( \"";//TODO:
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
        if (!innerRepresentation.File.empty() || !innerRepresentation.Directory.empty() ) {
            std::vector<std::string> indexing_services;
            indexing_services.clear();
            // Indexin services searching
            std::list<Arc::FileType>::const_iterator iter;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                if ( find( indexing_services.begin(), indexing_services.end(),(*iter).DataIndexingService.fullstr() ) == indexing_services.end() )
                   indexing_services.push_back((*iter).DataIndexingService.fullstr());
            }
            std::list<Arc::DirectoryType>::const_iterator iter_d;
            for (iter_d = innerRepresentation.Directory.begin(); iter_d != innerRepresentation.Directory.end(); iter_d++){
                if ( find( indexing_services.begin(), indexing_services.end(),(*iter).DataIndexingService.fullstr() ) == indexing_services.end() )
                   indexing_services.push_back((*iter_d).DataIndexingService.fullstr());
            }
            // XRSL output(s) generation
            std::vector<std::string>::const_iterator i_url;
            for (i_url = indexing_services.begin(); i_url != indexing_services.end(); i_url++){
                product += "( replicacollection = \"";
                product +=  *i_url;
                product += "\" )\n";
            }
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
        JobDescription::logger.msg(DEBUG, "[XRSLParser] Converting to XRSL");
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
	        JobDescription::logger.msg(DEBUG, "[JDLParser] This kind of JDL decriptor is not supported yet: %s", value);
                return false; // This kind of JDL decriptor is not supported yet
            }
            if ( value == "collection" ) {
                JobDescription::logger.msg(DEBUG, "[JDLParser] This kind of JDL decriptor is not supported yet: %s", value);
                return false; // This kind of JDL decriptor is not supported yet
            }
            JobDescription::logger.msg(DEBUG, "[JDLParser] Attribute name: %s, has unknown value: %s", attributeName , value);
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
                source.URI = *it;
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
//            innerRepresentation.JDL_elements["inputsandboxbaseuri"] = simpleJDLvalue( attributeValue ); 

            std::list<Arc::FileType>::iterator iter;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                std::list<Arc::SourceType>::iterator i_source;
                for (i_source = (*iter).Source.begin(); i_source != (*iter).Source.end(); i_source++){
                    if ( (*i_source).URI.Host() == "" )
                       (*i_source).URI.ChangeHost(simpleJDLvalue( attributeValue ));
                }
            }
            std::list<Arc::DirectoryType>::iterator iter_d;
            for (iter_d = innerRepresentation.Directory.begin(); iter_d != innerRepresentation.Directory.end(); iter_d++){
                std::list<Arc::SourceType>::iterator i_source;
                for (i_source = (*iter).Source.begin(); i_source != (*iter).Source.end(); i_source++){
                    if ( (*i_source).URI.Host() == "" )
                       (*i_source).URI.ChangeHost(simpleJDLvalue( attributeValue ));
                }
            }
            return true;
        } else if ( attributeName == "outputsandbox" ) {
            std::vector<std::string> inputfiles = listJDLvalue( attributeValue );
            for (std::vector<std::string>::const_iterator it=inputfiles.begin(); it<inputfiles.end(); it++) {
                Arc::FileType file;
                file.Name = (*it);
                Arc::TargetType target;
                target.URI = *it;
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
                      (*it).Target.begin()->URI = value[i];
                      i++;
                   }
                   else {
		      JobDescription::logger.msg(DEBUG, "Not enought outputsandboxdesturi element!");
                      return false;
                   }

                }
            }
            return true;
        } else if ( attributeName == "outputsandboxbaseuri" ) {
//            innerRepresentation.JDL_elements["outputsandboxbaseuri"] = simpleJDLvalue( attributeValue ); 

            std::list<Arc::FileType>::iterator iter;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                std::list<Arc::TargetType>::iterator i_target;
                for (i_target = (*iter).Target.begin(); i_target != (*iter).Target.end(); i_target++){
                    if ( (*i_target).URI.Host() == "" )
                       (*i_target).URI.ChangeHost(simpleJDLvalue( attributeValue ));
                }
            }
            std::list<Arc::DirectoryType>::iterator iter_d;
            for (iter_d = innerRepresentation.Directory.begin(); iter_d != innerRepresentation.Directory.end(); iter_d++){
                std::list<Arc::TargetType>::iterator i_target;
                for (i_target = (*iter).Target.begin(); i_target != (*iter).Target.end(); i_target++){
                    if ( (*i_target).URI.Host() == "" )
                       (*i_target).URI.ChangeHost(simpleJDLvalue( attributeValue ));
                }
            }
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
		    JobDescription::logger.msg(DEBUG, "[JDLParser] Environment variable has been defined without any equal sign.");
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
	JobDescription::logger.msg(DEBUG, "[JDL Parser]: Unknown attribute name: \'%s\', with value: %s", attributeName, attributeValue);
        return false;
    }

    bool JDLParser::parse( Arc::JobInnerRepresentation& innerRepresentation, const std::string source ) {
        unsigned long first = source.find_first_of( "[" );
        unsigned long last = source.find_last_of( "]" );
        if ( first == std::string::npos || last == std::string::npos ) {
	    JobDescription::logger.msg(DEBUG, "[JDLParser] There is at least one necessary ruler character missing. ('[' or ']')");
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
	    JobDescription::logger.msg(DEBUG, "[JDLParser] Syntax error found during the split function.");
            return false;
        }
        if (lines.size() <= 0) {
	    JobDescription::logger.msg(DEBUG, "[JDLParser] Lines count is zero or other funny error has occurred.");
            return false;
        }
        
        for ( unsigned long i=0; i < lines.size(); i++) {
            unsigned long equal_pos = lines[i].find_first_of("=");
            if ( equal_pos == std::string::npos ) {
                if ( i == lines.size()-1 ) continue;
                else {
		    JobDescription::logger.msg(DEBUG, "[JDLParser] JDL syntax error. There is at least one equal sign missing where it would be expected.");
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
        if (!innerRepresentation.File.empty()) {
            std::list<Arc::FileType>::const_iterator iter;
            bool first = true;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                if ( !(*iter).Target.empty()) {
                   if (!first) {
                      product += ",";
                   }
                   else {
                      product += "  OutputSandbox = {";
                      first = false;
                   }
                   product += " \"" +  (*iter).Name + "\"";
                }
            }
            if ( !first )
               product += " };\n";
        }
        if (!innerRepresentation.File.empty()) {
            bool first = true;
            std::list<Arc::FileType>::const_iterator iter;
            for (iter = innerRepresentation.File.begin(); iter != innerRepresentation.File.end(); iter++){
                std::list<Arc::TargetType>::const_iterator it_target;
                for (it_target = (*iter).Target.begin(); it_target != (*iter).Target.end(); it_target++){
                    if ( first ){
                       product += "  OutputSandboxDestURI = {\n";
                       first = false;
                    }
                    product += "    \"" +  (*it_target).URI.fullstr() + "\"";
                    if (it_target++ != (*iter).Target.end()) {
                       product += ",";
                       it_target--;
                    }
                    product += "\n";
                }
            }
            if ( !first)
               product += "    };\n";
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
