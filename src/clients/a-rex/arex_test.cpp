// arex_test.cpp
// A small test program that creates an A-REX client and calls its
// three methods.

#include <iostream>
#include <fstream>
#include <string>
#include <signal.h>
#include "arex_client.h"

int main(){
  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  signal(SIGPIPE,SIG_IGN);
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::rootLogger.addDestination(logcerr);
  try{
    Arc::AREXClient ac;
    std::string jobid;
    std::ifstream jsdlfile("jsdl.xml");
    jobid = ac.submit(jsdlfile);
    std::cout << "Submitted a job: " << jobid << std::endl;
    std::cout << "Status of the job: " << ac.stat(jobid) << std::endl;
    ac.kill(jobid);
    std::cout << "Killed the job!" << std::endl;
    return 0;
  }
  catch (Arc::AREXClientError err){
    std::cerr << "ERROR: " << err.what() << std::endl;
    return -1;
  }
}
