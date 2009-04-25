#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// perftest_slcs.cpp

#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <glibmm/thread.h>
#include <glibmm/timer.h>

#include <arc/GUID.h>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/MCC.h>
#include <arc/client/ClientInterface.h>
#include <arc/client/ClientSAML2SSO.h>
#include <arc/credential/Credential.h>

#include <arc/xmlsec/XmlSecUtils.h>

// Some global shared variables...
Glib::Mutex* mutex;
bool run;
int finishedThreads;
unsigned long completedRequests;
unsigned long failedRequests;
unsigned long totalRequests;
Glib::TimeVal completedTime;
Glib::TimeVal failedTime;
Glib::TimeVal totalTime;
std::string url_str;
std::string idp_name;
std::string username;
std::string password;

// Round off a double to an integer.
int Round(double x){
  return int(x+0.5);
}

// Send requests and collect statistics.
void sendRequests(){
  // Some variables...
  unsigned long completedRequests = 0;
  unsigned long failedRequests = 0;
  Glib::TimeVal completedTime(0,0);
  Glib::TimeVal failedTime(0,0);
  Glib::TimeVal tBefore;
  Glib::TimeVal tAfter;
  bool connected;

  Arc::URL url(url_str);

  Arc::MCCConfig mcc_cfg;
  mcc_cfg.AddCAFile("../../services/slcs/cacert2.pem");
  mcc_cfg.AddCADir("../echo/certificates");

  Arc::NS slcs_ns;
  slcs_ns["slcs"] = "http://www.nordugrid.org/schemas/slcs";
 
  while(run){
    // Create a Client.
    Arc::ClientSOAPwithSAML2SSO *client = NULL;
    client = new Arc::ClientSOAPwithSAML2SSO(mcc_cfg,url);

    connected=true;
    //while(run and connected){
      // Prepare the request and time it.
      Arc::PayloadSOAP req(slcs_ns);

      tBefore.assign_current_time();

      Arc::Time t;
      int lifetime = 12;
      int keysize = 1024;
      Arc::Credential request(t, Arc::Period(lifetime * 3600), keysize, "EEC");
      std::string cert_req_str;
      if (!request.GenerateRequest(cert_req_str))
        throw std::runtime_error("Failed to generate certificate request");

      std::string private_key;
      request.OutputPrivatekey(private_key);

      req.NewChild("GetSLCSCertificateRequest").NewChild("X509Request") = cert_req_str;

      // Send the request
      Arc::PayloadSOAP* resp = NULL;
      Arc::MCC_Status status = client->process(&req,&resp, idp_name, username, password);

      tAfter.assign_current_time();
 
      if(!status) {
        // Request failed.
        failedRequests++;
        failedTime+=tAfter-tBefore;
	connected=false;
      } else {
        if(resp == NULL) {
          // Response was not SOAP or no response at all.
          failedRequests++;
          failedTime+=tAfter-tBefore;
          connected=false;
        } else {
          if (std::string((*resp)["GetSLCSCertificateResponse"]["X509Certificate"]).size()==0){
            // The response was not what it should be.
            failedRequests++;
            failedTime+=tAfter-tBefore;
            connected=false;
          }
          else{
            // Everything worked just fine!
            completedRequests++;
            completedTime+=tAfter-tBefore;
          
            std::string cert_str = (std::string)((*resp)["GetSLCSCertificateResponse"]["X509Certificate"]);
            std::string ca_str = (std::string)((*resp)["GetSLCSCertificateResponse"]["CACertificate"]);
          }
        }
      }
      if(resp) delete resp;
    //}
    if(client) delete client;
  }

  // Update global variables.
  Glib::Mutex::Lock lock(*mutex);
  ::completedRequests+=completedRequests;
  ::failedRequests+=failedRequests;
  ::completedTime+=completedTime;
  ::failedTime+=failedTime;
  finishedThreads++;
  std::cout << "Number of finished threads: " << finishedThreads << std::endl;
}

