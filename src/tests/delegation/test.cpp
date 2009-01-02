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
#include <arc/client/ClientInterface.h>
#include <arc/client/ClientX509Delegation.h>
#ifdef WIN32
#include <arc/win32.h>
#endif

int main(void) {
  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  Arc::Logger logger(Arc::Logger::rootLogger, "Test");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::rootLogger.addDestination(logcerr);

  /******** Test to ARC delegation service **********/
  std::string arc_deleg_url_str("https://127.0.0.1:60000/delegation");
  Arc::URL arc_deleg_url(arc_deleg_url_str);
  Arc::MCCConfig arc_deleg_mcc_cfg;
  arc_deleg_mcc_cfg.AddPrivateKey("userkey-nopass.pem");
  arc_deleg_mcc_cfg.AddCertificate("usercert.pem");
  //arc_deleg_mcc_cfg.AddCAFile("cacert.pem");
  arc_deleg_mcc_cfg.AddCADir("certificates");
  //Create a delegation SOAP client 
  logger.msg(Arc::INFO, "Creating a delegation soap client");
  Arc::ClientX509Delegation *arc_deleg_client = NULL;
  arc_deleg_client = new Arc::ClientX509Delegation(arc_deleg_mcc_cfg, arc_deleg_url);
  std::string arc_delegation_id;
  if(arc_deleg_client) {
    if(!(arc_deleg_client->createDelegation(Arc::DELEG_ARC, arc_delegation_id))) {
      logger.msg(Arc::ERROR, "Delegation to ARC delegation service failed");
      throw std::runtime_error("Delegation to ARC delegation service failed");
    }
  }
  logger.msg(Arc::INFO, "Delegation ID: %s", arc_delegation_id.c_str());
  if(arc_deleg_client) delete arc_deleg_client;  

  /******** Test to gridsite delegation service **********/
  std::string gs_deleg_url_str("https://cream.grid.upjs.sk:8443/ce-cream/services/gridsite-delegation");
  Arc::URL gs_deleg_url(gs_deleg_url_str);
  Arc::MCCConfig gs_deleg_mcc_cfg;
  gs_deleg_mcc_cfg.AddProxy("x509up_u126587");
  //gs_deleg_mcc_cfg.AddPrivateKey("userkey-nopass.pem");
  //gs_deleg_mcc_cfg.AddCertificate("usercert.pem");
  gs_deleg_mcc_cfg.AddCADir("certificates");
  //Create a delegation SOAP client
  logger.msg(Arc::INFO, "Creating a delegation soap client");
  Arc::ClientX509Delegation *gs_deleg_client = NULL;
  gs_deleg_client = new Arc::ClientX509Delegation(gs_deleg_mcc_cfg, gs_deleg_url);
  std::string gs_delegation_id;
  gs_delegation_id = Arc::UUID();
  if(gs_deleg_client) {
    if(!(gs_deleg_client->createDelegation(Arc::DELEG_GRIDSITE, gs_delegation_id))) {
      logger.msg(Arc::ERROR, "Delegation to gridsite delegation service failed");
      throw std::runtime_error("Delegation to gridsite delegation service failed");
    }
  }
  logger.msg(Arc::INFO, "Delegation ID: %s", gs_delegation_id.c_str());
  if(gs_deleg_client) delete gs_deleg_client;

  return 0;
}
