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


  /******** Test to gridsite delegation service **********/
  std::string gs_deleg_url_str("https://cream.grid.upjs.sk:8443/ce-cream/services/gridsite-delegation");
  Arc::URL gs_deleg_url(gs_deleg_url_str);
  Arc::MCCConfig gs_deleg_mcc_cfg;
 
  //Use voms-proxy-init or arcproxy to generate a gsi legacy proxy, and 
  //put put it into mcc configuraton by using "AddProxy"
  gs_deleg_mcc_cfg.AddProxy("proxy.pem");

  //gs_deleg_mcc_cfg.AddPrivateKey("../echo/userkey-nopass.pem");
  //gs_deleg_mcc_cfg.AddCertificate("../echo/usercert.pem");
  gs_deleg_mcc_cfg.AddCADir("../echo/certificates");
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
