%{
#include <arc/delegation/DelegationInterface.h>
%}

#ifdef SWIGPYTHON
%ignore Arc::DelegationConsumer::Acquire(std::string&, std::string&);
%ignore Arc::DelegationConsumerSOAP::UpdateCredentials(std::string&, std::string&, const SOAPEnvelope&, SOAPEnvelope&);
%ignore Arc::DelegationConsumerSOAP::DelegatedToken(std::string&, std::string&, const SOAPEnvelope&, SOAPEnvelope&);
%ignore Arc::DelegationConsumerSOAP::DelegatedToken(std::string&, std::string&, const XMLNode&);
%ignore Arc::DelegationContainerSOAP::UpdateCredentials(std::string&, std::string&, const SOAPEnvelope&, SOAPEnvelope&);
%ignore Arc::DelegationContainerSOAP::DelegatedToken(std::string&, std::string&, const SOAPEnvelope&, SOAPEnvelope&);
%ignore Arc::DelegationContainerSOAP::DelegatedToken(std::string&, std::string&, const XMLNode&);
#endif

%apply std::string& OUTPUT { std::string& credentials };
%apply std::string& OUTPUT { std::string& identity };
%ignore Arc::DelegationContainerSOAP::UpdateCredentials(std::string &,const Arc::SOAPEnvelope &, Arc::SOAPEnvelope &);
%include "../src/hed/libs/delegation/DelegationInterface.h"
%clear std::string& identity;
%clear std::string& credentials;
