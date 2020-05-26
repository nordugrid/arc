#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <signal.h>
#include <stdexcept>

#include <arc/GUID.h>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/MCC.h>
#include <arc/communication/ClientInterface.h>
#include <arc/communication/ClientX509Delegation.h>

int main(void) {
  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  Arc::Logger logger(Arc::Logger::rootLogger, "Test");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::rootLogger.addDestination(logcerr);

  /******** Test to ARC delegation service **********/

  std::string arc_deleg_url_str("https://127.0.0.1:60000/delegation");
  //std::string arc_deleg_url_str("https://glueball.uio.no:60000/delegation");
  Arc::URL arc_deleg_url(arc_deleg_url_str);
  Arc::MCCConfig arc_deleg_mcc_cfg;
  arc_deleg_mcc_cfg.AddPrivateKey("../echo/testuserkey-nopass.pem");
  arc_deleg_mcc_cfg.AddCertificate("../echo/testusercert.pem");
  arc_deleg_mcc_cfg.AddCAFile("../echo/testcacert.pem");
  arc_deleg_mcc_cfg.AddCADir("../echo/certificates");

  //Create a delegation SOAP client 
  logger.msg(Arc::INFO, "Creating a delegation soap client");
  Arc::ClientX509Delegation *arc_deleg_client = NULL;
  arc_deleg_client = new Arc::ClientX509Delegation(arc_deleg_mcc_cfg, arc_deleg_url);

  //Delegate a credential to the delegation service.
  //The credential inside mcc configuration is used for delegating.
  std::string arc_delegation_id;
  if(arc_deleg_client) {
    if(!(arc_deleg_client->createDelegation(Arc::DELEG_ARC, arc_delegation_id))) {
      logger.msg(Arc::ERROR, "Delegation to ARC delegation service failed");
      throw std::runtime_error("Delegation to ARC delegation service failed");
    }
  }
  logger.msg(Arc::INFO, "Delegation ID: %s", arc_delegation_id.c_str());

  //Acquire the delegated credential from the delegation service
  std::string arc_delegation_cred;
  if(!arc_deleg_client->acquireDelegation(Arc::DELEG_ARC, arc_delegation_cred, arc_delegation_id)) {
    logger.msg(Arc::ERROR,"Can not get the delegation credential: %s from delegation service: %s",arc_delegation_id.c_str(),arc_deleg_url_str.c_str());
    throw std::runtime_error("Can not get the delegation credential from delegation service");
  }
  logger.msg(Arc::INFO, "Delegated credential from delegation service: %s", arc_delegation_cred.c_str());

  if(arc_deleg_client) delete arc_deleg_client;

  return 0;
}
