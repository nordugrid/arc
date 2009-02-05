#ifndef __ARC_JOBDESCRIPTION_H__
#define __ARC_JOBDESCRIPTION_H__

#include <string>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <time.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>
#include <arc/client/JobInnerRepresentation.h>


namespace Arc {

    #define VERBOSEX 0
    #define DEBUGX 0

    // Define variables for the jobDescriptionOrderer
    #define DEFAULT_PRIORITY 0
    #define TOP_PRIORITY 50

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
            std::string trim( const std::string original_string ) const;
            std::string toLowerCase( const std::string original_string ) const;
            std::vector<std::string> split( const std::string original_string, const std::string delimiter ) const;
    };

    //Abstract class for the different parsers
    class JobDescriptionParser {
        public:
            virtual bool parse( Arc::JobInnerRepresentation& innerRepresentation, const std::string source ) = 0;
            virtual bool getProduct( const Arc::JobInnerRepresentation& innerRepresentation, std::string& product ) = 0;
            virtual ~JobDescriptionParser(){};
    };

    class PosixJSDLParser : public JobDescriptionParser {
        private:
            StringManipulator sm;
        public:
            bool parse( Arc::JobInnerRepresentation& innerRepresentation, const std::string source );
            //bool handleJSDLattribute( std::string attributeName, std::string attributeValue, Arc::JobInnerRepresentation& innerRepresentation );
            bool getProduct( const Arc::JobInnerRepresentation& innerRepresentation, std::string& product );
    };

    class JSDLParser : public JobDescriptionParser {
        private:
            StringManipulator sm;
        public:
            bool parse( Arc::JobInnerRepresentation& innerRepresentation, const std::string source );
            //bool handleJSDLattribute( std::string attributeName, std::string attributeValue, Arc::JobInnerRepresentation& innerRepresentation );
            bool getProduct( const Arc::JobInnerRepresentation& innerRepresentation, std::string& product );
    };

    class XRSLParser : public JobDescriptionParser {
        private:
            StringManipulator sm;
            bool handleXRSLattribute( std::string attributeName, std::string attributeValue, Arc::JobInnerRepresentation& innerRepresentation );
            std::string simpleXRSLvalue( std::string attributeValue );
            std::vector<std::string> listXRSLvalue( std::string attributeValue );
            std::vector< std::vector<std::string> > doubleListXRSLvalue( std::string attributeValue );
        public:
            bool parse( Arc::JobInnerRepresentation& innerRepresentation, const std::string source );
            bool getProduct( const Arc::JobInnerRepresentation& innerRepresentation, std::string& product );
    };

    class JDLParser : public  JobDescriptionParser {
        private:
            StringManipulator sm;
            bool splitJDL(std::string original_string, std::vector<std::string>& lines);
            bool handleJDLattribute( std::string attributeName, std::string attributeValue, Arc::JobInnerRepresentation& innerRepresentation );
            std::string simpleJDLvalue( std::string attributeValue );
            std::vector<std::string> listJDLvalue( std::string attributeValue );
        public:
            bool parse( Arc::JobInnerRepresentation& innerRepresentation, const std::string source );
            bool getProduct( const Arc::JobInnerRepresentation& innerRepresentation, std::string& product );
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
            Arc::JobInnerRepresentation* innerRepresentation;
            void resetJobTree();
        public:
            JobDescription();
            JobDescription(const JobDescription& desc);
            ~JobDescription();

            // Returns with true if the source has been setted up and wasn't any syntax error in it and there
            // is a mandatory (Executable) element setted correctly or false otherwise
            bool isValid() const {return (sourceFormat.length() != 0) && 
                                    !(*innerRepresentation).Executable.empty();};

            // Try to parse the source string and store it in an inner job description representation.
            // If there is some syntax error then it throws an exception.
            bool setSource( const std::string source );

            // Transform the inner job description representation into a given format, if it's known as a parser (JSDL as default)
            // If there is some error during this method, then return with false.
            bool getProduct( std::string& product, std::string format = "POSIXJSDL" );

            // Returns with the original job descriptions format as a string. Right now, this value is one of the following:
            // "jsdl", "jdl", "xrsl". If there is an other parser written for another language, then this set can be extended.
            bool getSourceFormat( std::string& _sourceFormat );

            // Returns with the XML representation of the job description. This inner representation is very similar
            // to the JSDL structure and exactly the same in cases, which are defined in the JSDL specification.
            bool getXML( Arc::XMLNode& node );

            // Returns with true and the uploadable local files list, if the source has been setted up and is valid, else return with false.
            bool getUploadableFiles( std::vector< std::pair< std::string, std::string > >& sourceFiles );

            // Returns with the inner representation object.
            bool getInnerRepresentation( Arc::JobInnerRepresentation& job );
    };

} // namespace Arc

#endif // __ARC_JOBDESCRIPTION_H__
