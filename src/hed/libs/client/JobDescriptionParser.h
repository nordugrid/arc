#ifndef __ARC_JOBDESCRIPTIONPARSER_H__
#define __ARC_JOBDESCRIPTIONPARSER_H__

#include <string>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <time.h>
#include <arc/StringConv.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>
#include <arc/client/JobInnerRepresentation.h>
#include <arc/Logger.h>


namespace Arc {

    #define VERBOSEX 0
    #define DEBUGX 0

    // Define variables for the jobDescriptionOrderer
    #define DEFAULT_PRIORITY 0

    #define EXACT_EXTENSION_MATCHING 10
    #define PARTIAL_EXTENSION_MATCHING 5
    #define TEXT_PATTERN_MATCHING 12
    #define TEXT_PATTERN_MATCHING_WITHOUT_WHITESPACES 10
    #define NEGATIVE_TEXT_PATTERN_MATCHING -8
    #define NEGATIVE_TEXT_PATTERN_MATCHING_WITHOUT_WHITESPACES -6
    // End of jobDescriptionOrderer's define set

    // GIN JSDL elements
    #define GIN_ELEMENTS_NUMBER 42
    static const char *GIN_elements[ GIN_ELEMENTS_NUMBER ] = { "Meta", "Author", "DocumentExpiration", "LogDir",
          "LRMSReRun", "Notification", "Join", "IndividualWallTime", "ReferenceTime",
          "NetworkInfo", "OSFamily", "OSName", "OSVersion", "Platform", "CacheDiskSpace", "SessionDiskSpace",
          "Alias", "EndPointURL", "Location", "Country", "Place", "PostCode", "Latitude", "Longitude", "CEType",
          "Slots", "NumberOfProcesses", "Slots", "ProcessPerHost", "ThreadPerProcesses", "SPMDVariation",
          "Homogeneous", "NodeAccess", "InBound", "OutBound", "Threads", "Mandatory", "NeededReplicas", "KeepData",
          "DataIndexingService", "File", ""}; 
    // JSDL elements with ARC and POSIX extensions
    #define JSDL_ELEMENTS_NUMBER 31
    static const char *JSDL_elements[ JSDL_ELEMENTS_NUMBER ] = { "POSIXApplication", "CPUArchitectureType",
          "CPUArchitectureName", "LocalLogging", "URL", "Reruns", "CPUTimeLimit",
          "WallTimeLimit", "GridTimeLimit", "IndividualNetworkBandwidth", "OperatingSystem", "OperatingSystemName",
          "OperatingSystemVersion", "MemoryLimit", "VirtualMemoryLimit", "Middleware", "ProcessCountLimit",
          "TotalCPUCount", "ThreadCountLimit", "ApplicationName", "ApplicationVersion", "IndividualCPUSpeed",
          "IndividualCPUCount", "TotalPhysicalMemory", "TotalVirtualMemory", "TotalDiskSpace", "TotalResourceCount",
          "FileSystemName", "CreationFlag", "DeleteOnTermination", "" }; 

    class JobDescriptionError : public std::runtime_error {
        public:
            JobDescriptionError(const std::string& what="");
    };

    // The candidate's data structure contains every important attribute //
    struct Candidate {
        // The candidate Type written by text
        std::string typeName;
        // The candidate's chance to be the most possible
        // The greater is more possible
        // 0 - for impossible
        int priority;
        // REMOVED //
        // The parser function which belongs to the actual type.
        // bool (*parser)(std::string&);
        // REMOVED //
        // Possible extensions
        std::vector<std::string> extensions;
        // Possible text pattern
        std::vector<std::string> pattern;
        // Impossible text pattern
        std::vector<std::string> negative_pattern;
    }; // End of struct CandidateType

    class StringManipulator {
        public:
            std::string trim( const std::string original_string ) const;
            std::string toLowerCase( const std::string original_string ) const;
            std::vector<std::string> split( const std::string original_string, const std::string delimiter ) const;
    };

    //Abstract class for the different parsers
    class JobDescriptionParser {
        protected:
            long get_limit(Arc::XMLNode range);
            bool Element_Search( const Arc::XMLNode node, bool posix );
        public:
            virtual bool parse( Arc::JobInnerRepresentation& innerRepresentation, const std::string source ) = 0;
            virtual bool getProduct( const Arc::JobInnerRepresentation& innerRepresentation, std::string& product ) const = 0;
            virtual ~JobDescriptionParser(){};
            static Logger logger;
    };

    class JobDescriptionOrderer {
        private:
          std::string sourceString;
        public:
            void setSource( const std::string source );
            std::vector<Candidate> getCandidateList() const;
            void determinizeCandidates( std::vector<Candidate>& candidates ) const;
    };

} // namespace Arc

#endif // __ARC_JOBDESCRIPTIONPARSER_H__
