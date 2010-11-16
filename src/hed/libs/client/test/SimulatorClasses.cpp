#include <stdlib.h>

#include <arc/client/ClientInterface.h>
#include <arc/message/MCC.h>

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


