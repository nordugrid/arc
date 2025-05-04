#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// perftest_deleg_bydelegclient.cpp

#include <chrono>
#include <cmath>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include <arc/GUID.h>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/MCC.h>
#include <arc/communication/ClientInterface.h>
#include <arc/communication/ClientX509Delegation.h>

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

// Send requests and collect statistics.
void sendRequests(){
  // Some variables...
  unsigned long completedRequests = 0;
  unsigned long failedRequests = 0;
  std::chrono::system_clock::duration completedTime(0);
  std::chrono::system_clock::duration failedTime(0);
  bool connected;

  //std::string url_str("https://127.0.0.1:60000/echo");
  Arc::URL url(url_str);

  Arc::MCCConfig mcc_cfg;
  mcc_cfg.AddPrivateKey("../echo/userkey-nopass.pem");
  mcc_cfg.AddCertificate("../echo/usercert.pem");
  mcc_cfg.AddCAFile("../echo/testcacert.pem");
  mcc_cfg.AddCADir("../echo/certificates");

  while(run){

    // Create a Client.
    Arc::ClientX509Delegation *client = NULL;
    client = new Arc::ClientX509Delegation(mcc_cfg,url);

    connected=true;
    while(run and connected){
      // Send the delegation request and time it.
      auto tBefore = std::chrono::system_clock::now();
      std::string arc_delegation_id;
      bool res = false;
      if(client) {
        if(!(res = client->createDelegation(Arc::DELEG_ARC, arc_delegation_id))) {
          std::cerr<<"Delegation to ARC delegation service failed"<<std::endl;
        }
      }
      auto tAfter = std::chrono::system_clock::now();

      if(!res) {
        // The delegation has not succeeded.
        failedRequests++;
        failedTime+=tAfter-tBefore;
        connected=false;
      }
      else {
        // Everything worked just fine.
        std::cout<<"Delegation ID: "<<arc_delegation_id<<std::endl;
        completedRequests++;
        completedTime+=tAfter-tBefore;
      }
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
              << "perftest_deleg_bydelegclient [-c config] [-d debug] url threads duration" << std::endl
              << std::endl
              << "Arguments:" << std::endl
              << "url     The url of the delegation service." << std::endl
              << "threads  The number of concurrent requests." << std::endl
              << "duration The duration of the test in seconds." << std::endl
              << "config   The file containing client chain XML configuration with " << std::endl
              << "         'soap' entry point and HOSTNAME, PORTNUMBER and PATH " << std::endl
              << "         keyword for hostname, port and HTTP path of 'echo' service." << std::endl
              << "debug    The textual representation of desired debug level. Available " << std::endl
              << "         levels: DEBUG, VERBOSE, INFO, WARNING, ERROR, FATAL." << std::endl;
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
