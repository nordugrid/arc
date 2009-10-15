#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <signal.h>
#include <stdexcept>

#include <glibmm.h>

#include <arc/User.h>
#include <arc/StringConv.h>
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

//The following is for showing how to use the specific client API
// (ClientX509Delegation) to delegate a proxy to ARC delegation 
//service, and gLite (gridsite) delegation service individually.
int main(void) {
  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  Arc::Logger logger(Arc::Logger::rootLogger, "Test");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::rootLogger.addDestination(logcerr);

  /******** Test to ARC delegation service **********/
  std::string arc_deleg_url_str("https://127.0.0.1:60000/delegation");
//  std::string arc_deleg_url_str("https://glueball.uio.no:60000/delegation");
  Arc::URL arc_deleg_url(arc_deleg_url_str);
  Arc::MCCConfig arc_deleg_mcc_cfg;
  arc_deleg_mcc_cfg.AddPrivateKey("../echo/testkey-nopass.pem");
  arc_deleg_mcc_cfg.AddCertificate("../echo/testcert.pem");
  arc_deleg_mcc_cfg.AddCAFile("../echo/testcacert.pem");
  arc_deleg_mcc_cfg.AddCADir("../echo/certificates");
  //Create a delegation SOAP client 
  logger.msg(Arc::INFO, "Creating a delegation soap client");
  Arc::ClientX509Delegation *arc_deleg_client = NULL;
  arc_deleg_client = new Arc::ClientX509Delegation(arc_deleg_mcc_cfg, arc_deleg_url);
  std::string arc_delegation_id;
  if(arc_deleg_client) {
    if(!(arc_deleg_client->createDelegation(Arc::DELEG_ARC, arc_delegation_id))) {
      logger.msg(Arc::ERROR, "Delegation to ARC delegation service failed");
      if(arc_deleg_client) delete arc_deleg_client;
      return 1;
    }
  }
  logger.msg(Arc::INFO, "Delegation ID: %s", arc_delegation_id.c_str());
  if(arc_deleg_client) delete arc_deleg_client;  

  /******** Test to gridsite delegation service **********/
  std::string gs_deleg_url_str("https://cream.grid.upjs.sk:8443/ce-cream/services/gridsite-delegation");
  Arc::URL gs_deleg_url(gs_deleg_url_str);
  Arc::MCCConfig gs_deleg_mcc_cfg;
  //Somehow gridsite delegation service only accepts proxy certificate,
  //not the EEC certificate, so we need to generate the proxy certificate
  //firstly.
  //Note the proxy needs to be generated before running this test.
  //And the proxy should be created by using a credential signed by officially
  //certified CAs, if the peer gridside delegation service only trusts
  //official CAs, not testing CA such as the InstantCA. It also applies to
  //the delegation to ARC delegation service.
  Arc::User user;
  std::string proxy_path  = Glib::build_filename(Glib::get_tmp_dir(),"x509up_u" + Arc::tostring(user.get_uid()));
  gs_deleg_mcc_cfg.AddProxy(proxy_path);
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
      if(gs_deleg_client) delete gs_deleg_client;
      return 1;
    }
  }
  logger.msg(Arc::INFO, "Delegation ID: %s", gs_delegation_id.c_str());
  if(gs_deleg_client) delete gs_deleg_client;

  return 0;
}
