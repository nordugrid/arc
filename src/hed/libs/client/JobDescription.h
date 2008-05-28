#ifndef __ARC_JOBDESCRIPTION_H__
#define __ARC_JOBDESCRIPTION_H__

#include <string>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <time.h>
#include <arc/XMLNode.h>

namespace Arc {

    #define VERBOSEX 1
    #define DEBUG 1

    // Define variables for the jobDescriptionOrderer
    #define DEFAULT_PRIORITY 0

    #define EXACT_EXTENSION_MATCHING 10
    #define PARTIAL_EXTENSION_MATCHING 5
    #define TEXT_PATTERN_MATCHING 12
    #define TEXT_PATTERN_MATCHING_WITHOUT_WHITESPACES 10
    #define NEGATIVE_TEXT_PATTERN_MATCHING -8
    #define NEGATIVE_TEXT_PATTERN_MATCHING_WITHOUT_WHITESPACES -6
    // End of jobDescriptionOrderer's define set


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
            std::string trim( const std::string original_string );
            std::string toLowerCase( const std::string original_string );
            std::vector<std::string> split( const std::string original_string, const std::string delimiter );
    };

    //Abstract class for the different parsers
    class JobDescriptionParser {
        public:
            virtual bool parse( Arc::XMLNode& jobTree, const std::string source ) = 0;
            virtual bool getProduct( const Arc::XMLNode& jobTree, std::string& product ) = 0;
    };

    class JSDLParser : public JobDescriptionParser {
        public:
            bool parse( Arc::XMLNode& jobTree, const std::string source );
            bool getProduct( const Arc::XMLNode& jobTree, std::string& product );
    };

    class XRSLParser : public JobDescriptionParser {
        private:
            StringManipulator sm;
            bool handleXRSLattribute( std::string attributeName, std::string attributeValue, Arc::XMLNode& jobTree );
            std::string simpleXRSLvalue( std::string attributeValue );
            std::vector<std::string> listXRSLvalue( std::string attributeValue );
            std::vector< std::vector<std::string> > doubleListXRSLvalue( std::string attributeValue );
        public:
            bool parse( Arc::XMLNode& jobTree, const std::string source );
            bool getProduct( const Arc::XMLNode& jobTree, std::string& product );
    };

    class JDLParser : public  JobDescriptionParser {
        private:
            StringManipulator sm;
            bool splitJDL(std::string original_string, std::vector<std::string>& lines);
            bool handleJDLattribute( std::string attributeName, std::string attributeValue, Arc::XMLNode& jobTree );
            std::string simpleJDLvalue( std::string attributeValue );
            std::vector<std::string> listJDLvalue( std::string attributeValue );
        public:
            bool parse( Arc::XMLNode& jobTree, const std::string source );
            bool getProduct( const Arc::XMLNode& jobTree, std::string& product );
    };

    class JobDescriptionOrderer {
        private:
          std::string sourceString;
        public:
            void setSource( const std::string source );
            std::vector<Candidate> getCandidateList();
            void determinizeCandidates( std::vector<Candidate>& candidates );
    };

    class JobDescription {
        private:
            StringManipulator sm;
            Arc::XMLNode jobTree;
            std::string sourceString;
            std::string sourceFormat;
            JobDescriptionOrderer jdOrderer;
            void resetJobTree();
        public:
            JobDescription();
            bool isValid() {return (sourceFormat.length() != 0);};
            void setSource( const std::string source ) throw(JobDescriptionError);
            void getProduct( std::string& product, std::string format = "JSDL" ) throw(JobDescriptionError);
            Arc::XMLNode getXML() throw(JobDescriptionError);
    };

} // namespace Arc

#endif // __ARC_JOBDESCRIPTION_H__