#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// perftest.cpp

#include <chrono>
#include <cmath>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include <arc/GUID.h>
#include <arc/ArcConfig.h>
#include <arc/UserConfig.h>
#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/MCC.h>
#include <arc/communication/ClientInterface.h>

// Some global shared variables...
std::mutex mutex;
bool run;
int finishedThreads;
unsigned long completedRequests;
unsigned long failedRequests;
unsigned long totalRequests;
std::chrono::system_clock::duration completedTime;
std::chrono::system_clock::duration failedTime;
std::chrono::system_clock::duration totalTime;
std::string url_str;
bool alwaysReconnect = false;

// Send requests and collect statistics.
void sendRequests(){
  // Some variables...
  unsigned long completedRequests = 0;
  unsigned long failedRequests = 0;
  std::chrono::system_clock::duration completedTime;
  std::chrono::system_clock::duration failedTime;
  bool connected;

  //std::string url_str("https://127.0.0.1:60000/echo");
  Arc::URL url(url_str);

  Arc::MCCConfig mcc_cfg;
  Arc::UserConfig usercfg("");
  usercfg.ApplyToConfig(mcc_cfg);

  Arc::NS echo_ns; echo_ns["echo"]="http://www.nordugrid.org/schemas/echo";

  while(run){

    // Create a Client.
    Arc::ClientSOAP *client = NULL;
    client = new Arc::ClientSOAP(mcc_cfg,url,60);

    connected=true;
    while(run and connected){
      // Prepare the request.
      Arc::PayloadSOAP req(echo_ns);
      req.NewChild("echo:echo").NewChild("echo:say")="HELLO";

      // Send the request and time it.
      auto tBefore = std::chrono::system_clock::now();
      Arc::PayloadSOAP* resp = NULL;

      //std::string str;
      //req.GetXML(str);
      //std::cout<<"request: "<<str<<std::endl;
      Arc::MCC_Status status = client->process(&req,&resp);

      auto tAfter = std::chrono::system_clock::now();

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
      if(resp) delete resp;
      if(alwaysReconnect) connected=false;
    }
    if(client) delete client;
  }

  // Update global variables.
  std::unique_lock<std::mutex> lock(mutex);
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
  std::thread* threads;
  const char* config_file = NULL;
  int debug_level = -1;
  Arc::LogStream logcerr(std::cerr);

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
  threads = new std::thread[numberOfThreads];
  for (i=0; i<numberOfThreads; i++)
    threads[i] = std::thread(sendRequests);

  // Sleep while the threads are working.
  std::this_thread::sleep_for(std::chrono::seconds(duration));

  // Stop the threads
  run=false;
  while(finishedThreads<numberOfThreads)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Print the result of the test.
  std::unique_lock<std::mutex> lock(mutex);
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
            << rint(completedRequests * 100.0 / totalRequests)
            << "%)" << std::endl;
  std::cout << "Failed requests: "
            << failedRequests << " ("
            << rint(failedRequests * 100.0 / totalRequests)
            << "%)" << std::endl;
  std::cout << "Completed requests per second: "
            << rint((double)completedRequests / duration)
            << std::endl;
  std::cout << "Average response time for all requests: "
            << rint(std::chrono::duration<double, std::milli>(totalTime).count() / totalRequests)
            << " ms" << std::endl;
  if (completedRequests!=0)
    std::cout << "Average response time for completed requests: "
              << rint(std::chrono::duration<double, std::milli>(completedTime).count() / completedRequests)
              << " ms" << std::endl;
  if (failedRequests!=0)
    std::cout << "Average response time for failed requests: "
              << rint(std::chrono::duration<double, std::milli>(failedTime).count() / failedRequests)
              << " ms" << std::endl;
  std::cout << "========================================" << std::endl;

  return 0;
}
