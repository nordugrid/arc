// perftest.cpp

#include <iostream>
#include <string>
#include <stdlib.h>
#include <glibmm/thread.h>
#include <glibmm/timer.h>

#include "../../libs/common/ArcConfig.h"
#include "../../hed/libs/loader/Loader.h"
#include "../../hed/libs/message/SOAPMessage.h"
#include "../../hed/libs/message/PayloadSOAP.h"

// Some global shared variables...
Glib::Mutex* mutex;
bool run;
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
     </ModuleManager>\
     <Plugins><Name>mcctcp</Name></Plugins>\
     <Plugins><Name>mcctls</Name></Plugins>\
     <Plugins><Name>mcchttp</Name></Plugins>\
     <Plugins><Name>mccsoap</Name></Plugins>\
     <Chain>\
      <Component name='tcp.client' id='tcp'><tcp:Connect><tcp:Host>HOSTNAME</tcp:Host><tcp:Port>PORTNUMBER</tcp:Port></tcp:Connect></Component>\
      <!-- <Component name='tls.client' id='tls'><next id='tcp'/></Component> -->\
      <Component name='http.client' id='http'><next id='tcp'/><Method>POST</Method><Endpoint>/Echo</Endpoint></Component>\
      <Component name='soap.client' id='soap' entry='soap'><next id='http'/></Component>\
     </Chain>\
    </ArcConfig>";

// Replace a substring by another substring.
void replace(std::string& str,
	     const std::string& out,
	     const std::string& in)
{
  int index = str.find(out);
  if (index!=std::string::npos)
    str.replace(index, out.size(), in);
}

// Round off a double to an integer.
int round(double x){
  return int(x+0.5);
}

// Send requests and collect statistics.
void sendRequests(){
  // Some variables...
  unsigned long completedRequests;
  unsigned long failedRequests;
  Glib::TimeVal completedTime(0,0);
  Glib::TimeVal failedTime(0,0);
  Glib::TimeVal tBefore;
  Glib::TimeVal tAfter;
      
  // Create a client chain.
  Arc::Config client_config(confString);
  if(!client_config) {
    std::cerr << "Failed to load client configuration." << std::endl;
    return;
  }
  Arc::Loader client_loader(&client_config);
  Arc::MCC* client_entry = client_loader["soap"];
  if(!client_entry) {
    std::cerr << "Client chain have no entry point." << std::endl;
    return;
  }

  while(run){
    // Prepare the request.
    Arc::NS echo_ns;
    echo_ns["echo"]="urn:echo";
    Arc::PayloadSOAP req(echo_ns);
    req.NewChild("echo").NewChild("say")="HELLO";
    Arc::Message reqmsg;
    Arc::Message repmsg;
    reqmsg.Payload(&req);

    // Send the request and time it.
    tBefore.assign_current_time();
    Arc::MCC_Status status = client_entry->process(reqmsg,repmsg);
    tAfter.assign_current_time();

    if(!status) {
      // Request failed.
      failedRequests++;
      failedTime+=tAfter-tBefore;
    }
    Arc::PayloadSOAP* resp = NULL;
    if(repmsg.Payload() == NULL) {
      // There were no response.
      failedRequests++;
      failedTime+=tAfter-tBefore;
    }
    try{
      resp = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
    }
    catch(std::exception&){
      // Payload had wrong type.
      failedRequests++;
      failedTime+=tAfter-tBefore;
    }
    if(resp == NULL) {
      // Response was not SOAP.
      failedRequests++;
      failedTime+=tAfter-tBefore;
    }
    std::string xml;
    resp->GetXML(xml);
    if (std::string((*resp)["echoResponse"]["hear"]).size()==0){
      // The response was not what it should be.
      failedRequests++;
      failedTime+=tAfter-tBefore;
    }
    else{
      // Everything worked just fine!
      completedRequests++;
      completedTime+=tAfter-tBefore;
    }
  }

  // Update global variables.
  Glib::Mutex::Lock lock(*mutex);
  ::completedRequests+=completedRequests;
  ::failedRequests+=failedRequests;
  ::completedTime+=completedTime;
  ::failedTime+=failedTime;
}

int main(int argc, char* argv[]){
  // Some variables...
  std::string serviceHost;
  std::string portNumber;
  int numberOfThreads;
  int duration;
  int i;
  Glib::Thread** threads;

  // Extract command line arguments.
  if (argc!=5){
    std::cerr << "Wrong number of arguments!" << std::endl
	      << std::endl
	      << "Usage:" << std::endl
	      << "perftest host port threads duration" << std::endl
	      << std::endl
	      << "Arguments:" << std::endl
	      << "host     The name of the host of the service." << std::endl
	      << "port     The port to use on the host." << std::endl
	      << "threads  The number of concurrent requests." << std::endl
	      << "duration The duration of the test in seconds." << std::endl;
    exit(EXIT_FAILURE);
  }
  serviceHost = std::string(argv[1]);
  portNumber = std::string(argv[2]);
  numberOfThreads = atoi(argv[3]);
  duration = atoi(argv[4]);
  
  // Insert host name and port number into the configuration string.
  replace(confString, "HOSTNAME", serviceHost);
  replace(confString, "PORTNUMBER", portNumber);

  // Start threads.
  run=true;
  //Glib::thread_init();
  mutex=new Glib::Mutex;
  threads = new Glib::Thread*[numberOfThreads];
  for (i=0; i<numberOfThreads; i++)
    threads[i]=Glib::Thread::create(sigc::ptr_fun(sendRequests),true);

  // Sleep while the threads are working.
  Glib::usleep(duration*1000000);

  // Stop the threads.
  run=false;
  for (i=0; i<numberOfThreads; i++)
    threads[i]->join();
  delete[] threads;
  delete mutex;

  // Print the result of the test.
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
	    << round(completedRequests*100.0/totalRequests)
	    << "%)" << std::endl;
  std::cout << "Failed requests: "
	    << failedRequests << " ("
	    << round(failedRequests*100.0/totalRequests)
	    << "%)" << std::endl;
  std::cout << "Average response time for all requests: "
	    << round(1000*totalTime.as_double()/totalRequests)
	    << " ms" << std::endl;
  if (completedRequests!=0)
    std::cout << "Average response time for completed requests: "
	      << round(1000*completedTime.as_double()/completedRequests)
	      << " ms" << std::endl;
  if (failedRequests!=0)
    std::cout << "Average response time for failed requests: "
	      << round(1000*failedTime.as_double()/failedRequests)
	      << " ms" << std::endl;
  std::cout << "========================================" << std::endl;

  return 0;
}
