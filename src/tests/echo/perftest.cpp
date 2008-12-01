#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// perftest.cpp

#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <glibmm/thread.h>
#include <glibmm/timer.h>

#include <arc/ArcConfig.h>
#include <arc/message/MCCLoader.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/MCC.h>
#include <arc/StringConv.h>
#include <arc/Logger.h>

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

// The configuration string.
std::string confString = "\
    <ArcConfig\
      xmlns=\"http://www.nordugrid.org/schemas/ArcConfig/2007\"\
      xmlns:tcp=\"http://www.nordugrid.org/schemas/ArcMCCTCP/2007\">\
     <ModuleManager>\
        <Path>.libs/</Path>\
        <Path>../../hed/mcc/http/.libs/</Path>\
        <Path>../../hed/mcc/soap/.libs/</Path>\
        <Path>../../hed/mcc/tls/.libs/</Path>\
        <Path>../../hed/mcc/tcp/.libs/</Path>\
        <Path>../../lib/arc/</Path>\
     </ModuleManager>\
     <Plugins><Name>mcctcp</Name></Plugins>\
     <Plugins><Name>mcctls</Name></Plugins>\
     <Plugins><Name>mcchttp</Name></Plugins>\
     <Plugins><Name>mccsoap</Name></Plugins>\
     <Chain>\
      <Component name='tcp.client' id='tcp'><tcp:Connect><tcp:Host>HOSTNAME</tcp:Host><tcp:Port>PORTNUMBER</tcp:Port></tcp:Connect></Component>\
      <Component name='tls.client' id='tls'><next id='tcp'/></Component> \
      <Component name='http.client' id='http'><next id='tls'/><Method>POST</Method><Endpoint>HTTPPATH</Endpoint></Component>\
      <Component name='soap.client' id='soap' entry='soap'><next id='http'/></Component>\
     </Chain>\
    </ArcConfig>";

// Replace a substring by another substring.
void replace(std::string& str,
	     const std::string& out,
	     const std::string& in)
{
  std::string::size_type index = str.find(out);
  if (index!=std::string::npos)
    str.replace(index, out.size(), in);
}

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
      
  while(run){
    
    // Create a client chain.
    Arc::Config client_config(confString);
    if(!client_config) {
      std::cerr << "Failed to load client configuration." << std::endl;
      return;
    }
    Arc::MCCLoader client_loader(client_config);
    Arc::MCC* client_entry = client_loader["soap"];
    if(!client_entry) {
      std::cerr << "Client chain have no entry point." << std::endl;
      return;
    }
    connected=true;

    Arc::MessageContext context;
    while(run and connected){
      // Prepare the request.
      Arc::NS echo_ns;
      echo_ns["echo"]="urn:echo";
      Arc::PayloadSOAP req(echo_ns);
      req.NewChild("echo").NewChild("say")="HELLO";
      Arc::Message reqmsg;
      Arc::Message repmsg;
      Arc::MessageAttributes attributes_req;
      Arc::MessageAttributes attributes_rep;
      reqmsg.Attributes(&attributes_req);
      reqmsg.Context(&context);
      reqmsg.Payload(&req);
      repmsg.Attributes(&attributes_rep);
      repmsg.Context(&context);
      
      // Send the request and time it.
      tBefore.assign_current_time();
      Arc::MCC_Status status = client_entry->process(reqmsg,repmsg);
      tAfter.assign_current_time();
      
      if(!status) {
        // Request failed.
        failedRequests++;
        failedTime+=tAfter-tBefore;
	    connected=false;
      } else {
        Arc::PayloadSOAP* resp = NULL;
        if(repmsg.Payload() != NULL) {
          // There is response.
          try{
            resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
          }
          catch(std::exception&){ };
        }
        if(resp == NULL) {
          // Response was not SOAP or no response at all.
          failedRequests++;
          failedTime+=tAfter-tBefore;
          connected=false;
        } else {
          std::string xml;
          resp->GetXML(xml);
          if (std::string((*resp)["echoResponse"]["hear"]).size()==0){
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
      if(repmsg.Payload()) delete repmsg.Payload();
    }
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
  std::string serviceHost;
  std::string portNumber;
  std::string httpPath("/Echo");
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
  if(config_file) {
    std::ifstream f(config_file);
    if(!f) {
      std::cerr << "File "<<config_file<<" can't be open for reading!" << std::endl;
      exit(EXIT_FAILURE);
    }
    std::getline<char>(f, confString, 0);
  }
  if(debug_level >= 0) {
    Arc::Logger::getRootLogger().setThreshold((Arc::LogLevel)debug_level);
    Arc::Logger::getRootLogger().addDestination(logcerr);
  }
  // Extract command line arguments.
  if (argc!=5){
    std::cerr << "Wrong number of arguments!" << std::endl
	      << std::endl
	      << "Usage:" << std::endl
	      << "perftest [-c config] [-d debug] host port threads duration" << std::endl
	      << std::endl
	      << "Arguments:" << std::endl
	      << "host     The name of the host of the service." << std::endl
	      << "port     The port to use on the host." << std::endl
	      << "threads  The number of concurrent requests." << std::endl
	      << "duration The duration of the test in seconds." << std::endl
	      << "config   The file containing client chain XML configuration with " << std::endl
              << "         'soap' entry point and HOSTNAME, PORTNUMBER and PATH " << std::endl
              << "         keyword for hostname, port and HTTP path of 'echo' service." << std::endl
	      << "debug    The textual representation of desired debug level. Available " << std::endl
              << "         levels: VERBOSE, DEBUG, INFO, WARNING, ERROR, FATAL." << std::endl;
    exit(EXIT_FAILURE);
  }
  serviceHost = std::string(argv[1]);
  portNumber = std::string(argv[2]);
  numberOfThreads = atoi(argv[3]);
  duration = atoi(argv[4]);
  
  // Insert host name and port number into the configuration string.
  replace(confString, "HOSTNAME", serviceHost);
  replace(confString, "PORTNUMBER", portNumber);
  replace(confString, "HTTPPATH", httpPath);

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
  std::cout << "Host: "
	    << serviceHost << std::endl;
  std::cout << "Port: "
	    << portNumber << std::endl;
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
