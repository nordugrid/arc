#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// perftest_samlaa.cpp

#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <glibmm/thread.h>
#include <glibmm/timer.h>

#include <arc/GUID.h>
#include <arc/ArcConfig.h>
#include <arc/UserConfig.h>
#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/MCC.h>
#include <arc/communication/ClientInterface.h>

#include <arc/credential/Credential.h>
#include <arc/xmlsec/XmlSecUtils.h>
#include <arc/xmlsec/XMLSecNode.h>

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
bool alwaysReconnect = false;

static std::string cert("../../tests/echo/usercert.pem");
static std::string key("../../tests/echo/userkey-nopass.pem");
static std::string cafile("../../tests/echo/testcacert.pem");
static std::string cadir("../../tests/echo/certificates");

#define SAML_NAMESPACE "urn:oasis:names:tc:SAML:2.0:assertion"
#define SAMLP_NAMESPACE "urn:oasis:names:tc:SAML:2.0:protocol"

// Round off a double to an integer.
int Round(double x){
  return int(x+0.5);
}

static void prepareAttributeQuery(Arc::XMLNode& attr_query){
  Arc::Credential cred(cert, key, cadir, cafile, false);
  std::string local_dn_str = cred.GetDN();
  std::string local_dn;
  size_t pos1 = std::string::npos;
  size_t pos2;
  do { //The DN should be like "CN=test,O=UiO,ST=Oslo,C=NO", so we need to change the format here
    std::string str;
    pos2 = local_dn_str.find_last_of("/", pos1);
    if(pos2 != std::string::npos && pos1 == std::string::npos) {
      str = local_dn_str.substr(pos2+1);
      local_dn.append(str);
      pos1 = pos2-1;
    }
    else if (pos2 != std::string::npos && pos1 != std::string::npos) {
      str = local_dn_str.substr(pos2+1, pos1-pos2);
      local_dn.append(str);
      pos1 = pos2-1;
    }
    if(pos2 != (std::string::npos+1)) local_dn.append(",");
  }while(pos2 != std::string::npos && pos2 != (std::string::npos+1));

  //Compose <samlp:AttributeQuery/>
  std::string query_id = Arc::UUID();
  attr_query.NewAttribute("ID") = query_id;
  Arc::Time t;
  std::string current_time = t.str(Arc::UTCTime);
  attr_query.NewAttribute("IssueInstant") = current_time;
  attr_query.NewAttribute("Version") = std::string("2.0");

  //<saml:Issuer/>
  std::string issuer_name = local_dn;
  Arc::XMLNode issuer = attr_query.NewChild("saml:Issuer");
  issuer = issuer_name;
  issuer.NewAttribute("Format") = std::string("urn:oasis:names:tc:SAML:1.1:nameid-format:x509SubjectName");

  //<saml:Subject/>
  Arc::XMLNode subject = attr_query.NewChild("saml:Subject");
  Arc::XMLNode name_id = subject.NewChild("saml:NameID");
  name_id.NewAttribute("Format")=std::string("urn:oasis:names:tc:SAML:1.1:nameid-format:x509SubjectName");
  name_id = local_dn;

  //<saml:Attribute/>
  Arc::XMLSecNode attr_query_secnd(attr_query);
  std::string attr_query_idname("ID");
  attr_query_secnd.AddSignatureTemplate(attr_query_idname, Arc::XMLSecNode::RSA_SHA1);
  if(attr_query_secnd.SignNode(key,cert)) {
    std::cout<<"Succeeded to sign the signature under <samlp:AttributeQuery/>"<<std::endl;
  }
}

