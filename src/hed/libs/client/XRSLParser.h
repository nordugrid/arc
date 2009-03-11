#ifndef __ARC_XRSLPARSER_H__
#define __ARC_XRSLPARSER_H__

#include <string>
#include <vector>
#include <map>
#include <arc/client/JobInnerRepresentation.h>

#include "JobDescriptionParser.h"


namespace Arc {

    class XRSLParser : public JobDescriptionParser {
        private:
            StringManipulator sm;
            std::map<std::string, std::string> rsl_substitutions;
            std::string input_files;
            std::string output_files;
            bool handleXRSLattribute( std::string attributeName, std::string attributeValue, Arc::JobInnerRepresentation& innerRepresentation );
            std::string simpleXRSLvalue( std::string attributeValue ) const;
            std::vector<std::string> listXRSLvalue( std::string attributeValue ) const;
            std::vector< std::vector<std::string> > doubleListXRSLvalue( std::string attributeValue ) const;
        public:
            bool parse( Arc::JobInnerRepresentation& innerRepresentation, const std::string source );
            bool getProduct( const Arc::JobInnerRepresentation& innerRepresentation, std::string& product ) const;
    };

} // namespace Arc

#endif // __ARC_XRSLPARSER_H__
