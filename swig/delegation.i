%{
#include <arc/delegation/DelegationInterface.h>
%}

#ifdef SWIGPYTHON
%ignore Arc::DelegationConsumer::Acquire(std::string&, std::string&);
%ignore Arc::DelegationConsumerSOAP::UpdateCredentials(std::string&, std::string&, const SOAPEnvelope&, SOAPEnvelope&);
%ignore Arc::DelegationConsumerSOAP::DelegatedToken(std::string&, std::string&, const SOAPEnvelope&, SOAPEnvelope&);
%ignore Arc::DelegationConsumerSOAP::DelegatedToken(std::string&, std::string&, const XMLNode&);
%ignore Arc::DelegationContainerSOAP::UpdateCredentials(std::string&, std::string&, const SOAPEnvelope&, SOAPEnvelope&);
%ignore Arc::DelegationContainerSOAP::UpdateCredentials(std::string&, const Arc::SOAPEnvelope&, Arc::SOAPEnvelope &);
%ignore Arc::DelegationContainerSOAP::DelegatedToken(std::string&, std::string&, const SOAPEnvelope&, SOAPEnvelope&);
%ignore Arc::DelegationContainerSOAP::DelegatedToken(std::string&, std::string&, const XMLNode&);
#endif

#ifdef SWIGPYTHON
%apply std::string& OUTPUT { std::string& credentials };
%apply std::string& OUTPUT { std::string& identity };

%apply std::string& OUTPUT { std::string& content };
/* ... applies to:
 * bool DelegationConsumer::Backup(std::string& content);
 * bool DelegationConsumer::Request(std::string& content);
 * bool DelegationConsumer::Acquire(std::string& content);
 */
#endif
%include "../src/hed/libs/delegation/DelegationInterface.h"
#ifdef SWIGPYTHON
%clear std::string& identity;
%clear std::string& credentials;
%clear std::string& content;
#endif
