#include <cstring>
#include <algorithm>
#include <sys/stat.h>
#include <fcntl.h>
#include <exception>

#include <arc/StringConv.h>

#include "JobDescriptionParser.h"


namespace Arc {
    Arc::Logger JobDescriptionParser::logger(Arc::Logger::getRootLogger(), "jobdescription");

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
    void JobDescriptionOrderer::determinizeCandidates( std::vector<Candidate>& candidates ) const {

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
    std::vector<Candidate> JobDescriptionOrderer::getCandidateList() const {

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

        std::string jobDescription = lower(sourceString);

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
                if ( jobDescription.find( lower(*j) ) != std::string::npos ) {
                    JobDescriptionParser::logger.msg(DEBUG, "[JobDescriptionOrderer]\tText pattern matching: \"%s\" to %s", *j, (*i).typeName);
                    (*i).priority += TEXT_PATTERN_MATCHING;
                } else {
                    if ( jobDescriptionWithoutWhitespaces.find( lower(*j) ) != std::string::npos ) {
                    JobDescriptionParser::logger.msg(DEBUG, "[JobDescriptionOrderer]\tText pattern matching: \"%s\" to %s", *j, (*i).typeName);
                        (*i).priority += TEXT_PATTERN_MATCHING_WITHOUT_WHITESPACES;
                    }
                }
            }
            for ( std::vector<std::string>::const_iterator j = (*i).negative_pattern.begin(); j != (*i).negative_pattern.end(); j++ ) {
                if ( jobDescription.find( lower(*j) ) != std::string::npos ) {
                    JobDescriptionParser::logger.msg(DEBUG, "[JobDescriptionOrderer]\tText negative_pattern matching: \"%s\" to %s", *j, (*i).typeName);
                    (*i).priority += NEGATIVE_TEXT_PATTERN_MATCHING;
                } else {
                    if ( jobDescriptionWithoutWhitespaces.find( lower(*j) ) != std::string::npos ) {
                        JobDescriptionParser::logger.msg(DEBUG, "[JobDescriptionOrderer]\tText negative_pattern matching: \"%s\" to %s", *j, (*i).typeName);
                        (*i).priority += NEGATIVE_TEXT_PATTERN_MATCHING_WITHOUT_WHITESPACES;
                    }
                }
            }
        }

        // Sort candidates by priority
        std::sort(candidates.begin(), candidates.end(), CompareDesc<Candidate>() );

        return candidates;
    }


    long JobDescriptionParser::get_limit(Arc::XMLNode range)
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
    bool JobDescriptionParser::Element_Search( const Arc::XMLNode node, bool posix ) {
        std::string parser_name("[JSDLParser]");
        if (posix) parser_name = "[PosixJSDLParser]";
        // check all child nodes
        for (int i=0; bool(node.Child(i)); i++) {
            Arc::XMLNode it(node.Child(i));
            if ( it.Size() > 0 ){
                std::ostringstream message ;
                message << parser_name << " " << it.Name() << " has " << it.Size() << " child(s).";
                logger.msg(DEBUG, message.str());
            }
            if ( i==0 || i>0 && node.Child(i).Name() != node.Child(i-1).Name() ){
                logger.msg(DEBUG, parser_name+ " element: " + it.Name());
                // The element is not valid in this job description.
                if ( posix ){
                    // POSIX-JSDL
                    int size = sizeof(GIN_elements)/sizeof(std::string) ;
                    if ( find(&GIN_elements[0],&GIN_elements[size-1],it.Name()) != &GIN_elements[size-1]){
                        logger.msg(DEBUG, parser_name + " Not valid JSDL: \"" + it.Name() + "\" is GIN-JSDL element!");
                        return false;
                    }
                }
                else{
                    // GIN-JSDL
                    int size = sizeof(JSDL_elements)/sizeof(std::string) ;
                    if ( find(&JSDL_elements[0],&JSDL_elements[size-1],it.Name()) != &JSDL_elements[size-1]){
                        logger.msg(DEBUG, parser_name + " Not valid GIN-JSDL: \"" + it.Name() + "\" is POSIX-JSDL element!");
                        return false;
                    }
                }
            }
            if ( !Element_Search(it, posix ) ) return false;
        }
        return true;
    }

} // namespace Arc