int main(int argc, char* argv[]){
  // Some variables...
  int numberOfThreads;
  int duration;
  int i;
  Glib::Thread** threads;
  const char* config_file = NULL;
  int debug_level = -1;
  Arc::LogStream logcerr(std::cerr);

  // Process options - quick hack, must use Glib options later
  while(argc >= 3) {
    if(strcmp(argv[1],"-c") == 0) {
      config_file = argv[2];
      argv[2]=argv[0]; argv+=2; argc-=2;
    } else if(strcmp(argv[1],"-d") == 0) {
      debug_level=Arc::string_to_level(argv[2]);
      argv[2]=argv[0]; argv+=2; argc-=2;
    } else {
      break;
    };
  } 
  if(debug_level >= 0) {
    Arc::Logger::getRootLogger().setThreshold((Arc::LogLevel)debug_level);
    Arc::Logger::getRootLogger().addDestination(logcerr);
  }
  // Extract command line arguments.
  if (argc!=7){
    std::cerr << "Wrong number of arguments!" << std::endl
	      << std::endl
	      << "Usage:" << std::endl
	      << "perftest [-c config] [-d debug] url idpname username password threads duration" << std::endl
	      << std::endl
	      << "Arguments:" << std::endl
	      << "url     The url of the slcs service." << std::endl
              << "idpname   The name of the SP, e.g. https://squark.uio.no/idp/shibboleth" << std::endl
              << "username  The username to IdP " << std::endl
              << "password  The password to IdP   " << std::endl
	      << "threads  The number of concurrent requests." << std::endl
	      << "duration The duration of the test in seconds." << std::endl
	      << "config   The file containing client chain XML configuration with " << std::endl
              << "         'soap' entry point and HOSTNAME, PORTNUMBER and PATH " << std::endl
              << "         keyword for hostname, port and HTTP path of 'echo' service." << std::endl
	      << "debug    The textual representation of desired debug level. Available " << std::endl
              << "         levels: VERBOSE, DEBUG, INFO, WARNING, ERROR, FATAL." << std::endl;
    exit(EXIT_FAILURE);
  }
  url_str = std::string(argv[1]);
  idp_name = std::string(argv[2]);
  username = std::string(argv[3]);
  password = std::string(argv[4]);
  numberOfThreads = atoi(argv[5]);
  duration = atoi(argv[6]);

  Arc::init_xmlsec();

  // Start threads.
  run=true;
  finishedThreads=0;
  //Glib::thread_init();
  mutex=new Glib::Mutex;
  threads = new Glib::Thread*[numberOfThreads];
  for (i=0; i<numberOfThreads; i++) {
    threads[i]=Glib::Thread::create(sigc::ptr_fun(sendRequests),true);
    //Glib::usleep(1000000);
  }

  // Sleep while the threads are working.
  Glib::usleep(duration*1000000);

  // Stop the threads
  run=false;
  while(finishedThreads<numberOfThreads)
    Glib::usleep(100000);

  // Print the result of the test.
  Glib::Mutex::Lock lock(*mutex);
  totalRequests = completedRequests+failedRequests;
  totalTime = completedTime+failedTime;
  std::cout << "========================================" << std::endl;
  std::cout << "URL: "
	    << url_str << std::endl;
  std::cout << "Number of threads: "
	    << numberOfThreads << std::endl;
  std::cout << "Duration: "
	    << duration << " s" << std::endl;
  std::cout << "Number of requests: "
	    << totalRequests << std::endl;
  std::cout << "Completed requests: "
	    << completedRequests << " ("
	    << Round(completedRequests*100.0/totalRequests)
	    << "%)" << std::endl;
  std::cout << "Failed requests: "
	    << failedRequests << " ("
	    << Round(failedRequests*100.0/totalRequests)
	    << "%)" << std::endl;
  std::cout << "Completed requests per min: "
            << Round(((double)completedRequests)/duration*60)
            << std::endl;
  std::cout << "Average response time for all requests: "
	    << Round(1000*totalTime.as_double()/totalRequests)
	    << " ms" << std::endl;
  if (completedRequests!=0)
    std::cout << "Average response time for completed requests: "
	      << Round(1000*completedTime.as_double()/completedRequests)
	      << " ms" << std::endl;
  if (failedRequests!=0)
    std::cout << "Average response time for failed requests: "
	      << Round(1000*failedTime.as_double()/failedRequests)
	      << " ms" << std::endl;
  std::cout << "========================================" << std::endl;

  Arc::final_xmlsec();

  return 0;
}
