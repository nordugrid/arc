#ifndef __ARC_POSIXJSDLPARSER_H__
#define __ARC_POSIXJSDLPARSER_H__

#include <string>
#include <arc/client/JobInnerRepresentation.h>

#include "JobDescriptionParser.h"


namespace Arc {

    class PosixJSDLParser : public JobDescriptionParser {
        public:
            bool parse( Arc::JobInnerRepresentation& innerRepresentation, const std::string source );
            bool getProduct( const Arc::JobInnerRepresentation& innerRepresentation, std::string& product ) const;
    };

} // namespace Arc

#endif // __ARC_POSIXJSDLPARSER_H__
