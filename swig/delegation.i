%{
#include <arc/delegation/DelegationInterface.h>
%}

%apply std::string& OUTPUT { std::string& credentials };
%apply std::string& OUTPUT { std::string& identity };
namespace Arc {
    %ignore DelegationContainerSOAP::UpdateCredentials(std::string &,const SOAPEnvelope &, SOAPEnvelope &);
}
%include "../src/hed/libs/delegation/DelegationInterface.h"
%clear std::string& identity;
%clear std::string& credentials;
