#include <stdlib.h>

#include <arc/client/ClientInterface.h>
#include <arc/message/MCC.h>
#include <arc/delegation/DelegationInterface.h>

#include "SimulatorClasses.h"

std::string       Arc::ClientSOAPTest::action = "";
Arc::PayloadSOAP  Arc::ClientSOAPTest::request  = Arc::PayloadSOAP(Arc::NS());
Arc::PayloadSOAP* Arc::ClientSOAPTest::response = NULL;
Arc::MCC_Status   Arc::ClientSOAPTest::status(Arc::GENERIC_ERROR);

Arc::MCC_Status Arc::ClientSOAPTest::process(Arc::PayloadSOAP *request, Arc::PayloadSOAP **response) {
  ClientSOAPTest::request = Arc::PayloadSOAP(*request);
  *response = ClientSOAPTest::response;
  return ClientSOAPTest::status;
}

Arc::MCC_Status Arc::ClientSOAPTest::process(const std::string& action, Arc::PayloadSOAP *request, Arc::PayloadSOAP **response) {
  Arc::ClientSOAPTest::action = action;
  ClientSOAPTest::request = Arc::PayloadSOAP(*request);
  *response = ClientSOAPTest::response;
  return ClientSOAPTest::status;
}

/* DelegationProviderSOAPTest class body specification */

std::string       Arc::DelegationProviderSOAPTest::action = "";
Arc::PayloadSOAP  Arc::DelegationProviderSOAPTest::request  = Arc::PayloadSOAP(Arc::NS());
Arc::PayloadSOAP* Arc::DelegationProviderSOAPTest::response = NULL;
Arc::MCC_Status   Arc::DelegationProviderSOAPTest::status(Arc::GENERIC_ERROR);


bool Arc::DelegationProviderSOAPTest::DelegateCredentialsInit(Arc::MCCInterface& mcc_interface,Arc::MessageContext* context) {
   id_ = "id";
   request_ = "request";
   return true;
}

bool Arc::DelegationProviderSOAPTest::DelegatedToken(XMLNode parent)
{
  XMLNode token = parent.NewChild("deleg:DelegatedToken");
  token.NewAttribute("deleg:Format")="x509";
  token.NewChild("deleg:Id")=id_;
  token.NewChild("deleg:Value")= "delegation";
  return true;
}