static bool verifySAML(Arc::PayloadSOAP* soap_resp) {
  Arc::XMLNode attr_resp;

  //<samlp:Response/>
  attr_resp = (*soap_resp).Body().Child(0);

  std::string str;
  attr_resp.GetXML(str);
  std::cout<<"SAML Response: "<<str<<std::endl;

  //Check validity of the signature on <samlp:Response/>
  std::string resp_idname = "ID";
  Arc::XMLSecNode attr_resp_secnode(attr_resp);
  if(attr_resp_secnode.VerifyNode(resp_idname, cafile, cadir, false, true)) {
    std::cout<<"Succeeded to verify the signature under <samlp:Response/>"<<std::endl;
  }
  else {
    std::cerr<<"Failed to verify the signature under <samlp:Response/>"<<std::endl;
    return false;
  }

  //<samlp:Status/>
  std::string statuscode_value = attr_resp["Status"]["StatusCode"].Attribute("Value");
  if(statuscode_value == "urn:oasis:names:tc:SAML:2.0:status:Success")
    std::cout<<"The StatusCode is Success"<<std::endl;
  else std::cerr<<"Can not find StatusCode"<<std::endl;

  //<saml:Assertion/>
  Arc::XMLNode assertion = attr_resp["saml:Assertion"];
  std::string assertion_idname = "ID";
  Arc::XMLSecNode assertion_secnode(assertion);
  if(assertion_secnode.VerifyNode(assertion_idname, cafile, cadir, false, false)) {
    std::cout<<"Succeeded to verify the signature under <saml:Assertion/>"<<std::endl;
  }
  else {
    std::cerr<<"Failed to verify the signature under <saml:Assertion/>"<<std::endl;
    return false;
  }
  return true;
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

  //std::string url_str("https://127.0.0.1:60000/echo");
  Arc::URL url(url_str);

  Arc::MCCConfig mcc_cfg;
  //Arc::UserConfig usercfg("");
  //usercfg.ApplyToConfig(mcc_cfg);
  if (!cert.empty())
    mcc_cfg.AddCertificate(cert);
  if (!key.empty())
    mcc_cfg.AddPrivateKey(key);
  if (!cafile.empty())
    mcc_cfg.AddCAFile(cafile);
  if (!cadir.empty())
    mcc_cfg.AddCADir(cadir);

  Arc::NS saml_ns;
  saml_ns["saml"] = SAML_NAMESPACE;
  saml_ns["samlp"] = SAMLP_NAMESPACE;
 
  Arc::init_xmlsec();
 
  while(run){
    
    // Create a Client.
    Arc::ClientSOAP *client = NULL;
    client = new Arc::ClientSOAP(mcc_cfg,url,60);

    connected=true;
    while(run and connected){
      // Prepare the request.
      Arc::XMLNode attr_query(saml_ns,"samlp:AttributeQuery");
      prepareAttributeQuery(attr_query);
      Arc::NS soap_ns;
      Arc::SOAPEnvelope envelope(soap_ns);
      envelope.NewChild(attr_query);
      Arc::PayloadSOAP req(envelope);
      std::string tmp;
      req.GetXML(tmp);
      //std::cout<<"SOAP request: "<<tmp<<std::endl<<std::endl;

      // Send the request and time it.
      tBefore.assign_current_time();
      Arc::PayloadSOAP* resp = NULL;

      Arc::MCC_Status status = client->process(&req,&resp);

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
          //std::string xml;
          //resp->GetXML(xml);
          if (!verifySAML(resp)){
            // The response was not what it should be.
            failedRequests++;
            failedTime+=tAfter-tBefore;
            connected=false;
          }
          else{
            // Everything worked just fine!
            completedRequests++;
            completedTime+=tAfter-tBefore;
          }
        }
      }
      if(resp) delete resp;
      if(alwaysReconnect) connected=false;
    }
    if(client) delete client;
  }

  Arc::final_xmlsec();

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
      debug_level=Arc::istring_to_level(argv[2]);
      argv[2]=argv[0]; argv+=2; argc-=2;
    } else if(strcmp(argv[1],"-r") == 0) {
      alwaysReconnect=true; argv+=1; argc-=1;
    } else {
      break;
    };
  } 
  if(debug_level >= 0) {
    Arc::Logger::getRootLogger().setThreshold((Arc::LogLevel)debug_level);
    Arc::Logger::getRootLogger().addDestination(logcerr);
  }
  // Extract command line arguments.
  if (argc!=4){
    std::cerr << "Wrong number of arguments!" << std::endl
	      << std::endl
	      << "Usage:" << std::endl
	      << "perftest [-c config] [-d debug] [-r] url threads duration" << std::endl
	      << std::endl
	      << "Arguments:" << std::endl
	      << "url       The url of the service." << std::endl
	      << "threads   The number of concurrent requests." << std::endl
	      << "duration  The duration of the test in seconds." << std::endl
	      << "-c config The file containing client chain XML configuration with " << std::endl
              << "          'soap' entry point and HOSTNAME, PORTNUMBER and PATH " << std::endl
              << "           keyword for hostname, port and HTTP path of 'echo' service." << std::endl
	      << "-d debug   The textual representation of desired debug level. Available " << std::endl
              << "           levels: DEBUG, VERBOSE, INFO, WARNING, ERROR, FATAL." << std::endl
	      << "-r         If specified close connection and reconnect after " << std::endl
              << "           every request." << std::endl;
    exit(EXIT_FAILURE);
  }
  url_str = std::string(argv[1]);
  numberOfThreads = atoi(argv[2]);
  duration = atoi(argv[3]);

  // Start threads.
  run=true;
  finishedThreads=0;
  //Glib::thread_init();
  mutex=new Glib::Mutex;
  threads = new Glib::Thread*[numberOfThreads];
  for (i=0; i<numberOfThreads; i++)
    threads[i]=Glib::Thread::create(sigc::ptr_fun(sendRequests),true);

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
  std::cout << "Completed requests per second: "
            << Round(completedRequests/duration)
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

  return 0;
}
